#ifndef __CLIENT_H_
#define __CLIENT_H_

char *edit_message(start *line1, table *hash);

int http_client(char *ip, char *port, int connect_fd, start *line1, table *hash);

#endif
