#ifndef _CONN_FTP_H_
#define _CONN_FTP_H_

#include <netinet/in.h>

int ftp_conn_create( int fd, struct sockaddr_in *addr );

#endif
