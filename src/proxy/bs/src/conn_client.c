#include "conn_client.h"
#include "connection.h"
#include "conn_peer.h"
#include "fdevents.h"
#include "conn_io.h"
#include "conn_backend.h"
#include "conn_common.h"
#include "job.h"
#include "timehandle.h"
#include "log.h"
#include "forward.h"
#include "policy_cache.h"
#include "msg_audit.h"
#include "hexstr.h"
#include "monitor_log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "chunk.h"
#define FORWARD_HOST  "192.168.6.19"
int client_handle_read( connection_t *conn )
{
    connection_t *peer = conn->peer;
    if( NULL == peer )
    {
        struct sockaddr_in addr;
        memset( &addr, 0, sizeof(struct sockaddr_in) );
        addr.sin_family = AF_INET;
        inet_aton( FORWARD_HOST, &(addr.sin_addr) );
        addr.sin_port = htons( 80 );
        log_debug( "client connection %d connect to backend", conn->fd );
        peer = backend_conn_create( conn, &addr );
        if( NULL == peer )
        {
            log_debug( "client %d connect to backend failed", conn->fd );
            return -1;
        }
        conn->peer = peer;
    }
    else
    {
        if( peer->status != CONSTAT_WORKING && peer->status != CONSTAT_CONNECTING )
        {
            log_debug( "backend %d stat %d", peer->fd, peer->status );
            return -1;
        }
    }
    log_debug( "client %d ==> backend %d", conn->fd, peer->fd );

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

    if( peer->status == CONSTAT_WORKING )
        job_addconn( FDEVENT_OUT, peer );

    return 0;
}

static void client_send_flow_log(connection_t *c)
{
    int ret;
    struct policy_node *pnode;

    if (c->upsize != 0 || c->downsize != 0)
    {
        pnode = policy_node_find_from_rbtree_byconn(c);
        if (pnode && !pnode->isforbidden)
        {
            if (c->action_prev) {
                mlog_user_access_done(pnode->snstr, get_prev_url(c), c->mlog_up_flow, c->mlog_down_flow);
            }

            ret = audit_log_flow(pnode->sn, pnode->snlen, c->upsize, c->downsize);
            if (ret) {
                log_debug("connection %d send flow log failed", c->fd);
            } else {
                log_debug("connection %d send flow log, up %u down %u", c->fd, c->upsize, c->downsize);
                c->upsize = 0;
                c->downsize = 0;
            }
        }
    }
}

int client_handle( int events, connection_t *conn )
{
    int ret = 0;
    connection_t *peer;

    if( NULL == conn )
        return CONN_ERR;
    peer = conn->peer;

    if( FDEVENT_PEERCLOSE & events )
    {
        log_debug( "client event peerclose, fd:%d", conn->fd );
        ret = conn_handle_peerclose( conn );
        if( CONN_AGN == ret )
            events |= FDEVENT_OUT;
        else
            events |= FDEVENT_CLOSE;
    }

    if( FDEVENT_CLOSE & events )
    {
        log_debug( "client event close, fd:%d", conn->fd );
        if( mode_frontend )
            client_send_flow_log( conn );
        connection_req_free( conn );
        conn_handle_thisclose( conn );
        return CONN_OK;
    }

    if( FDEVENT_ERR & events )
    {
        log_debug( "client event err, fd:%d", conn->fd );
        goto err;
    }

    if( FDEVENT_HUP & events )
    {
        log_debug( "client event hup, fd:%d", conn->fd );
        // read ?
        goto err;
    }

    if( FDEVENT_IN & events )
    {
        log_debug( "client event in, fd:%d", conn->fd );
        conn->timeinfo.time_lastrec = curtime;
        conn_timeupdate( &connhead, conn );

        if (ssl_use_client_cert && conn->peer && CONTYPE_SSL_BACKEND==peer->type && CONSTAT_WORKING==peer->status) {
            ret = conn_handle_in( conn, backend_handle_read );
        } else {
            ret = conn_handle_in( conn, http_handle_parseandforwarddata );
        }
        if( CONN_OK != ret )
        {
            if( CONSTAT_CLOSING_WRITE==conn->status )
                events |= FDEVENT_OUT;
            else
                goto err;
        }
    }

    if( FDEVENT_OUT & events )
    {
        log_debug( "client event out, fd:%d", conn->fd );
        conn->timeinfo.time_lastsend = curtime;
        conn_timeupdate( &connhead, conn );

        ret = conn_handle_out( conn );
        if( CONN_OK == ret )
        {
            if( CONSTAT_PEERCLOSED==conn->status || CONSTAT_CLOSING_WRITE==conn->status )
                goto errclose;
            else
                waitconn_wake( conn, WAITEVENT_CANWRITE );
        }
        else if( CONN_AGN != ret )
            return CONN_ERR;
    }

    return CONN_OK;

err:
errclose:
    conn_signal_thisclose( conn );
    return CONN_ERR;
}


/** 
 * @brief 创建客户端连接
 * 
 * @param[in] fd 客户端连接socket
 * @param[in] addr 客户端地址
 * 
 * @retval CONN_ERR 失败 
 * @retval CONN_OK 成功
 */
int client_conn_create( int fd, struct sockaddr_in *addr )
{
    int ret = setnonblocking( fd );
    if( ret < 0 )
    {
        log_error( "set client socket %d nonblocking failed", fd );
        return CONN_ERR;
    }

    connection_t *conn = connection_new( fd, CONTYPE_CLIENT );
    if( NULL == conn )
        return CONN_ERR;

    conn->addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == conn->addr )
    {
        log_error( "malloc client addr failed" );
        goto errconn;
    }
    memcpy( conn->addr, addr, sizeof(struct sockaddr_in) );

    conn->local_addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == conn->local_addr )
    {
        free(conn->addr);
        log_error( "malloc client local_addr failed" );
        goto errconn;
    }
    socklen_t addrlen = sizeof(struct sockaddr_in);
    getsockname( fd, conn->local_addr, &addrlen );

    conn->peer = NULL;       //no backend connection
    conn->event_handle = client_handle;
    conn->status = CONSTAT_WORKING;

    ret = connection_req_init( conn );
    if( 0 != ret )
        goto errconn;

    if( ev_addfd( fd, FDEVENT_IN, conn ) != FDEVENT_OK )
        goto errreq;

    conn_add_queue( &connhead, conn );
    return CONN_OK;

errreq:
    connection_req_free( conn );
errconn:
    connection_del( conn );
    return CONN_ERR;
}

