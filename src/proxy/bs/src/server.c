/** 
 * @file server.c
 * @brief 主函数
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#include "connection.h"
#include "job.h"
#include "fdevents.h"
#include "conn_gateway_lsn.h"
#include "conn_policy.h"
#include "conn_client_lsn.h"
#include "conn_ssl_lsn.h"
#include "conn_peer.h"
#include "ssl_common.h"
#include "timehandle.h"
#include "log.h"
#include "options.h"
#include "readconf.h"
#include "ssl.h"
#include "conn_dns.h"
#include "dns_parse.h"
#include "dnscache.h"
#ifdef ENABLE_IPFILTE_MANAGER
#include "conn_ipfilter.h"
#endif
#include "conn_ftp_lsn.h"
#include "monitor_log.h"
#include "fd_rpc_listen.h"
#include "license.h"
#include "policy_cache.h"
#include "url_policy.h"
#include "conn_audit.h"
#include "heartbeat.h"
#include "ipt_redirect.h"
#include "log.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

static volatile sig_atomic_t isalarm = 0;
static volatile sig_atomic_t isterm = 0;
static void signal_handle( int sig, siginfo_t *si, void *context )
{
    static siginfo_t empty_siginfo;
    if( !si ) si = &empty_siginfo;
    if( sig==SIGALRM ) isalarm = 1;
    if( SIGTERM==sig || SIGINT==sig )
        isterm = sig;
}

static const char *program_name = "urlserver";
void server_exit()
{
    log_info( "%s service stop", program_name );
}

#define CARD_CHECK_SCRIPT "/usr/local/urlfilter/script/checkupcard"
static int check_card()
{
    int ret;
    struct stat st;

    ret = stat( CARD_CHECK_SCRIPT, &st );
    if( ret!=0 || 0==st.st_size )
    {
        log_trace( "stat %s return %d, %s", CARD_CHECK_SCRIPT, ret, strerror(errno) );
        return 0;
    }

    ret = system( CARD_CHECK_SCRIPT );
    log_trace( "system %s return %d, %s", CARD_CHECK_SCRIPT, ret, strerror(errno) );
    return ret;
}

//设置最大文件描述符数目
int set_nfile_limit( const unsigned int num )
{
    struct rlimit rlim = {0};

    rlim.rlim_cur = num;
    rlim.rlim_max = num;

    if( -1 == setrlimit( RLIMIT_NOFILE, &rlim ) )
    {
        log_error( "set file number limit to %u failed, %s", num, strerror(errno) );
        return -1;
    }

    log_debug( "set file number limit to %u", num );
    return 0;
}

int main( int argc, char *argv[] )
{
    int ret = 0;
    int nevs = 0;
    int i = 0;
    int fd = 0;
    int events = 0;
    connection_t *conn = NULL;
    job_t *job;
    
    // config
    ret = conf_init();
    if( -1 == ret )
    {
        log_error( "config init failed" );
        return -1;
    }
    atexit( conf_destroy );

    // options
    options_parse( argc, argv );
    program_name = basename( argv[0] );

    ret = readallconfig();
    if( ret != 0 )
    {
        log_error( "read config failed" );
        return -1;
    }

    if( daemonize )
    {
        ret = daemon( 0, 0 );
        if( -1 == ret )
            log_error( "daemon failed %d, %s", errno, strerror(errno) );
        log_set_mode( LOG_MODE_SYSLOG );
    }

    ret = set_nfile_limit( conf[CONNECTIONS_MAX].value.num );

    // init heartbeat
    hb_init( program_name, conf[HEARTBEAT_PATH].value.str );

    print_bar( program_name );

    if( ! mode_frontend )
    {
        ret = check_card();
        if( ret != 0 )
        {
            log_error( "check pci crypt card failed" );
            return -1;
        }
    }

    ret = license_verify( conf[LICENSE_FILE].value.str );
    mlog_set_devid( license_get_devid() );
    if( ret != 0 )
    {
        log_warn( "license verify failed" );
        mlog_alert( "license verify failed" );
        return -1;
    }

    // signal & timer
    struct sigaction sigact;
    sigemptyset( &(sigact.sa_mask) );
    sigact.sa_flags = SA_SIGINFO;
    sigact.sa_sigaction = signal_handle;

    sigaction( SIGINT, &sigact, NULL );
    sigaction( SIGTERM, &sigact, NULL );
    sigaction( SIGALRM, &sigact, NULL );

    ret = time_init();

    if (!ssl_use_client_cert) {
        ret = ssl_init();
        if( SSL_OK != ret )
        {
            log_error( "ssl init failed" );
            return -1;
        }
        atexit( ssl_unload );
    }

    // job
    ret = job_init();
    if( ret != JOB_OK )
        goto err;
    atexit( job_destroy );
 
    // epoll
    ret = ev_init();
    if( FDEVENT_OK != ret )
    {
        log_error( "event init failed" );
        goto err;
    }
    atexit( ev_free );
    log_trace( "event init ok" );

    if( mode_frontend )
    {
        if( conf[TRANSPARENT_ENABLE].value.num )
        {
            ret = ipt_redirect_init( );
            if( 0 != ret )
            {
                log_error( "init iptables redirect failed" );
                goto err;
            }
            atexit( ipt_redirect_clear );
        }

        ret = policy_cache_create( conf[POLICY_CACHE_SIZE].value.num );
        if( 0 != ret )
        {
            log_error( "create policy cache failed" );
            goto err;
        }
        atexit( policy_cache_destroy );
        url_judge_set_method( conf[URL_CMP_METHOD].value.str );

        ret = rpc_listen_create();
        if( CONN_OK != ret )
        {
            log_error( "rpc listen create failed" );
            goto err;
        }
        atexit( rpc_listen_close );
        atexit( rpc_connections_destroy );

        ret = gateway_lsn_init();
        if( CONN_OK != ret )
        {
            log_error( "gateway listen init failed" );
            goto err;
        }
        atexit( gateway_lsn_close );
        atexit( gateway_connections_destroy );

        ret = policy_conn_create();
        if( CONN_OK != ret )
        {
            log_error( "policy connection create failed" );
            goto err;
        }
        atexit( policy_conn_close );

        ret = audit_conn_create();
        if( CONN_OK != ret )
        {
            log_error( "audit connection create failed" );
            goto err;
        }
        atexit( audit_conn_close );

#ifndef _DEBUG_NOSSL
        ret = ssl_lsn_init();
        if( CONN_OK != ret )
        {
            log_error( "ssl listen init failed" );
            goto err;
        }
        atexit( ssl_lsn_close );
#endif

        ret = ftp_lsn_init();
        if( CONN_OK != ret )
        {
            log_error( "ftp listen init failed" );
            goto err;
        }
        atexit( ftp_lsn_close );

#ifdef ENABLE_IPFILTE_MANAGER
        ret = ipfilter_conn_create();
        if( CONN_OK != ret )
        {
            log_error( "ipfilter connection create failed" );
            goto err;
        }
        atexit( ipfilter_conn_close );
#endif
    }
    else
    {
        ret = dnscache_init();
        if( 0 != ret )
        {
            log_error( "dns cache init failed" );
            goto err;
        }
        atexit( dnscache_free );

        ret = dns_tree_init();
        if( DNS_OK != ret )
        {
            log_error( "dns tree init failed" );
            goto err;
        }

        ret = dns_conn_create();
        if( CONN_OK != ret )
        {
            log_error( "dns connection create failed" );
            goto err;
        }
        atexit( dns_conn_close );

#ifndef _DEBUG_NOSSL
        if (ssl_use_client_cert) {
            ret = ssl_lsn_init();
            if( CONN_OK != ret )
            {
                log_error( "ssl listen init failed" );
                goto err;
            }
            atexit( ssl_lsn_close );
        }
#endif
    }

    // client listen
    ret = client_lsn_init();
    if( CONN_OK != ret )
    {
        log_error( "client listen init failed" );
        goto err;
    }
    atexit( client_lsn_close );
    atexit( connections_destroy );

    mlog_service_start( program_name );
    log_info( "%s service start", program_name );
    atexit( server_exit );

    hb_write_pid();

    // main loop
    while( ! isterm )
    {
        if( isalarm )
        {
            isalarm = 0;
            curtime = time( NULL );
            time_handle();
        }

        nevs = ev_wait();
        for( i=0; i<nevs; i++ )
        {
            ret = ev_getnext( &fd, &events, &conn );
            if( FDEVENT_ERROR == ret )
                break;
            conn->event_handle( events, conn );
        }

        job = job_getfirst();
        while( NULL != job )
        {
            if( ! job_isdel( job ) )
            {
                job->conn->event_handle( job->events, job->conn );
                job_del( job );
            }
            job = job_getnext( job );
        }
        job_clear();
    }

    log_debug( "got signal %d", isterm );
    mlog_service_stop( program_name );
    return 0;

err:
    return -1;
}
