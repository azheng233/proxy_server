#include "conn_client_lsn.h"
#include "connection.h"
#include "conn_common.h"
#include "conn_client.h"
#include "fdevents.h"
#include "log.h"
#include "readconf.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static connection_t *client_lsn_conn;
extern connhead_t connhead;


int client_lsn_handle( int events, connection_t *conn )
{
    if( events & FDEVENT_ERR )
        return CONN_ERR;

    int cfd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr);
    int errnum;

    int i = 0;
    for( i=0; i<CLIENT_LSN_NUM; i++ )
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
                    log_warn( "accept client connection error %d, %s", errnum, strerror(errnum) );
                    continue;
            }
        }
        else
        {
            log_debug( "new client connection, fd:%d, ip:%s, port:%d", cfd, inet_ntoa(addr.sin_addr), addr.sin_port );
            client_conn_create( cfd, &addr );
        }
    }
    return 0;
}


/** 
 * @brief 初始化客户端监听
 * 
 * @retval CONN_ERR 发生错误
 * @retval CONN_OK  成功
 */
int client_lsn_init()
{
    client_lsn_conn = NULL;

    int fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( fd < 0 )
    {
        log_error( "create client listen socket failed, %s", strerror(errno) );
        return CONN_ERR;
    }

    int ret = setsockreuse( fd );
    if( ret < 0 )
        goto errfd;
    ret = setnonblocking( fd );
    if( ret < 0 )
        goto errfd;

    client_lsn_conn = connection_new( fd, CONTYPE_CLIENT_LSN );
    if( NULL == client_lsn_conn )
        goto errfd;

    struct sockaddr_in *addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == addr )
    {
        log_error( "malloc client addr failed" );
        goto errconn;
    }

    memset( addr, 0, sizeof(struct sockaddr_in) );
    addr->sin_family = AF_INET;
    inet_aton( conf[CLIENT_LISTEN_ADDR].value.str, &(addr->sin_addr) );
    addr->sin_port = htons( conf[CLIENT_LISTEN_PORT].value.num );
    client_lsn_conn->addr = (struct sockaddr *)addr;

    ret = conn_listen( client_lsn_conn, CLIENT_LSN_NUM );
    if( -1 == ret )
        goto errconn;

    client_lsn_conn->event_handle = client_lsn_handle;
    client_lsn_conn->status = CONSTAT_WORKING;

    ret = ev_addfd( fd, FDEVENT_IN, client_lsn_conn );
    if( FDEVENT_ERROR == ret )
        goto errconn;
    log_debug( "client listen connection %d created", fd );

    conn_queue_init( &connhead );
    return CONN_OK;

errconn:
    connection_del( client_lsn_conn );
    client_lsn_conn = NULL;
errfd:
    close( fd );
    return CONN_ERR;
}


/** 
 * @brief 关闭客户端监听
 */
void client_lsn_close()
{
    if( NULL == client_lsn_conn )
        return;
    log_debug( "client listen connection %d closed", client_lsn_conn->fd );
    connection_del( client_lsn_conn );
    client_lsn_conn = NULL;
}

