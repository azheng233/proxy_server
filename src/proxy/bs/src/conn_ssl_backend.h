#ifndef _CONN_SSL_BACKEND_H_
#define _CONN_SSL_BACKEND_H_

#include "connection.h"

connection_t *ssl_backend_conn_create( connection_t *clntconn, struct sockaddr_in *addr );

connection_t *ssl_backend_conn_create_noip( connection_t *clntconn, struct sockaddr_in *addr );

#endif
