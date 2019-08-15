#include "conn_ssl_backend.h"
#include "conn_common.h"
#include "ssl_common.h"
#include "ssl.h"
#include "conn_peer.h"
#include "conn_io.h"
#include "fdevents.h"
#include "timehandle.h"
#include "log.h"
#include "errpage.h"
#include "chunk.h"
#include "job.h"
#include "monitor_log.h"
#include "forward.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

void ssl_backend_handle_close( connection_t *conn )
{
    //ssl_shutdown( conn->ssl );
    if (!ssl_use_client_cert) {
        del_ssl( conn );
    }
    conn_handle_thisclose( conn );
}

int ssl_backend_handle( int events, connection_t *conn )
{
    int ret = 0;
    if( NULL == conn )
        return CONN_ERR;
    connection_t *peer = conn->peer;
    connection *cs = conn->ssl;
    if (!ssl_use_client_cert) {
        if( NULL == cs )
            return CONN_ERR;
    }
  
    if( FDEVENT_PEERCLOSE & events )
    {
        log_debug( "ssl backend event peerclose, fd:%d", conn->fd );
        ret = conn_handle_peerclose( conn );
        if( CONN_AGN == ret )
            events |= FDEVENT_OUT;
        else
            events |= FDEVENT_CLOSE;
    }

    if( FDEVENT_CLOSE & events )
    {
        log_debug( "ssl backend event close, fd:%d", conn->fd );
        ssl_backend_handle_close( conn );
        return CONN_OK;
    }

    if( FDEVENT_ERR & events )
    {
        log_debug( "ssl backend event err, fd:%d", conn->fd );
        if( CONSTAT_CONNECTING == conn->status )
        {
            if( NULL != peer )
            {
                //putmsg_err( peer->writebuf, HTTP_504_GW_TIMEOUT, "connection error", inet_ntoa( ((struct sockaddr_in *)peer->local_addr)->sin_addr ) );
                mlog_log_appstat( get_req_url(conn->peer, NULL), APP_STAT_DOWN );
            }
        }
        goto errclose;
    }

    if( FDEVENT_HUP & events )
    {
        log_debug( "ssl backend event hup, fd:%d", conn->fd );
        //SSL_set_quiet_shutdown( ((connection *)(conn->ssl))->ssl_con->ssl, 1 );
        goto errclose;
    }

    if (!ssl_use_client_cert) {
        if( read_waiton_write == cs->wait )
        {
            log_debug( "ssl backend %d events 0x%x, read waiton write", conn->fd, events );
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
            log_debug( "ssl backend %d events 0x%x, write waiton read", conn->fd, events );
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
        log_debug( "ssl backend event in, fd:%d", conn->fd );
        conn->timeinfo.time_lastrec = curtime;
        conn_timeupdate( &connhead, conn );

        if( CONSTAT_HANDSHAKE == conn->status )
        {
            ret = ssl_handle_handshake( conn );
            if( CONN_OK == ret )
            {
                conn->status = CONSTAT_WORKING;
                if( ! chunk_isempty( conn->writebuf) )
                    ret = job_addconn( FDEVENT_OUT, conn );
                return CONN_OK;
            }
            else if( CONN_AGN == ret )
                return CONN_OK;
            else goto err;
        }

        ret = conn_handle_in( conn, backend_handle_read );
        if( CONN_OK != ret )
            goto err;
    }

    if( FDEVENT_OUT & events )
    {
        log_debug( "ssl backend event out, fd:%d", conn->fd );
        conn->timeinfo.time_lastsend = curtime;
        conn_timeupdate( &connhead, conn );

        if( CONSTAT_CONNECTING == conn->status )
        {
            if (ssl_use_client_cert) {
                conn->status = CONSTAT_WORKING;
            } else {
                conn->status = CONSTAT_HANDSHAKE;
            }
            ret = ev_modfd( conn->fd, FDEVENT_IN, conn );

            // connect ok msg
            if( NULL != peer )
            {
                ret = putmsg_connect_ok( peer->writebuf );
                ret = job_addconn( FDEVENT_OUT, peer );
                mlog_log_appstat( get_prev_url(conn->peer), APP_STAT_DOWN );
            }
        }

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
            else
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

connection_t *ssl_backend_conn_create( connection_t *clntconn, struct sockaddr_in *addr )
{
    connection_t *conn = ssl_backend_conn_create_noip( clntconn, addr );
    if( NULL == conn )
        return NULL;

    int ret = backend_conn_connect( conn, addr->sin_addr.s_addr );
    if( ret != CONN_OK )
        goto errssl;

    if (!ssl_use_client_cert) {
        if (conn->status == CONSTAT_WORKING)
            conn->status = CONSTAT_HANDSHAKE;
    }

    return conn;

errssl:
    conn_del_queue( &connhead, conn );
    del_ssl( conn );
    connection_del( conn );
    return NULL;
}


connection_t *ssl_backend_conn_create_noip( connection_t *clntconn, struct sockaddr_in *addr )
{
    int fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( fd < 0 )
    {
        log_debug( "create socket to ssl backend failed, %s\n", strerror(errno) );
        return NULL;
    }
    
    int ret = setnonblocking( fd );
    if( ret != 0 )
    {
        log_debug( "set ssl backend socket %d nonblocking failed", fd );
        goto errfd;
    }

    connection_t *conn = connection_new( fd, CONTYPE_SSL_BACKEND );
    if( NULL == conn )
        goto errfd;

    conn->peer = clntconn;
    conn->event_handle = ssl_backend_handle;
    conn->status = CONSTAT_INIT;

    // ssl
    if (!ssl_use_client_cert) {
        ret = init_ssl( conn );
        if( ret != 0 )
        {
            log_debug( "ssl init failed, fd:%d", fd );
            goto errconn;
        }
    }

    conn->addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == conn->addr )
    {
        log_debug( "malloc ssl backend addr failed" );
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

