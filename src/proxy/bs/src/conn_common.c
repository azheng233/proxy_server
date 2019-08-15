#include "conn_common.h"
#include "fdevents.h"
#include "log.h"
#include "chunk.h"
#include "job.h"
#include "monitor_log.h"
#include "forward.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/un.h>
#include <linux/tcp.h>

/** 
 * @brief 设置socket地址可重用
 * 
 * @param[in] fd socket描述符
 * 
 * @retval -1 出错
 * @retval 0 成功
 */
int setsockreuse( int fd )
{
    int on = 1;
    return setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int) );
}

int setsocknodelay( int fd )
{
    int on = 1;
    return setsockopt( fd, IPPROTO_TCP, SO_REUSEADDR|TCP_NODELAY, (char *)&on, sizeof(int) );
}

void setsockbufsize( int fd )
{
    int buf_size = 200*1024;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
}

/** 
 * @brief 设置socket为非阻塞模式
 * 
 * @param[in] fd socket描述符
 * 
 * @retval -1 出错
 * @retval 0 成功
 */
int setnonblocking( int fd )
{
    int opts;
    if( fd < 0 )
        return -1;
    opts = fcntl( fd, F_GETFL );
    if( opts<0 )
        return -1;
    opts |= O_NONBLOCK;
    if( fcntl( fd, F_SETFL, opts ) < 0 )
        return -1;
//    setsockbufsize( fd );
    return setsocknodelay( fd );
}

int setnonblocking_notsockfd( int fd )
{
    int opts;
    if( fd < 0 )
        return -1;
    opts = fcntl( fd, F_GETFL );
    if( opts<0 )
        return -1;
    opts |= O_NONBLOCK;
    if( fcntl( fd, F_SETFL, opts ) < 0 )
        return -1;
    return 0;
}

/** 
 * @brief 使 \a conn 开始监听
 *
 * 监听地址为 \a conn->addr
 * 
 * @param[in] conn
 * @param[in] listenum 监听队列长度
 * 
 * @retval -1 出错
 * @retval 0 成功
 */
int conn_listen( connection_t *conn, int listenum )
{
    int ret = bind( conn->fd, conn->addr, sizeof(struct sockaddr) );
    if( -1 == ret )
    {
        log_error( "bind %d failed, %d, %s\n", conn->fd, errno, strerror(errno) );
        return -1;
    }
    ret = listen( conn->fd, listenum );
    if( -1 == ret )
    {
        log_error( "listen %d failed, %s\n", conn->fd, strerror(errno) );
        return -1;
    }
    return 0;
}


#define log_backend_connect_result( conn, result ) \
    if( CONTYPE_BACKEND==(conn)->type || CONTYPE_SSL_BACKEND==(conn)->type )    \
    { \
        if( (conn)->peer && ((connection_t*)((conn)->peer))->context )          \
            mlog_log_appstat( get_prev_url( (conn)->peer ), result );           \
    }

/** 
 * @brief 使 \a conn 开始连接
 *
 * 连接地址为 \a conn->addr, 将连接加入epoll
 * 
 * @param[in] conn
 * 
 * @retval -1 出错
 * @retval 0 成功
 */
int conn_connect( connection_t *conn )
{
    int events = 0;
    if( NULL == conn->addr )
        return -1;

    socklen_t addrlen = sizeof(struct sockaddr_in);
    if( AF_UNIX == conn->addr->sa_family )
        addrlen = SUN_LEN( (struct sockaddr_un *)(conn->addr) );

    int ret = connect( conn->fd, conn->addr, addrlen );
    if( ret == 0 )
    {
        conn->status = CONSTAT_WORKING;
        events = FDEVENT_IN;
        if( ! mode_frontend )
            log_backend_connect_result( conn, APP_STAT_LIVE );
    }
    else if( EINPROGRESS == errno )
    {
        conn->status = CONSTAT_CONNECTING;
        events = FDEVENT_OUT;
    }
    else
    {
        if( ! mode_frontend )
            log_backend_connect_result( conn, APP_STAT_LIVE );
        log_error( "connection %d connect failed, %s\n", conn->fd, strerror(errno) );
        return -1;
    }

    {
        struct sockaddr_in maddr = {0};
        socklen_t mlen = sizeof(maddr);
        if(0 == getsockname( conn->fd, (struct sockaddr *)&maddr, &mlen )) {
            log_debug( "----->backend %d, local port:%u", conn->fd, ntohs(maddr.sin_port) );
        }
    }

    return ev_addfd( conn->fd, events, conn );
}


int backend_handle_read( connection_t *conn )
{
    connection_t *peer =  conn->peer;
    if( NULL == peer )
    {
        log_warn( "client is null" );
        return -1;
    }
    if( peer->status != CONSTAT_WORKING && peer->status != CONSTAT_HTTPCONNECT )
    {
        log_warn( "client stat %d\n", peer->status );
        return -1;
    }
    log_debug( "client %d <== backend %d", peer->fd, conn->fd );

    chunk_t *ckfrom = conn->readbuf;
    chunk_t *ckto = peer->writebuf;

    if( chunk_isempty( ckfrom ) )
        return 0;

    queue_t *qfrom = &(ckfrom->queue);
    queue_t *qto = &(ckto->queue);

    qto->prev->next = qfrom->next;
    qfrom->next->prev = qto->prev;
    qto->prev = qfrom->prev;
    qfrom->prev->next = qto;
    queue_init( qfrom );

    ckto->num += ckfrom->num;
    ckto->total_size += ckfrom->total_size;
    ckto->used_size += ckfrom->used_size;
    ckfrom->num = 0;
    ckfrom->total_size = 0;
    ckfrom->used_size = 0;

    job_addconn( FDEVENT_OUT, peer );
    return 0;
}


int conn_signal_close( connection_t *conn )
{
    set_conn_status( conn, CONSTAT_CLOSED );
    if( job_addconn( FDEVENT_CLOSE, conn ) != JOB_OK )
        return CONN_ERR;
    return CONN_OK;
}


int backend_conn_connect( connection_t *conn, in_addr_t ip )
{
    ((struct sockaddr_in *)(conn->addr))->sin_addr.s_addr = ip;

    int ret = conn_connect( conn );
    if( -1 == ret )
        return CONN_ERR;

    return CONN_OK;
}
