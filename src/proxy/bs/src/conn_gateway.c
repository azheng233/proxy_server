#include "conn_gateway.h"
#include "connection.h"
#include "conn_io.h"
#include "conn_common.h"
#include "fdevents.h"
#include "job.h"
#include "log.h"
#include "timehandle.h"
#include "chunk.h"
#include "head.h"
#include <string.h>
#include <stdlib.h>

connhead_t gw_connhead;


int gateway_handle_close( connection_t *conn )
{
    // del policy tree ?
    job_delconn( conn );
    conn_del_queue( &gw_connhead, conn );
    ev_delfd( conn->fd, conn );
    connection_del( conn );
    return CONN_OK;
}


int gateway_signal_close( connection_t *conn )
{
    set_conn_status( conn, CONSTAT_CLOSED );
    conn->peer = NULL;
    if( job_addconn( FDEVENT_CLOSE, conn ) != JOB_OK )
        return CONN_ERR;
    return CONN_OK;
}


int gateway_handle( int events, connection_t *conn )
{
    int ret = 0;
    if( NULL == conn )
        return CONN_ERR;

    if( FDEVENT_CLOSE & events )
    {
        log_debug( "gateway event close, fd:%d", conn->fd );
        ret = gateway_handle_close( conn );
        return CONN_OK;
    }

    if( FDEVENT_ERR & events )
    {
        log_debug( "gateway event err, fd:%d", conn->fd );
        goto err;
    }

    if( FDEVENT_HUP & events )
    {
        log_debug( "gateway event hup, fd:%d", conn->fd );
        goto err;
    }

    if( FDEVENT_IN & events )
    {
        log_debug( "gateway event in, fd:%d", conn->fd );
        conn->timeinfo.time_lastrec = curtime;
        conn_timeupdate( &gw_connhead, conn );

        ret = conn_handle_in( conn, gateway_rbuffer_read );
        if( CONN_OK != ret )
            goto err;
    }

    return CONN_OK;

err:
    ret = gateway_signal_close( conn );
    return CONN_ERR;
}


int gateway_conn_create( int fd, struct sockaddr_in *addr )
{
    int ret = setnonblocking( fd );
    if( ret < 0 )
    {
        log_error( "set gw socket %d nonblocking failed", fd );
        return CONN_ERR;
    }

    connection_t *conn = connection_new( fd, CONTYPE_CLIENT );
    if( NULL == conn )
    {
        log_error( "create new gateway connection %d failed", fd );
        return CONN_ERR;
    }

    conn->addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == conn->addr )
    {
        log_error( "malloc gateway addr failed" );
        goto errconn;
    }
    memcpy( conn->addr, addr, sizeof(struct sockaddr_in) );

    conn->peer = NULL;       //no policy connection
    conn->event_handle = gateway_handle;
    conn->status = CONSTAT_WORKING;

    if( ev_addfd( fd, FDEVENT_IN, conn ) != FDEVENT_OK )
    {
        log_error( "add gw connection %d to epoll failed", fd );
        goto errconn;
    }

    ret = conn_add_queue( &gw_connhead, conn );
    return CONN_OK;

errconn:
    connection_del( conn );
    return CONN_ERR;
}

int gateway_ipcmp( unsigned char *ip )
{
    connection_t *c = NULL;
    struct sockaddr_in *addr = NULL;
    queue_t *q = gw_connhead.queue.next;
    while( q != &(gw_connhead.queue) )
    {
        c = queue_data( q, connection_t, queue );
        q = q->next;

        addr = (struct sockaddr_in *)c->addr;
        if( memcmp( ip, &(addr->sin_addr.s_addr), 4 ) == 0 )
            return 0;
    }
    return -1;
}
