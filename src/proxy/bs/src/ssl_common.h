#ifndef _SSL_COMMON_H_
#define _SSL_COMMON_H_

#include "connection.h"
#include <sys/uio.h>

struct certificate {
    unsigned char *cert;
    unsigned int len;
};

#define CA_DEPTH  1

int init_ssl( connection_t *conn );
void del_ssl( connection_t *conn );

int ssl_handle_handshake( connection_t *conn );

int ssl_writev( connection_t *conn, const struct iovec *iov, int iovcnt );

int ssl_toread( connection_t *conn );

int loadcert( const char *file, struct certificate *cert );
int loadallcert();

#endif
