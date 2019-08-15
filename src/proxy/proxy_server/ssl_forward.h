#ifndef __SSL_FORWARD_H_
#define __SSL_FORWARD_H_

#include "https.h"
#include "sort_request.h"
#include "recvline.h"
#include "ssl_handle.h"

struct link;
typedef struct request_start start;
typedef struct hash_table table;

int SSL_forward(SSL *server_ssl, SSL *client_ssl, start *line1, struct link *whitelist);

#endif
