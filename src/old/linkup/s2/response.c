#include "http.h"
#include "recvline.h"
#include "edit_message.h"

void free_pointer(char *p)
{
	free(p);
	p = NULL;
}

char *response_head()
{
	char *head;
        int n, len;
        time_t t;
        struct tm *timeinfo;
    
        time(&t);
        timeinfo = localtime(&t);

        head = malloc(100);
        n = sprintf(head, "Server: weizheng\r\nDate: ");
        len = n;
        n = sprintf(head + len, asctime(timeinfo));
        len += n; --len;
        n = sprintf(head + len, "\r\n");

	return head;

}

void send_error(int connect_fd, int error_code) 
{
	char *response_message;
	int len, n, err;
	char *res_head;

	if (error_code == -1) {
		free_pointer(response_message);
		free_pointer(res_head);
		return;
	}

	response_message = malloc(1024);
	
	if (error_code == 500) 
		n = sprintf(response_message, "HTTP/1.1 500 Internal Server Error\r\n");
	if (error_code == 501) 
		n = sprintf(response_message, "HTTP/1.1 501 Not Impelmented\r\n");
	if (error_code == 505)
		n = sprintf(response_message, "HTTP/1.1 505 HTTP Version Not Supported\r\n");
	if (error_code == 400)
		n = sprintf(response_message, "HTTP/1.1 400 Bad Request\r\n");
	if (error_code == 403)
		n = sprintf(response_message, "HTTP/1.1 403 Forbidden\r\n");
	if (error_code == 404)
		n = sprintf(response_message, "HTTP/1.1 404 Not Found\r\n");
	
	len = n;
	n = sprintf(response_message + n, res_head);
	len += n;	
	n = sprintf(response_message + len, "Connection: close\r\n\r\n");

	printf("Send\n%s", response_message);

	err = send(connect_fd, response_message, strlen(response_message), 0);
	if (-1 == err) {
		printf("send errno: %m (%d)\n", errno);
	}
	free_pointer(response_message);
	free_pointer(res_head);
}

int send_file(int connect_fd, FILE *fd, char *URL)
{
	int n, len, err;
	char *buf;
	char *response_message, *res_head;
	struct stat a;

	res_head = response_head();
	err = stat(URL, &a);
	if (err) {
		free_pointer(res_head);
		printf("stat error: %m\n");
		return 500;
	}
	
	response_message = malloc(1024);
		
	n = sprintf(response_message, "HTTP/1.1 200 OK\r\n");
	len = n;
	n = sprintf(response_message + len, res_head);
	len += n;
	n = sprintf(response_message + len, "Content-Type: text/html\r\nContent-Length: ");
	len += n;
	n = sprintf(response_message + len, "%d", a.st_size);
	len += n;
	n = sprintf(response_message + len, "\r\nConnection: close\r\n\r\n");

	err = send(connect_fd, response_message, strlen(response_message), 0);
	if (err < 0) {
		printf("send first error: %m\n");
		return -1;
	}
	printf("%s",response_message);

	buf = malloc(BUFLEN + 1);
	while (1) {
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
		printf("%s",buf);
	}

	free_pointer(buf);
	free_pointer(response_message);
	free_pointer(res_head);

	return 0;
} 
