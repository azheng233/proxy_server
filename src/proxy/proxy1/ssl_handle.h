#ifndef __SSL_HANDLE_H_
#define __SSL_HANDLE_H_


void refuse_ssl_connect(int connect_fd);

int ssl_process(char *ip, char *port, int connect_fd, start *line1, table *hash);

#endif
