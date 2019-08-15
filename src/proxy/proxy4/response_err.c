#include "response_err.h"

char *gmt_time()
{
	time_t t;
	struct tm *gmt;
	char *timeinfo;
	size_t n;

	timeinfo = malloc(100);
	if (timeinfo == NULL) {
		return NULL;
	}

	t = time(NULL);
	gmt = gmtime(&t);

	n = strftime(timeinfo, 100, "Date: %a, %d %b %Y %X GMT\r\n", gmt);
	if (n <= 0) {
		timeinfo[0] = '\0';
	} else {
		timeinfo[n] = '\0';
	}
	return timeinfo;
}

static void send_fail(char *fail, int connect_fd)
{
	char *buff, *err_message;
	FILE *fd;
	struct stat failinfo;
	int err, n;
	
        err = stat(fail, &failinfo);
        if(err) {
                printf("stat err500 error\n");
               	return;
        }

	err_message = malloc(1024);
	if (err_message == NULL) {
		return;
	}
        n = sprintf(err_message,
        "Content-Type:text; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        failinfo.st_size);

	n = send(connect_fd, err_message, strlen(err_message), 0);
	if (n <= 0) {
		return;
	}
fprintf(stderr, "err message: %s", err_message);
	free(err_message);
	err_message = NULL;
	
	buff = malloc(1024);
	if (buff == NULL) {
		fprintf(stderr, "response_err malloc buff error\n");
		return;
	}
	
	fd = fopen(fail, "r");
	if (fd == NULL)
		return;

        while (1) {
                n = fread(buff, 1, 1023, fd);
                if (ferror(fd)) {
                        printf("fread error\n");
                        break;

                }
                if (n == 0) {
                        break;
                }

                buff[n] = '\0';
		fprintf(stderr, "n: %d  strlen: %d  buff[n-1]: %d\n", n, strlen(buff), buff[n-1] );
                err = send(connect_fd, buff, n, 0);
                if (err < 0) {
                        printf("send file error: %m\n");
                        break;
                }
                fprintf(stderr, "%s\n", buff);
        }
		
	free(buff);
	buff = NULL;

	fclose(fd);
}

void err_500(int connect_fd)
{
	char err_message[1024];
	char *fail = "err/err500.html";
	char *time;
	int n;

	time = gmt_time();

	n = sprintf(err_message, 
	"HTTP/1.1 500 Internal Server Error\r\n%s", time);
	if (n < 0) {
		printf("Sprintf error: %m\n");
		return;
	}
	send(connect_fd, err_message, strlen(err_message), 0);
	send_fail(fail, connect_fd);

	free(time);
	time = NULL;
}

void err_400(int connect_fd)
{
	char err_message[1024];
	char *fail = "err/err400.html";
	char *time;
	int n;

	time = gmt_time();
	
	n = sprintf(err_message, 
	"HTTP/1.1 400 Bad Request\r\n%s", time);
	if (n < 0) {
		printf("Sprintf error: %m\n");
		return;
	}
	send(connect_fd, err_message, strlen(err_message), 0);
	send_fail(fail, connect_fd);

	free(time);
	time = NULL;
}

void err_403(int connect_fd)
{
	char err_message[1024];
	char *fail = "err/err403.html";
	char *time;
	int n;

	time = gmt_time();
	
	n = sprintf(err_message, 
	"HTTP/1.1 403 Forbidden\r\n%s", time);
	if (n < 0) {
		printf("Sprintf error: %m\n");
		return;
	}
	send(connect_fd, err_message, strlen(err_message), 0);
	send_fail(fail, connect_fd);

	free(time);
	time = NULL;
}

void err_404(int connect_fd)
{
	char err_message[1024];
	char *fail = "err/err404.html";
	char *time;
	int n;

	time = gmt_time();
	
	n = sprintf(err_message, 
	"HTTP/1.1 404 Not Found\r\n%s", time);
	if (n < 0) {
		printf("Sprintf error: %m\n");
		return;
	}
	send(connect_fd, err_message, strlen(err_message), 0);
	send_fail(fail, connect_fd);

	free(time);
	time = NULL;
}

void err_105(int connect_fd)
{
	char err_message[1024];
	char *fail = "err/err105.html";
	char *time;
	int n;

	time = gmt_time();
	
	n = sprintf(err_message, 
	"HTTP/1.1 400 Not Found\r\n%s", time);
	if (n < 0) {
		printf("Sprintf error: %m\n");
		return;
	}
	fprintf(stderr, "%s", err_message);
	send(connect_fd, err_message, strlen(err_message), 0);
	send_fail(fail, connect_fd);

	free(time);
	time = NULL;
}

void send_err(int connect_fd, int err_code)
{

	if (err_code == 500) {
		err_500(connect_fd);
	} else if (err_code == 400) {
		err_400(connect_fd);
	} else if (err_code == 403) {
		err_403(connect_fd);
	} else if (err_code == 404) {
		err_404(connect_fd);
	} else if (err_code == 105) {
		err_105(connect_fd);
	}
	
}
