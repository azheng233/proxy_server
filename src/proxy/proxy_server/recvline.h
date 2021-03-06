#ifndef __RECVLINE_H_
#define __RECVLINE_H_

#include "https.h"
#include "ssl_forward.h"
#include "sort_request.h"
#include "ssl_handle.h"

struct link {
        char str[4096];
        struct link *next;
};

struct link *create();

int find_word(char *buf, char *word);

void print(struct link *head);

void destroy(struct link *data);

char *sslread_line (SSL *server_ssl);

struct link *resolve(char *buf, char *symbol);

char *receive_tail();

struct link *receive_line(int connect_fd, char *symbol, SSL *server_ssl);

#endif
