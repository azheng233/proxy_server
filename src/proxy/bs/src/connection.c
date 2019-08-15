/** 
 * @file connection.c
 * @brief 连接操作和处理函数
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#include "connection.h"
#include "chunk.h"
#include "waitconn.h"
#include "timehandle.h"
#include "log.h"
#include "forward.h"
#include "ssl_common.h"
#include "dns_parse.h"
#include "queue.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/** 
 * @brief 初始化连接队列
 * 
 * @param[in] connhead 连接队列头
 * 
 * @return CONN_OK
 */
int conn_queue_init( connhead_t *connhead )
{
    queue_init( &(connhead->queue) );
    connhead->num = 0;
    return CONN_OK;
}


/** 
 * @brief 新建一个连接
 *
 * 创建一个指定类型的连接结构并与socket关联
 * 初始化连接，包括读写缓冲、等待队列和节点
 * 
 * @param[in] fd  socket描述符
 * @param[in] type 连接类型
 * 
 * @return 新的连接指针
 */
connection_t *connection_new( int fd, enum conn_type type )
{
    connection_t *conn = malloc( sizeof(connection_t) );
    if( NULL == conn )
        return NULL;

    conn->fd = fd;
    conn->type = type;
    conn->status = CONSTAT_INIT;
    conn->addr = NULL;
    conn->local_addr = NULL;
    conn->curevents = 0;
    conn->lsnevents = 0;

    conn->readbuf = chunk_create( CHUNKSIZE_LIMIT );
    if( NULL == conn->readbuf )
        goto errconn;
    conn->writebuf = chunk_create( CHUNKSIZE_LIMIT );
    if( NULL == conn->writebuf )
        goto errbuf;
    conn->upsize = 0;
    conn->downsize = 0;
    conn->mlog_up_flow = 0;
    conn->mlog_down_flow = 0;
     
    if( waitqueue_init( &(conn->waitq) ) != WQ_OK )
        goto errwbuf;
    if( waitnode_init( &(conn->wqnode) ) != WQ_OK )
        goto errwbuf;
    conn->wqnode.thisconn = conn;

    conn->peer = NULL;
    conn->context = NULL;
    conn->ssl = NULL;
    conn->event_handle = NULL;
    conn->get_leftsize = NULL;
    queue_init( &(conn->queue) );
    conn->timeinfo.time_create = curtime;
    conn->timeinfo.time_lastrec = curtime;
    conn->timeinfo.time_lastsend = curtime;
    conn->timeinfo.time_waitclose = 0;

    conn->rbnode = NULL;
    conn->action_prev = 0;

    log_trace( "new connection, fd:%d, type:%d", fd, type );
    return conn;

errwbuf:
    chunk_destroy( conn->writebuf );
errbuf:
    chunk_destroy( conn->readbuf );
errconn:
    connection_del( conn );
    return NULL;
}


/** 
 * @brief 删除一个连接
 *
 * 销毁缓冲区、等待队列和节点，关闭socket并释放内存
 * 
 * @param[in] conn 连接结构指针
 */
void connection_del( connection_t *conn )
{
    if( NULL == conn )
        return;

    waitconn_del( conn );
    waitqueue_clear( &(conn->waitq) );

    if( NULL != conn->rbnode )
        dns_delconn( conn );
    if( NULL != conn->addr )
        free( conn->addr );
    if( NULL != conn->local_addr )
        free( conn->local_addr );
    if( NULL != conn->readbuf )
        chunk_destroy( conn->readbuf );
    if( NULL != conn->writebuf )
        chunk_destroy( conn->writebuf );

    log_trace( "connection del, fd:%d, type:%d", conn->fd, conn->type );
    close( conn->fd );
    free( conn );
}


/** 
 * @brief 将一个连接加入队列
 * 
 * @param[in] connhead 连接队列头
 * @param[in] conn 连接
 * 
 * @retval CONN_ERR 错误，参数有空指针
 * @retval CONN_OK  成功
 */
int conn_add_queue( connhead_t *connhead, connection_t *conn )
{
    if( NULL==connhead || NULL==conn )
        return CONN_ERR;

    queue_insert_tail( &(connhead->queue), &(conn->queue) );
    connhead->num++;
    return CONN_OK;
}


/** 
 * @brief 从队列中去掉一个连接
 * 
 * @param[in] connhead 连接队列头
 * @param[in] conn 连接
 * 
 * @retval CONN_ERR 错误，参数有空指针
 * @retval CONN_OK  成功
 */
int conn_del_queue( connhead_t *connhead, connection_t *conn )
{
    if( NULL==connhead || NULL==conn )
        return CONN_ERR;

    queue_remove( &(conn->queue) );
    connhead->num--;
    return CONN_OK;
}


/** 
 * @brief 更新连接在队列中的位置
 *
 * 连接队列按连接发生读写事件的事件排序，以减少超时处理时的循环遍历。
 * 在连接发生读写事件时调用此函数
 * 
 * @param[in] connhead 连接队列头
 * @param[in] conn 连接
 * 
 * @return CONN_OK
 */
int conn_timeupdate( connhead_t *connhead, connection_t *conn )
{
    queue_t *q = &(conn->queue);
    queue_remove( q );
    queue_insert_tail( &(connhead->queue), q );
    return CONN_OK;
}


void conn_queue_destroy( connhead_t *connhead )
{
    if( NULL == connhead )
        return;
    connection_t *conn = NULL;
    queue_t *q = connhead->queue.next;
    while( q != &(connhead->queue) )
    {
        conn = queue_data( q, connection_t, queue );
        q = q->next;

        conn_del_queue( connhead, conn );

        if( NULL != conn->context )
            connection_req_free( conn );
        if( NULL != conn->ssl )
            del_ssl( conn );
        connection_del( conn );
    }
}
