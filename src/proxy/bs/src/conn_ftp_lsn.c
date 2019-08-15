#include "conn_ftp_lsn.h"
#include "connection.h"
#include "log.h"
#include "conn_common.h"
#include "readconf.h"
#include "conn_ftp.h"
#include "fdevents.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static connection_t *ftp_lsn_conn = NULL;

static int ftp_lsn_handle( int events, connection_t *c )
{
    int cfd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr);
    int i;

    if( ! (events & FDEVENT_IN) )
        return CONN_ERR;

    for( i=0; i<FTP_LSN_NUM; i++ )
    {
        cfd = accept( c->fd, (struct sockaddr *)&addr, &addrlen );
        if( cfd < 0 )
        {
            switch( errno )
            {
                case EAGAIN:
#if EWOULDBLOCK != EAGAIN
                case EWOULDBLOCK:
#endif
                case EINTR:
                case ECONNABORTED: 	
                case EMFILE:
                    break;
                default:
                    log_warn( "accept ftp connection error %d, %s", errno, strerror(errno) );
                    continue;
            }
        }
        else
        {
            log_debug( "new ftp connection, fd:%d", cfd );
            ftp_conn_create( cfd, &addr );
        }
    }
    return CONN_OK;
}

int ftp_lsn_init()
{
    int fd;
    int ret;
    connection_t *c;
    struct sockaddr_in *addr;

    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( fd < 0 )
    {
        log_error( "create ftp listen socket failed, %s", strerror(errno) );
        return CONN_ERR;
    }

    ret = setsockreuse( fd );
    if( ret < 0 )
    {
        log_error( "set ftp listen socket reuse addr failed, %s", strerror(errno) );
        goto errfd;
    }
    ret = setnonblocking( fd );
    if( ret < 0 )
    {
        log_error( "set ftp listen socket nonblocking addr failed, %s", strerror(errno) );
        goto errfd;
    }

    c = connection_new( fd, CONTYPE_FTP_LSN );
    if( NULL == c )
    {
        log_error( "create new ftp listen connection failed" );
        goto errfd;
    }

    addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == addr )
    {
        log_error( "malloc ftp listen addr failed" );
        goto errconn;
    }

    memset( addr, 0, sizeof(struct sockaddr_in) );
    addr->sin_family = AF_INET;
    inet_aton( conf[FTP_LISTEN_ADDR].value.str, &(addr->sin_addr) );
    addr->sin_port = htons( conf[FTP_LISTEN_PORT].value.num );
    c->addr = (struct sockaddr *)addr;

    ret = conn_listen( c, FTP_LSN_NUM );
    if( 0 != ret )
        goto errconn;

    c->event_handle = ftp_lsn_handle;
    c->status = CONSTAT_WORKING;

    ret = ev_addfd( fd, FDEVENT_IN, c );
    if( FDEVENT_ERROR == ret )
    {
        log_error( "add ftp listen connection to epoll failed" );
        goto errconn;
    }

    ftp_lsn_conn = c;
    log_debug( "ftp listen connection %d created", fd );
    return CONN_OK;

errconn:
    connection_del( c );
errfd:
    close( fd );
    return CONN_ERR;
}

void ftp_lsn_close()
{
    if( NULL != ftp_lsn_conn )
    {
        log_debug( "ftp listen connection %d closed", ftp_lsn_conn->fd );
        connection_del( ftp_lsn_conn );
        ftp_lsn_conn = NULL;
    }
}

