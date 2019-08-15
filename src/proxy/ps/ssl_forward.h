#ifndef __SSL_FORWARD_H_
#define __SSL_FORWARD_H_

#include "sort_request.h"
#include "recvline.h"

int SSL_forward(SSL *server_ssl, SSL *client_ssl, start *line1, struct link *whitelist);

#endif
