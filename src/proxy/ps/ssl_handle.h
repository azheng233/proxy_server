#ifndef __SSL_HANDLE_H_
#define __SSL_HANDLE_H_

#include "sort_request.h"
#include "recvline.h"

void refuse_ssl_connect(int connect_fd);

int ssl_process(char *ip, char *port, int connect_fd, start *line1, table *hash, SSL_CTX *s_ctx, SSL_CTX *c_ctx, struct link *whitelist);

#endif
