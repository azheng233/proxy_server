#ifndef __CLIENT_H_
#define __CLIENT_H_


#include "https.h"
#include "sort_request.h"
#include "recvline.h"
#include "judge_request.h"

struct link;
typedef struct request_start start;
typedef struct hash_table table;

char *edit_message(start *line1, table *hash);

int http_client(char *ip, char *port, int connect_fd, \
		start *line1, table *hash, struct link *whitelist);

#endif
