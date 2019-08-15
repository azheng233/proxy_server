#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8000
#define MAXLINE 100

void conn(char *buff_one, char *buff_new, char *p)
{

        for(; *buff_one!='\0';) {
                *p=*buff_one;
                buff_one++;
                p++;
        }
        for(; *buff_new!='\0';) {
                *p=*buff_new;
                buff_new++;
                p++;
        }
        *p='\0';
}

int main ()
{
	int socket_fd, connect_fd;
	struct sockaddr_in seraddr;
	char buff_one[4096], buff_new[4096], buff[4096];
	int n;
	char *p;
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

		n = recv(connect_fd, buff_one, sizeof(buff_one), 0);
		if (n == -1) {
			printf("recv error\n");
		}
		printf("%s\n", buff_one);
	
		p = buff;
	int i = 0;
	while (i<10) {
		i++;
		recv(connect_fd, buff_new, sizeof(buff_new), 0);
		printf("%s\n", buff_new);
//		if (buff == 0) {
			
//			break;
//		} else {
		conn(buff_one, buff_new, p);
		printf("revc msg from client: %s\n",p);
		buff_one[4096] = *p;
		p = buff;
	}
/*
	while (1) {
		printf("Received:");
                scanf("%s",buff);
		if (buff[1] == 0) {
                        break;
                } else {
                
			if ((send(connect_fd, buff, sizeof(buff), 0)) < 0) {
        			perror("send error");
				break;
        		}
		}
	}
*/	close(connect_fd);
	close(socket_fd);

	return 0;
}
