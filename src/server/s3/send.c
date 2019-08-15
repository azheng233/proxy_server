#include "http.h"

void SERROR (char *xyfirst, int connect_fd, int a) 
{
	char *xyline;
	int n, err;

	xyline = malloc(1024);
	
	if (a == 500) 
		n = sprintf(xyline, "HTTP/1.1 500 Internal Server Error\r\n");
	if (a == 501) 
		n = sprintf(xyline, "HTTP/1.1 501 Not Impelmented\r\n");
	if (a == 505)
		n = sprintf(xyline, "HTTP/1.1 505 HTTP Version Not Supported\r\n");
	if (a == 400)
		n = sprintf(xyline, "HTTP/1.1 400 Bad Request\r\n");
	if (a == 403)
		n = sprintf(xyline, "HTTP/1.1 403 Forbidden\r\n");
	if (a == 404)
		n = sprintf(xyline, "HTTP/1.1 404 Not Found\r\n");
	
	printf("%s", xyline);

	sprintf(xyline + n , xyfirst);
	
	err = send(connect_fd, xyline, strlen(xyline), 0);
	if (-1 == err) {
		printf("send errno: %m (%d)\n", errno);
	}
	free(xyline);
	xyline = NULL;
}

int SFILE (char *xyfirst, int connect_fd, FILE *fd, char *URL)
{
	int n, len, err;
	char *buf;
	char *xyline;
	
	struct stat a;
	err = stat(URL, &a);
	if (err) {
		printf("stat error: %m\n");
		return -1;
	}

	xyline = malloc(1024);
		
	n = sprintf(xyline, "HTTP/1.1 200 OK\r\n");
	len = n;
	n = sprintf(xyline + len, xyfirst);
	len = len + (n - 2);
	n = sprintf(xyline + len, "Content-Type: application/pdf; charset=utf-8\r\nContent-Length: ");
	len += n;
	n = sprintf(xyline + len, "%d", a.st_size);
	len += n;
	n = sprintf(xyline + len, "\r\nConnection: close\r\n\r\n");

	err = send(connect_fd, xyline, strlen(xyline), 0);
	if (err < 0) {
		printf("send first error: %m\n");
		return -1;
	}
printf("%s",xyline);

	while (1) {
		buf = malloc(BUFLEN + 1);
		n = fread(buf, 1, BUFLEN, fd);
		if (ferror(fd)) {
			printf("fread error\n");
			return -1;
		}
		if (n == 0) {
			break;
		}
		
		buf[n] = '\0';
		
		err = send(connect_fd, buf, BUFLEN, 0);
		if (err < 0) {
			printf("send file error: %m\n");
			return -1;
		}
		free(buf);
		buf = NULL;
	}

	free(buf);
	buf = NULL;
	free(xyline);
	xyline = NULL;

	return 0;
} 
