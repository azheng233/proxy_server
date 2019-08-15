#ifndef _SSL_CONN_H_
#define _SSL_CONN_H_

#include <netinet/in.h>
#include "connection.h"

int ssl_conn_create( int fd, struct sockaddr_in *addr );

#endif
