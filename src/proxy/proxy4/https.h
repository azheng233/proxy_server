#ifndef __HTTP_H_
#define __HTTP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8000
#define MAXLINE 100
#define BUFLEN 1024
#define BUCKET 10
/*
enum ERR_CODE {
	OK = 0;
	DIS_CONNECTION = 1
	PROXY_UNANTICIPAT_ERR = 500,
	HEADER_FORMAT_ERR = 400,
	PROXY_REFUSE_SERVICE = 403,
	REQUEST_URL_ERR = 404;
}
*/
#endif
