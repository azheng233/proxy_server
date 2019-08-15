#include "conn_backend.h"
#include "conn_common.h"
#include "conn_peer.h"
#include "conn_io.h"
#include "fdevents.h"
#include "chunk.h"
#include "job.h"
#include "timehandle.h"
#include "log.h"
#include "monitor_log.h"
#include "forward.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


int backend_handle( int events, connection_t *conn )
{
    int ret = 0;
    if( NULL == conn )
        return CONN_ERR;
  
    if( FDEVENT_PEERCLOSE & events )
    {
        log_debug( "backend event peerclose, fd:%d", conn->fd );
        ret = conn_handle_peerclose( conn );
        if( CONN_AGN == ret )
            events |= FDEVENT_OUT;
        else
            events |= FDEVENT_CLOSE;
    }

    if( FDEVENT_CLOSE & events )
    {
        log_debug( "backend event close, fd:%d", conn->fd );
        conn_handle_thisclose( conn );
        return CONN_OK;
    }

    if( FDEVENT_ERR & events )
    {
        if( !mode_frontend && conn->status==CONSTAT_CONNECTING )
            mlog_log_appstat( get_prev_url(conn->peer), APP_STAT_DOWN );
        log_debug( "backend event err, fd:%d", conn->fd );
        goto errclose;
    }

    if( FDEVENT_HUP & events )
    {
        log_debug( "backend event hup, fd:%d", conn->fd );
        // read ?
        goto errclose;
    }

    if( FDEVENT_IN & events )
    {
        log_debug( "backend event in, fd:%d", conn->fd );
        conn->timeinfo.time_lastrec = curtime;
        conn_timeupdate( &connhead, conn );

        ret = conn_handle_in( conn, backend_handle_read );
        if( CONN_OK != ret )
            goto err;
    }

    if( FDEVENT_OUT & events )
    {
        log_debug( "backend event out, fd:%d", conn->fd );
        conn->timeinfo.time_lastsend = curtime;
        conn_timeupdate( &connhead, conn );

        if( conn->status == CONSTAT_CONNECTING )
        {
            if( !mode_frontend )
                mlog_log_appstat( get_prev_url(conn->peer), APP_STAT_LIVE );
            conn->status = CONSTAT_WORKING;
            ret = ev_modfd( conn->fd, FDEVENT_IN, conn );
        }

        if( conn->status == CONSTAT_WORKING )
        {
            ret = conn_handle_out( conn );
            if( CONN_OK == ret )
            {
                ret = waitconn_wake( conn, WAITEVENT_CANWRITE );
            }
            else if( CONN_AGN != ret )
                return CONN_ERR;
        }
        else goto err;
    }

    return CONN_OK;

err:
errclose:
    conn_signal_thisclose( conn );
    return CONN_ERR;
}


/** 
 * @brief 创建与后端服务的连接
 *
 * 创建连接结构，连接后端服务，与客户端连接组成对等关系
 * 
 * @param[in] clntconn 客户端连接
 * @param[in] addr 连接地址
 * 
 * @return 与后端连接
 * @retval NULL 失败
 */
connection_t *backend_conn_create( connection_t *clntconn, struct sockaddr_in *addr )
{
    connection_t *conn = backend_conn_create_noip( clntconn, addr );
    if( NULL == conn )
        return NULL;

    int ret = backend_conn_connect( conn, addr->sin_addr.s_addr );
    if( ret != CONN_OK )
        goto errconn;

    return conn;

errconn:
    conn_del_queue( &connhead, conn );
    connection_del( conn );
    return NULL;
}


connection_t *backend_conn_create_noip( connection_t *clntconn, struct sockaddr_in *addr )
{
    int fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( fd < 0 )
    {
        log_error( "create socket to backend failed, %s\n", strerror(errno) );
        return NULL;
    }
    
    int ret = setnonblocking( fd );
    if( ret != 0 )
    {
        log_error( "set backend socket %d nonblocking failed", fd );
        goto errfd;
    }

    connection_t *conn = connection_new( fd, CONTYPE_BACKEND );
    if( NULL == conn )
        goto errfd;

    conn->peer = clntconn;
    conn->event_handle = backend_handle;
    conn->status = CONSTAT_INIT;

    conn->addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == conn->addr )
    {
        log_error( "malloc backend addr failed" );
        goto errconn;
    }
    memcpy( conn->addr, addr, sizeof(struct sockaddr_in) );

    ret = conn_add_queue( &connhead, conn );
    return conn;

errconn:
    connection_del( conn );
errfd:
    close( fd );
    return NULL;
}

