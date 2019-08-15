#ifndef __RECVLINE_H_
#define __RECVLINE_H_

struct link;

void print(struct link *head);

void destroy(struct link *data);

struct link *receive_line(int connect_fd, char *symbol);

#endif
