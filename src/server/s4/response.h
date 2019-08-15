#ifndef __RESPONSE_H__
#define __RESPONSE_H__

void send_error(int connect_fd, int error_code);

int send_file(int connect_fd, FILE *fd, char *URL);

#endif
