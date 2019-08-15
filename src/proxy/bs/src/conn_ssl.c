#include "conn_ssl.h"
#include "ssl_common.h"
#include "ssl.h"
#include "forward.h"
#include "conn_peer.h"
#include "log.h"
#include "fdevents.h"
#include "timehandle.h"
#include "conn_common.h"
#include "conn_io.h"

static void ssl_handle_close( connection_t *conn )
{
    //ssl_shutdown( conn->ssl );
    if (!ssl_use_client_cert) {
        del_ssl( conn );
    }
    connection_req_free( conn );
    conn_handle_thisclose( conn );
}

static int ssl_handle( int events, connection_t *conn )
{
    int ret = 0;
    if( NULL == conn )
        return CONN_ERR;
    connection *cs = conn->ssl;
    if (!ssl_use_client_cert) {
        if( NULL == cs )
            return CONN_ERR;
    }
  
    if( FDEVENT_PEERCLOSE & events )
    {
        log_debug( "ssl event peerclose, fd:%d", conn->fd );
        ret = conn_handle_peerclose( conn );
        if( CONN_AGN == ret )
            events |= FDEVENT_OUT;
        else
            events |= FDEVENT_CLOSE;
    }

    if( FDEVENT_CLOSE & events )
    {
        log_debug( "ssl event close, fd:%d", conn->fd );
        ssl_handle_close( conn );
        return CONN_OK;
    }

    if( FDEVENT_ERR & events )
    {
        log_debug( "ssl event hup, fd:%d", conn->fd );
        goto errclose;
    }

    if( FDEVENT_HUP & events )
    {
        log_debug( "ssl event hup, fd:%d", conn->fd );
        goto errclose;
    }

    if (!ssl_use_client_cert) {
        if( read_waiton_write == cs->wait )
        {
            log_debug( "ssl %d events 0x%x, read waiton write", conn->fd, events );
            if( events & FDEVENT_OUT )
            {
                cs->wait = none_wait;
                events &= ~FDEVENT_OUT;
                events |= FDEVENT_IN;
            }
            else return CONN_ERR;
        }
        else if( write_waiton_read == cs->wait )
        {
            log_debug( "ssl %d events 0x%x, write waiton read", conn->fd, events );
            if ( events & FDEVENT_IN )
            {
                cs->wait = none_wait;
                events &= ~FDEVENT_IN;
                events |= FDEVENT_OUT;
            }
            else return CONN_ERR;
        }
    }

    if( FDEVENT_IN & events )
    {
        log_debug( "ssl event in, fd:%d", conn->fd );
        conn->timeinfo.time_lastrec = curtime;
        conn_timeupdate( &connhead, conn );

        if( CONSTAT_HANDSHAKE == conn->status )
        {
            ret = ssl_handle_handshake( conn );
            if( CONN_OK == ret )
            {
                conn->status = CONSTAT_WORKING;
                return CONN_OK;
            }
            else if( CONN_AGN == ret )
                return CONN_OK;
            else goto err;
        }

        if (ssl_use_client_cert && CONSTAT_WORKING==conn->status) {
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
        log_debug( "ssl event out, fd:%d", conn->fd );
        conn->timeinfo.time_lastsend = curtime;
        conn_timeupdate( &connhead, conn );

        if( CONSTAT_HANDSHAKE == conn->status )
        {
            ret = ssl_handle_handshake( conn );
            if( CONN_OK == ret )
                conn->status = CONSTAT_WORKING;
            else if( CONN_AGN == ret )
                return CONN_OK;
            else goto err;
        }

        ret = conn_handle_out( conn );
        if( CONN_OK == ret )
        {
            if( CONSTAT_PEERCLOSED==conn->status || CONSTAT_CLOSING_WRITE==conn->status )
                goto errclose;
            else if( CONSTAT_HTTPCONNECT == conn->status ) {
                if (!ssl_use_client_cert) {
                    conn->status = CONSTAT_HANDSHAKE;       //发送完connect后握手
                } else {
                    conn->status = CONSTAT_WORKING;
                    waitconn_wake( conn, WAITEVENT_CANWRITE );
                }
            } else
                waitconn_wake( conn, WAITEVENT_CANWRITE );
        }
        else if( CONN_AGN != ret )
            goto err;
    }

    return CONN_OK;

err:
errclose:
    conn_signal_thisclose( conn );
    return CONN_ERR;
}


int ssl_conn_create( int fd, struct sockaddr_in *addr )
{
    int ret = setnonblocking( fd );
    if( ret < 0 )
    {
        log_debug( "set ssl socket %d nonblocking failed", fd );
        return CONN_ERR;
    }

    connection_t *conn = connection_new( fd, CONTYPE_SSL );
    if( NULL == conn )
    {
        log_debug( "new ssl connection failed, fd:%d", fd );
        return CONN_ERR;
    }

    conn->addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == conn->addr )
    {
        log_debug( "malloc ssl addr failed" );
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
    conn->event_handle = ssl_handle;
    conn->status = CONSTAT_HTTPCONNECT;

    // ssl
    if (!ssl_use_client_cert) {
        ret = init_ssl( conn );
        if( ret != 0 )
        {
            log_debug( "ssl init failed, fd:%d", fd );
            goto errconn;
        }
    }

    ret = connection_req_init( conn );
    if( 0 != ret )
        goto errssl;

    if( ev_addfd( fd, FDEVENT_IN, conn ) != FDEVENT_OK )
        goto errreq;

    conn_add_queue( &connhead, conn );
    return CONN_OK;

errreq:
    connection_req_free( conn );
errssl:
    del_ssl( conn );
errconn:
    connection_del( conn );
    return CONN_ERR;
}

