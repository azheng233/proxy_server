#include "fd_rpc.h"
#include "msg_rpc.h"
#include "job.h"
#include "fdevents.h"
#include "timehandle.h"
#include "fdrw.h"
#include "conn_common.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

#define RPC_CONN_READBUF_SIZE 4096

connhead_t rpc_connhead;

static int rpc_handle_close( connection_t *conn )
{
    // del policy tree ?
    job_delconn( conn );
    conn_del_queue( &rpc_connhead, conn );
    ev_delfd( conn->fd, conn );
    rwbuf_free( conn );
    connection_del( conn );
    return CONN_OK;
}

int rpc_signal_close( connection_t *conn )
{
    set_conn_status( conn, CONSTAT_CLOSED );
    conn->peer = NULL;
    if( job_addconn( FDEVENT_CLOSE, conn ) != JOB_OK )
        return CONN_ERR;
    return CONN_OK;
}

static int rpc_handle_read( connection_t *c )
{
    int ret;

    ret = readfd_common(c);
    switch (ret) {
        case FDRW_MOR:
            ev_modfd(c->fd, c->lsnevents|FDEVENT_IN, c);
        case FDRW_OK:
            while ((ret = rpc_msg_reslove(c)) != FDRW_NOM)
            {
                if (ret!=FDRW_OK && ret!=FDRW_MOR)
                    goto err;

                ret = rpc_msg_parse(c);
                if (ret==FDRW_PUS)
                    goto errpause;
                if (ret!=0)
                    goto err;
            }
            break;
        case FDRW_AGN:
            ev_modfd(c->fd, c->lsnevents|FDEVENT_IN, c);
            break;
        case FDRW_CLS:
        default:
            goto err;
    }

    return FDRW_OK;
err:
    return FDRW_OTH;
errpause:
    return FDRW_OK;
}

static int rpc_conn_handle( int events, connection_t *c )
{
    int ret;

    if (!c) {
        log_warn("got null connection in rpc handle, fd %d", c->fd);
        return CONN_ERR;
    }

    if( FDEVENT_CLOSE & events ) {
        log_debug( "rpc event close, fd:%d", c->fd );
        ret = rpc_handle_close( c );
        return CONN_OK;
    }

    if (events & FDEVENT_ERR) {
        log_warn("rpc connection %d error", c->fd);
        goto errclose;
    }

    if (events & FDEVENT_HUP) {
        log_warn("rpc connection %d hup", c->fd);
        goto errclose;
    }

    if (events & FDEVENT_IN) {
        log_debug("rpc connection %d event in", c->fd);
        c->timeinfo.time_lastrec = curtime;
        conn_timeupdate( &rpc_connhead, c );

        ret = rpc_handle_read(c);
        if (ret != 0 ) {
            log_warn("rpc connection %d read return %d", c->fd, ret);
            goto errclose;
        }
    }

    if (events & FDEVENT_OUT) {
        log_debug("rpc connection %d event out", c->fd);
        c->timeinfo.time_lastsend = curtime;
        conn_timeupdate( &rpc_connhead, c );

        ret = writefd_handle_common(c);
        if (FDRW_OK!=ret && FDRW_AGN!=ret) {
            log_warn("rpc connection %d write return %d", c->fd, ret);
            goto errclose;
        }
    }

    return CONN_OK;

errclose:
    rpc_signal_close(c);
    return CONN_ERR;
}

int rpc_conn_init( int fd, struct sockaddr_in *addr )
{
    int ret = setnonblocking( fd );
    if( ret < 0 )
    {
        log_error( "set rpc socket %d nonblocking failed", fd );
        return CONN_ERR;
    }

    connection_t *conn = connection_new( fd, CONTYPE_RPC );
    if( NULL == conn )
    {
        log_error( "create new rpc connection %d failed", fd );
        return CONN_ERR;
    }

    conn->addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == conn->addr )
    {
        log_error( "malloc rpc addr failed" );
        goto errconn;
    }
    memcpy( conn->addr, addr, sizeof(struct sockaddr_in) );

    conn->peer = NULL;
    conn->event_handle = rpc_conn_handle;
    conn->status = CONSTAT_WORKING;

    if( ev_addfd( fd, FDEVENT_IN, conn ) != FDEVENT_OK )
    {
        log_error( "add rpc connection %d to epoll failed", fd );
        goto errconn;
    }

    if( rwbuf_init(conn, RPC_CONN_READBUF_SIZE) != 0 ) {
        log_error("alloc rpc conn's rw buf failed");
        goto errconn;
    }
    conn->rbuf.msgmax = RPC_CONN_READBUF_SIZE;

    ret = conn_add_queue( &rpc_connhead, conn );
    return CONN_OK;

errconn:
    connection_del( conn );
    return CONN_ERR;
}

