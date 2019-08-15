#include "backend_common.h"
#include "readconf.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

int backend_set_host( connection_t *conn, const char *name, unsigned short len )
{
    if( NULL==conn || NULL==name )
        return -1;
    if( len==0 || len>=URL_NAME_MAXLEN )
    {
        log_debug( "wrong host len:%hu", len );
        return -1;
    }
    if( mode_frontend )
    {
        log_debug( "frontend server cannot set backend host" );
        return -1;
    }

    struct backend_host *bh = malloc( sizeof(struct backend_host) );
    if( NULL == bh )
    {
        log_debug( "malloc backend host struct failed" );
        return -1;
    }

    strncpy( bh->name, name, len );
    bh->name[len] = 0;
    bh->len = len;
    conn->context = bh;
    return 0;
}


void backend_del_host( connection_t *conn )
{
    if( NULL != conn && NULL != conn->context && !mode_frontend )
    {
        free( conn->context );
        conn->context = NULL;
    }
}
