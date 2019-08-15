#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


#define PORT 8000
#define MAXLINE 1024

int main ()
{
        int socket_fd, connect_fd;
        struct sockaddr_in seraddr;
        int on = 1;

        if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                printf("create socket error\n");
                exit(0);
        }

	int flags;

        memset(&seraddr, 0, sizeof(seraddr));

        seraddr.sin_family = AF_INET;
        seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        seraddr.sin_port = htons(PORT);

        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

        if (-1 == bind(socket_fd, (struct sockaddr *)&seraddr, sizeof(seraddr))) {
                printf("bind socket error\n");
                exit(0);
        }

        if (listen(socket_fd, 10) == -1) {
                printf("listen socket error\n");
                exit(0);
        }

        printf("======waiting for client's request======\n");

        connect_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL);
        if (connect_fd < 0) {
                perror("accept error");
                exit(1);
        }

	char buf[MAXLINE];
	int n, a;
//	while (1) {
		n = recv(connect_fd, buf, 1024, 0);
			printf("%s\n",buf);
/*		
		if (n >= 0) {
			break;
		}
		
		if (n < 0 && errno == EAGAIN) {
			printf("1\n");
		} else {
			printf("recv error: %m(errno:%d)",errno);
			break;
		}
		
	}*/

	scanf("%d", &a);

}
