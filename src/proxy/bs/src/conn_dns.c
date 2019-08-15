#include "conn_dns.h"
#include "connection.h"
#include "conn_common.h"
#include "log.h"
#include "fdevents.h"
#include "timehandle.h"
#include "conn_io.h"
#include "dns_parse.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

connection_t *dns_conn = NULL;

int dns_handler( int events, connection_t *conn )
{
    int ret = 0;
    if( NULL==conn || conn != dns_conn )
    {
        log_debug( "wrong dns connection" );
        return CONN_ERR;
    }

    if( FDEVENT_CLOSE & events )
    {
        log_debug( "dns event close, fd:%d", conn->fd );
        dns_conn_close();
        return CONN_OK;
    }

    if( FDEVENT_ERR & events )
    {
        log_debug( "dns event err, fd:%d", conn->fd );
        goto errclose;
    }

    if( FDEVENT_HUP & events )
    {
        log_debug( "dns event hup, fd:%d", conn->fd );
        goto errclose;
    }

    if( FDEVENT_IN & events )
    {
        log_debug( "dns event in, fd:%d", conn->fd );
        conn->timeinfo.time_lastrec = curtime;

        ret = conn_handle_in( conn, dns_parse_echo );
        if( CONN_OK != ret )
            goto err;
    }

    if( FDEVENT_OUT & events )
    {
        log_debug( "dns event out, fd:%d", conn->fd );
        conn->timeinfo.time_lastsend = curtime;

        if( conn->status == CONSTAT_CONNECTING )
        {
            conn->status = CONSTAT_WORKING;
            ret = ev_modfd( conn->fd, FDEVENT_IN, conn );
        }
        if( conn->status == CONSTAT_WORKING )
        {
            ret = conn_handle_out( conn );
            if( CONN_OK != ret && CONN_AGN != ret )
                return CONN_ERR;
        }
        else goto err;
    }

    return CONN_OK;

err:
errclose:
    ret = conn_signal_close( conn );
    return CONN_ERR;
}


int dns_conn_create()
{
    int fd = socket( PF_UNIX, SOCK_STREAM, 0 );
    if( -1 == fd )
    {
        log_debug( "create dns socket failed, %s", strerror(errno) );
        return CONN_ERR;
    }

    int ret = setnonblocking_notsockfd( fd );
    if( ret != 0 )
    {
        log_debug( "set dns socket %d nonblocking failed", fd );
        goto errfd;
    }

    dns_conn = connection_new( fd, CONTYPE_DNS );
    if( NULL == dns_conn )
    {
        log_debug( "create new dns connection failed, fd:%d", fd );
        goto errfd;
    }

    dns_conn->event_handle = dns_handler;

    struct sockaddr_un *addr = malloc( sizeof(struct sockaddr_un) );
    if( NULL == addr )
    {
        log_debug( "malloc dns addr failed" );
        goto errconn;
    }

    memset( addr, 0, sizeof(struct sockaddr_un) );
    addr->sun_family = AF_UNIX;
    strncpy( addr->sun_path, DNS_SOCK_PATH, 108 );
    dns_conn->addr = (struct sockaddr *)addr;

    ret = conn_connect( dns_conn );
    if( ret != 0 )
    {
        log_debug( "dns %d connect failed", fd );
        goto errconn;
    }

    return CONN_OK;

errconn:
    connection_del( dns_conn );
    dns_conn = NULL;
errfd:
    close( fd );
    return CONN_ERR;
}


void dns_conn_close()
{
    if( NULL != dns_conn )
    {
        connection_del( dns_conn );
        dns_conn = NULL;
        log_debug( "dns connection closed" );
    }
}
