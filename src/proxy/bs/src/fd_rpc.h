#ifndef _FD_RPC_H_
#define _FD_RPC_H_

#include <arpa/inet.h>
#include "connection.h"

int rpc_conn_init( int fd, struct sockaddr_in *addr );
int rpc_signal_close( connection_t *conn );

#endif
