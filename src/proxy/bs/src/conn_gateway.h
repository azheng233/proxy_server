#ifndef _CONN_GATEWAY_H_
#define _CONN_GATEWAY_H_

#include <netinet/in.h>
#include "connection.h"

int gateway_conn_create( int fd, struct sockaddr_in *addr );

int gateway_signal_close( connection_t *conn );

int gateway_ipcmp( unsigned char *ip );

#endif
