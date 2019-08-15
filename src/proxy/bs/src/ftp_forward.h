#ifndef _FTP_FORWARD_H_
#define _FTP_FORWARD_H_

#include "connection.h"

#define FTP_FORWARD_OK 0
#define FTP_FORWARD_ERR -1

int ftp_forward( connection_t *c );

int ftp_forward_http( connection_t *c );

#endif
