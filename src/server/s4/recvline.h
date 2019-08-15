#ifndef __RECVLINE_H_
#define __RECVLINE_H_

struct link {
        char str[4096];
        struct link *next;
};

void print(struct link *head);

void destroy(struct link *data);

struct link *receive_line(int connect_fd, char *symbol);

#endif
