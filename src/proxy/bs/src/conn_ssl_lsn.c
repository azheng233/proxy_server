#include "conn_ssl_lsn.h"
#include "connection.h"
#include "conn_common.h"
#include "conn_ssl.h"
#include "log.h"
#include "fdevents.h"
#include "readconf.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

connection_t *ssl_lsn_conn;


int ssl_lsn_handle( int events, connection_t *conn )
{
    if( events & FDEVENT_ERR )
        return CONN_ERR;

    int cfd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr);
    int errnum;

    int i = 0;
    for( i=0; i<SSL_LSN_NUM; i++ )
    {
        cfd = accept( conn->fd, (struct sockaddr *)&addr, &addrlen );
        if( cfd < 0 )
        {
            errnum = errno;
            switch( errnum )
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
                    log_warn( "accept ssl connection error %d, %s", errnum, strerror(errnum) );
                    continue;
            }
        }
        else
        {
            log_debug( "new ssl connection, fd:%d, ip:%s, port:%d", cfd, inet_ntoa(addr.sin_addr), addr.sin_port );
            ssl_conn_create( cfd, &addr );
        }
    }
    return 0;
}


int ssl_lsn_init()
{
    ssl_lsn_conn = NULL;

    int fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( fd < 0 )
    {
        log_error( "create ssl listen socket failed, %s", strerror(errno) );
        return CONN_ERR;
    }

    int ret = setsockreuse( fd );
    if( ret < 0 )
        goto errfd;
    ret = setnonblocking( fd );
    if( ret < 0 )
        goto errfd;

    ssl_lsn_conn = connection_new( fd, CONTYPE_SSL_LSN );
    if( NULL == ssl_lsn_conn )
    {
        log_error( "new ssl listen connection failed, fd:%d", fd );
        goto errfd;
    }

    struct sockaddr_in *addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == addr )
    {
        log_error( "malloc ssl listen addr failed" );
        goto errconn;
    }

    memset( addr, 0, sizeof(struct sockaddr_in) );
    addr->sin_family = AF_INET;
    inet_aton( conf[SSL_LISTEN_ADDR].value.str, &(addr->sin_addr) );
    addr->sin_port = htons( conf[SSL_LISTEN_PORT].value.num );
    ssl_lsn_conn->addr = (struct sockaddr *)addr;

    ret = conn_listen( ssl_lsn_conn, SSL_LSN_NUM );
    if( -1 == ret )
        goto errconn;

    ssl_lsn_conn->event_handle = ssl_lsn_handle;
    ssl_lsn_conn->status = CONSTAT_WORKING;

    ret = ev_addfd( fd, FDEVENT_IN, ssl_lsn_conn );
    if( FDEVENT_ERROR == ret )
    {
        log_error( "add ssl lsn to epoll failed, fd:%d", fd );
        goto errconn;
    }

    log_debug( "ssl listen connection %d created", fd );
    return CONN_OK;

errconn:
    connection_del( ssl_lsn_conn );
    ssl_lsn_conn = NULL;
errfd:
    close( fd );
    return CONN_ERR;
}


void ssl_lsn_close()
{
    if( NULL == ssl_lsn_conn )
        return;
    log_debug( "ssl listen connection %d closed", ssl_lsn_conn->fd );
    connection_del( ssl_lsn_conn );
    ssl_lsn_conn = NULL;
}
