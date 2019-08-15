#ifndef __SSL_HANDLE_H_
#define __SSL_HANDLE_H_

#include "https.h"
#include "client.h"
#include "sort_request.h"
#include "recvline.h"
#include "ssl_forward.h"

struct link;
typedef struct request_start start;
typedef struct hash_table table;

void refuse_ssl_connect(int connect_fd);

int ssl_process(char *ip, char *port, int connect_fd, start *line1, table *hash, SSL_CTX *s_ctx, SSL_CTX *c_ctx, struct link *whitelist);

#endif
