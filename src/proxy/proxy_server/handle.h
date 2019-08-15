#ifndef __HANDLE_H_
#define __HANDLE_H_

#include "https.h"
#include "sort_request.h"
#include "recvline.h"
#include "judge_request.h"
#include "client.h"
#include "ssl_handle.h"
#include "response_err.h"

struct link;
typedef struct request_start start;
typedef struct hash_table table;

int handle(int connect_fd, SSL_CTX *server_ssl, SSL_CTX *client_ctx, struct link *whitelist);

#endif
