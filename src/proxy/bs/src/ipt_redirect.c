#include "ipt_redirect.h"
#include "get_port_from_url.h"
#include "get_interface_ip.h"
#include "iptc_mgr.h"
#include "readconf.h"
#include "log.h"
#include <stdlib.h>

#define IPT_PORTS_DEFAULT_COUNT  16
#define IPT_FTP_PORT  21
#define IPT_HTTP_PORT  80
#define IPT_HTTPS_PORT  443

static struct ipt_redirect {
    unsigned total;
    unsigned num;
    uint16_t *ports;
    iptc_mgr_t *iptc;
} iptr = { 0, 0, NULL, NULL };


int ipt_redirect_init()
{
    uint32_t ip;

    if( NULL==iptr.ports )
        iptr.ports = malloc( IPT_PORTS_DEFAULT_COUNT*sizeof(uint16_t) );
    if( NULL==iptr.ports )
        return -1;

    iptr.total = IPT_PORTS_DEFAULT_COUNT;
    iptr.num = 0;

    ip = get_interface_ip( conf[TRANSPARENT_INTERFACE].value.str );
    if( ip == -1 )
        goto err;

    iptr.iptc = iptc_mgr_init( conf[TRANSPARENT_INTERFACE].value.str, ip, conf[CLIENT_LISTEN_PORT].value.num );
    if( NULL==iptr.iptc )
    {
        log_error( "init iptc failed" );
        goto err;
    }

    ipt_redirect_add_port( IPT_HTTP_PORT );
    return 0;

err:
    ipt_redirect_clear();
    return -1;
}

void ipt_redirect_clear()
{
    int i;

    if( iptr.ports )
    {
        if( iptr.iptc )
        {
            for( i=0; i<iptr.num; i++ )
                iptc_entry_del( iptr.iptc, iptr.ports[i] );
            iptc_mgr_destroy( iptr.iptc );
        }
        free( iptr.ports );
    }

    iptr.total = 0;
    iptr.num = 0;
    iptr.ports = NULL;
}


static inline int ipt_redirect_search( uint16_t port )
{
    int l, m, h;

    if( NULL==iptr.ports || iptr.num<=0 )
        return -1;

    l = 0;
    h = iptr.num-1;

    while( l<=h )
    {
        m = l + (h-l)/2;
        if( port == iptr.ports[m] )
            return m;
        if( port < iptr.ports[m] )
            h = m-1;
        else
            l = m+1;
    }

    return -1;
}

int ipt_redirect_add_port( uint16_t port )
{
    uint16_t *p;
    int i;

    if( ipt_redirect_search( port ) >= 0 )
        return 0;

    if( iptr.num >= iptr.total )
    {
        p = realloc( iptr.ports, (iptr.total+IPT_PORTS_DEFAULT_COUNT) * sizeof(uint16_t) );
        if( NULL==p )
            return -1;
        iptr.ports = p;
        iptr.total += IPT_PORTS_DEFAULT_COUNT;
    }

    i = iptc_entry_add( iptr.iptc, port );
    if( i != 0 )
    {
        log_error( "add redirect rule for port %hu failed", port );
        return -1;
    }

    for( i=iptr.num-1; i>=0; i-- )
    {
        if( port > iptr.ports[i] )
        {
            iptr.ports[i+1] = port;
            break;
        }
        iptr.ports[i+1] = iptr.ports[i];
    }
    if( i<0 )
        iptr.ports[0] = port;

    iptr.num++;
    return 0;
}

int ipt_redirect_del_port( uint16_t port )
{
    int ret;
    int i;

    ret = ipt_redirect_search( port );
    if( ret < 0 )
        return 0;

    ret = iptc_entry_del( iptr.iptc, port );
    if( ret != 0 )
    {
        log_error( "del redirect rule for port %hu failed", port );
        return -1;
    }

    for( i=ret; i<iptr.num; i++ )
        iptr.ports[i] = iptr.ports[i+1];

    iptr.num--;
    return 0;
}


int ipt_redirect_add_url( char *url, size_t len )
{
    uint16_t port;

    port = get_port_from_url( url, len );
    if( 0==port )
        return -1;

    return ipt_redirect_add_port( port );
}

int ipt_redirect_del_url( char *url, size_t len )
{
    uint16_t port;

    port = get_port_from_url( url, len );
    if( 0==port )
        return -1;

    return ipt_redirect_del_port( port );
}
