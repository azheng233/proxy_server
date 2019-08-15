#include "fd_rpc_listen.h"
#include "connection.h"
#include "fd_rpc.h"
#include "fdevents.h"
#include "conn_common.h"
#include "log.h"
#include "readconf.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define RPC_ONCE_ACCEPT_NUM 10

connection_t *rpclsn_conn;
extern connhead_t rpc_connhead;

static int rpc_listen_handle( int events, connection_t *conn )
{
    struct sockaddr_in caddr;
    socklen_t socklen;
    int cfd;
    int i;

    if (events & FDEVENT_ERR) {
        log_warn("rpc listen connection got error event");
        return CONN_ERR;
    }

    if (events & FDEVENT_IN) {
        for (i=0; i<RPC_ONCE_ACCEPT_NUM; i++) {
            socklen = sizeof(caddr);
            cfd = accept(conn->fd, (struct sockaddr*)&caddr, &socklen);
            if (cfd < 0) {
                switch (errno) {
                case EAGAIN:
#if EWOULDBLOCK != EAGAIN
                case EWOULDBLOCK:
#endif
                case EINTR:
                    log_debug("rpc listen accept interrupted, %m");
                    goto out;
                case ECONNABORTED: 	
                case EMFILE:
                default:
                    log_warn("rpc listen accept error, %m");
                    break;
                }
            } else {
                rpc_conn_init(cfd, &caddr);
            }
        }
    }

out:
    return 0;
}

int rpc_listen_create()
{
    struct sockaddr_in *addr;
    int fd;
    int ret;

    rpclsn_conn = NULL;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd <= 0) {
        log_error("create rpc listen socket failed, %m");
        return -1;
    }

    ret = setsockreuse( fd );
    if( ret < 0 ){
        log_error( "set rpc listen socket %d reuse failed", fd );
        goto errfd;
    }
    ret = setnonblocking( fd );
    if( ret < 0 ){
        log_error( "set rpc listen socket %d nonblocking failed", fd );
        goto errfd;
    }

    rpclsn_conn = connection_new( fd, CONTYPE_RPC_LSN );
    if( NULL == rpclsn_conn ) {
        log_error( "create new connection for rpc listen failed" );
        goto errfd;
    }

    addr = malloc( sizeof(struct sockaddr_in) );
    if( NULL == addr )
    {
        log_error( "malloc rpc listen addr failed" );
        goto errconn;
    }

    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(conf[RPC_LISTEN_PORT].value.num);
    inet_aton(conf[RPC_LISTEN_ADDR].value.str, &(addr->sin_addr));
    rpclsn_conn->addr = (struct sockaddr *)addr;

    ret = conn_listen( rpclsn_conn, RPC_ONCE_ACCEPT_NUM );
    if( -1 == ret )
    {
        log_error( "rpc listen connection failed to listen" );
        goto errconn;
    }

    rpclsn_conn->event_handle = rpc_listen_handle;
    rpclsn_conn->status = CONSTAT_WORKING;

    ret = ev_addfd( fd, FDEVENT_IN, rpclsn_conn );
    if( FDEVENT_ERROR == ret )
    {
        log_error( "add rpc listen connection %d to epoll failed", rpclsn_conn->fd );
        goto errconn;
    }
    log_debug( "rpc listen connection %d created", rpclsn_conn->fd );

    conn_queue_init( &rpc_connhead );
    return CONN_OK;

errconn:
    connection_del( rpclsn_conn );
    rpclsn_conn = NULL;
errfd:
    close( fd );
    return CONN_ERR;
}

void rpc_listen_close()
{
    if( NULL == rpclsn_conn )
        return;
    log_debug( "rpc listen connection %d closed", rpclsn_conn->fd );
    connection_del( rpclsn_conn );
    rpclsn_conn = NULL;
}

void rpc_connections_destroy()
{
    conn_queue_destroy( &rpc_connhead );
}

