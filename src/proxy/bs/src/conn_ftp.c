#include "conn_ftp.h"
#include "conn_common.h"
#include "conn_io.h"
#include "conn_peer.h"
#include "fdevents.h"
#include "log.h"
#include "timehandle.h"
#include "ftp_forward.h"
#include "forward.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int ftp_handle( int events, connection_t *c )
{
    int ret;

    if( NULL == c )
        return CONN_ERR;

    if( FDEVENT_PEERCLOSE & events )
    {
        log_debug( "ftp event peerclose, fd:%d", c->fd );
        ret = conn_handle_peerclose( c );
        if( CONN_AGN == ret )
            events |= FDEVENT_OUT;
        else
            events |= FDEVENT_CLOSE;
    }

    if( FDEVENT_CLOSE & events )
    {
        log_debug( "ftp event close, fd:%d", c->fd );
        conn_handle_thisclose( c );
        return CONN_OK;
    }

    if( FDEVENT_ERR & events )
    {
        log_debug( "ftp event hup, fd:%d", c->fd );
        goto err;
    }

    if( FDEVENT_HUP & events )
    {
        log_debug( "ftp event hup, fd:%d", c->fd );
        goto err;
    }

    if( FDEVENT_IN & events )
    {
        log_debug( "ftp event in, fd:%d", c->fd );
        c->timeinfo.time_lastrec = curtime;
        conn_timeupdate( &connhead, c );

        ret = conn_handle_in( c, ftp_forward_http );
        if( CONN_OK != ret )
        {
            if( CONSTAT_CLOSING_WRITE==c->status )
                events |= FDEVENT_OUT;
            else
                goto err;
        }
    }

    if( FDEVENT_OUT & events )
    {
        log_trace( "ftp event out, fd:%d", c->fd );
        c->timeinfo.time_lastsend = curtime;
        conn_timeupdate( &connhead, c );

        ret = conn_handle_out( c );
        if( CONN_OK == ret )
        {
            if( CONSTAT_PEERCLOSED==c->status || CONSTAT_CLOSING_WRITE==c->status )
                goto errclose;
            else
                waitconn_wake( c, WAITEVENT_CANWRITE );
        }
        else if( CONN_AGN != ret )
            return CONN_ERR;
    }

    return CONN_OK;
err:
errclose:
    conn_signal_thisclose( c );
    return CONN_ERR;
}

int ftp_conn_create( int fd, struct sockaddr_in *addr )
{
    int ret;
    connection_t *c;

    ret = setnonblocking( fd );
    if( ret != 0 )
    {
        log_debug( "set ftp socket %d nonblocking failed", fd );
        goto errfd;
    }

    c = connection_new( fd, CONTYPE_FTP );
    if( NULL == c )
    {
        log_debug( "create new ftp connection failed" );
        goto errfd;
    }

    c->addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == c->addr )
    {
        log_debug( "malloc ftp connection addr failed" );
        goto errconn;
    }
    memcpy( c->addr, addr, sizeof(struct sockaddr_in) );

    c->local_addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == c->local_addr )
    {
        free(c->addr);
        log_error( "malloc ftp local_addr failed" );
        goto errconn;
    }
    socklen_t addrlen = sizeof(struct sockaddr_in);
    getsockname( fd, c->local_addr, &addrlen );

    c->peer = NULL;       //no backend connection
    c->event_handle = ftp_handle;
    c->status = CONSTAT_WORKING;

    ret = connection_req_init( c );
    if( 0 != ret )
        goto errconn;

    if( ev_addfd( fd, FDEVENT_IN, c ) != FDEVENT_OK )
        goto errreq;

    conn_add_queue( &connhead, c );
    return CONN_OK;

errreq:
errconn:
    connection_del( c );
    return CONN_ERR;
errfd:
    close( fd );
    return CONN_ERR;
}

