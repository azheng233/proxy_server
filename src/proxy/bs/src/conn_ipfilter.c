#include "conn_ipfilter.h"
#include "connection.h"
#include "log.h"
#include "conn_common.h"
#include "job.h"
#include "fdevents.h"
#include "timehandle.h"
#include "conn_io.h"
#include "readconf.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

connection_t *ipf_conn = NULL;

int ipfilter_handle( int events, connection_t *conn )
{
    int ret = 0;
    if( NULL==conn || conn != ipf_conn )
    {
        log_debug( "wrong ipfilter connection" );
        return CONN_ERR;
    }

    if( FDEVENT_CLOSE & events )
    {
        log_debug( "ipfilter event close, fd:%d", conn->fd );
        ipfilter_conn_close();
        return CONN_OK;
    }

    if( FDEVENT_ERR & events )
    {
        log_debug( "ipfilter event err, fd:%d", conn->fd );
        goto errclose;
    }

    if( FDEVENT_HUP & events )
    {
        log_debug( "ipfilter event hup, fd:%d", conn->fd );
        goto errclose;
    }

    if( FDEVENT_IN & events )
    {
        log_debug( "ipfilter event in, fd:%d", conn->fd );
        conn->timeinfo.time_lastrec = curtime;

        unsigned char tmp[4] = {0};
        ret = read( conn->fd, tmp, 4 );
        log_debug( "ipfilter read %d bytes, %02x %02x %02x %02x", ret, \
                tmp[0], tmp[1], tmp[2], tmp[3] );
        // ipfilter should not read
        goto err;
    }

    if( FDEVENT_OUT & events )
    {
        log_debug( "ipfilter event out, fd:%d", conn->fd );
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

int ipfilter_conn_create()
{
    int fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( fd < 0 )
    {
        log_debug( "create ipfilter socket failed, %s", strerror(errno) );
        return CONN_ERR;
    }

    int ret = setnonblocking( fd );
    if( ret != 0 )
    {
        log_debug( "set ipfilter socket %d nonblocking failed", fd );
        goto errfd;
    }

    ipf_conn = connection_new( fd, CONTYPE_IPFILTER );
    if( NULL == ipf_conn )
    {
        log_debug( "create new ipfilter connection failed, fd:%d", fd );
        goto errfd;
    }

    ipf_conn->event_handle = ipfilter_handle;

    struct sockaddr_in *addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == addr )
    {
        log_debug( "malloc ipfilter addr failed" );
        goto errconn;
    }
    ipf_conn->addr = (struct sockaddr *)addr;

    memset( addr, 0, sizeof(struct sockaddr_in) );
    addr->sin_family = AF_INET;
    inet_aton( IPFILTER_SERVER_ADDR, &(addr->sin_addr) );
    addr->sin_port = htons( conf[IPFILTER_PORT].value.num );

    ret = conn_connect( ipf_conn );
    if( ret != 0 )
    {
        log_debug( "ipfilter %d connect failed", fd );
        goto errconn;
    }

    return CONN_OK;

errconn:
    connection_del( ipf_conn );
    ipf_conn = NULL;
errfd:
    close( fd );
    return CONN_ERR;
}


void ipfilter_conn_close()
{
    if( NULL != ipf_conn )
    {
        connection_del( ipf_conn );
        ipf_conn = NULL;
        log_debug( "ipfilter connection closed" );
    }
}
