#include "ftp_forward.h"
#include "readconf.h"
#include "log.h"
#include "conn_backend.h"
#include "conn_common.h"
#include "forward.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int ftp_forward( connection_t *c )
{
    int ret;
    connection_t *peer = c->peer;
    struct sockaddr_in addr;

    //TODO ftp parse & forward

    if( NULL == peer )
    {
        memset( &addr, 0, sizeof(struct sockaddr_in) );
        addr.sin_family = AF_INET;
        inet_aton( conf[BACKEND_SERVER_ADDR].value.str, &(addr.sin_addr) );
        addr.sin_port = htons( conf[FTP_BACKEND_PORT].value.num );

        peer = backend_conn_create( c, &addr );
        if( NULL == peer )
        {
            log_debug( "create backend ftp connection failed, fd:%d", c->fd );
            goto errpeer;
        }
        c->peer = peer;
    }
    else if( CONSTAT_WORKING!=peer->status || CONSTAT_CONNECTING!=peer->status )
    {
        goto errstat;
    }

    // forward ftp data directly
    ret = backend_handle_read( c );
    if( ret != 0 )
    {
        log_debug( "forward ftp data failed" );
        goto errfwd;
    }

    return FTP_FORWARD_OK;

errfwd:
errstat:
errpeer:
    return FTP_FORWARD_ERR;
}

int ftp_forward_http( connection_t *c )
{
    return http_handle_parseandforwarddata( c );
}
