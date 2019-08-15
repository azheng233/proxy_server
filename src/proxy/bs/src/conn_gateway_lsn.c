#include "conn_gateway_lsn.h"
#include "connection.h"
#include "log.h"
#include "conn_gateway.h"
#include "fdevents.h"
#include "conn_common.h"
#include "readconf.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

connection_t *gwlsn_conn;
extern connhead_t gw_connhead;

int gateway_lsn_handle( int events, connection_t *conn )
{
    if( events & FDEVENT_ERR )
        return CONN_ERR;

    int cfd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr);
    int errnum;

    int i = 0;
    for( i=0; i<GATEWAY_LSN_NUM; i++ )
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
                    log_warn( "accept gw connection error %d, %s", errnum, strerror(errnum) );
                    continue;
            }
        }
        else
        {
            log_debug( "new gw connection, fd:%d", cfd );
            gateway_conn_create( cfd, &addr );
        }
    }
    return 0;
}



/** 
 * @brief 初始化网关监听
 * 
 * @retval CONN_ERR 发生错误
 * @retval CONN_OK  成功
 */
int gateway_lsn_init()
{
    gwlsn_conn = NULL;

    int fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( fd < 0 )
    {
        log_error( "create gw listen socket failed, %s", strerror(errno) );
        return CONN_ERR;
    }

    int ret = setsockreuse( fd );
    if( ret < 0 )
    {
        log_error( "set gw listen socket %d reuse failed", fd );
        goto errfd;
    }
    ret = setnonblocking( fd );
    if( ret < 0 )
    {
        log_error( "set gw listen socket %d nonblocking failed", fd );
        goto errfd;
    }

    gwlsn_conn = connection_new( fd, CONTYPE_GATEWAY_LSN );
    if( NULL == gwlsn_conn )
    {
        log_error( "create new connection for gw listen failed" );
        goto errfd;
    }

    struct sockaddr_in *addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == addr )
    {
        log_error( "malloc gateway listen addr failed" );
        goto errconn;
    }

    memset( addr, 0, sizeof(struct sockaddr_in) );
    addr->sin_family = AF_INET;
    inet_aton( conf[GATEWAY_LISTEN_ADDR].value.str, &(addr->sin_addr) );
    addr->sin_port = htons( conf[GATEWAY_LISTEN_PORT].value.num );
    gwlsn_conn->addr = (struct sockaddr *)addr;

    ret = conn_listen( gwlsn_conn, GATEWAY_LSN_NUM );
    if( -1 == ret )
    {
        log_error( "gw listen connection failed to listen" );
        goto errconn;
    }

    gwlsn_conn->event_handle = gateway_lsn_handle;
    gwlsn_conn->status = CONSTAT_WORKING;

    ret = ev_addfd( fd, FDEVENT_IN, gwlsn_conn );
    if( FDEVENT_ERROR == ret )
    {
        log_error( "add gw listen connection %d to epoll failed", gwlsn_conn->fd );
        goto errconn;
    }
    log_debug( "gw listen connection %d created", gwlsn_conn->fd );

    conn_queue_init( &gw_connhead );
    return CONN_OK;

errconn:
    connection_del( gwlsn_conn );
    gwlsn_conn = NULL;
errfd:
    close( fd );
    return CONN_ERR;
}


/** 
 * @brief 关闭网关监听
 */
void gateway_lsn_close()
{
    if( NULL == gwlsn_conn )
        return;
    log_debug( "gw listen connection %d closed", gwlsn_conn->fd );
    connection_del( gwlsn_conn );
    gwlsn_conn = NULL;
}


void gateway_connections_destroy()
{
    conn_queue_destroy( &gw_connhead );
}
