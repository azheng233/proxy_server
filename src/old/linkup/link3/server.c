#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8000
#define MAXLINE 4096

int main ()
{
	int socket_fd, connect_fd;
	struct sockaddr_in seraddr;
	char buff[4096];
	int n;
        int on = 1;

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("create socket error\n");
		exit(0);
	}

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

		memset(buff,0,sizeof(buff));
		n = recv(connect_fd, buff, sizeof(buff), 0);
		if (n == -1) {
			printf("recv error\n");
		}
		
//		if (buff == 0) {
			
//			break;
//		} else {
			printf("revc msg from client: %s\n",buff);
//		}
	close(connect_fd);
	close(socket_fd);

	return 0;
}