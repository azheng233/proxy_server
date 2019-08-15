#ifndef __RESPONSE_ERR_
#define __RESPONSE_ERR_

char *get_time();

void send_err(int connect_fd, int err_code);

#endif
