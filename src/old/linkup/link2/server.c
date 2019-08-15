#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DEFAULT_PORT 8000
#define MAXLINE 4096

struct link {
	char str[4096];
	struct link *next;
};

struct link *create(void)
{
        struct link *head;

        head = (struct link *)malloc(sizeof(struct link));
        head->next = NULL;
        strcpy(head->str, "0");
        return head;
}

void conn(char *buff1, char *buff2, char *p)
{

        for(; *buff1!='\0';) {
                *p=*buff1;
                buff1++;
                p++;
        }
        for(; *buff2!='\0';) {
                *p=*buff2;
                buff2++;
                p++;
        }
        *p='\0';
}

void print(struct link *head)
{
        struct link *p;

        p = head->next;

        while (p != NULL) {
		if (strcmp(p->str, "\n")) {
                	printf("%s->", p->str);
		} else {
			printf("回车\n");
		}
                p = p->next;
        }
}


int main(int argc, char** argv)
{
    	int socket_fd, connect_fd;
    	struct sockaddr_in servaddr;
    	char buff[4096];
    	int n;
    	int on=1;
	
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
    	servaddr.sin_family = AF_INET;
    	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。
	servaddr.sin_port = htons(DEFAULT_PORT);//设置的端口为DEFAULT_PORT

    	setsockopt( socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );//设置端口重用
	
	if (bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
    		printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
    		exit(0);
    	}

	if (listen(socket_fd, 10) == -1)
    	{
    		printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
    		exit(0);
    	}

 	printf("======waiting for client's request======\n");
	
	if( (connect_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL)) == -1) {
        	printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
		exit(0);
        }

	struct link *head, *p1, *p2, *enter;
	head = create();

	char buff_one[2], buff_new[100], str[100], buff_s[4096];
	char *p;
	int j=1;
	while (1) {	
		int i=0;
		n = recv(connect_fd, buff, 5, 0);
		if (n == 0) {
			printf("over\n");
			break;
		}	

		while (i < n) {
			while (buff[i] == ' ') {
				i = i+1;
				if (buff[i] != ' ') {
					printf("数据1：%s\n", buff_new);

					p1 = create();

					strcpy(p1->str, buff_new);
					if (head->next == NULL) {
						head->next = p1;
					} else {
						p2->next = p1;
					}
					p2 = p1;
					j = 1;
				}
			}

			while (buff[i] == '\n') {
				i = i+1;
				if (buff[i] != '\n') {
					printf("数据2：%s\n", buff_new);
				
					p1 = create();
					enter = create();
					strcpy(enter->str , "\n");

					strcpy(p1->str, buff_new);
					if (head->next == NULL) {
						head->next = p1;
					} else {
						p2->next = p1;
					}
					p1->next = enter;
					p2 = enter;
				}	
				j = 1;
			}

			if (j) {
				if (buff[i] == ' ') {
					i = i+1;	
				}
				buff_new[0] = buff[i];
				buff_new[1] = '\0';
				i = i+1;
			}

                        if (i == n) {
                                j = 0;
                        }

			for (i;i<n;i++) {
				if (buff[i] == ' ' || buff[i] == '\n') {
					break;	
				}
				p = str;
				buff_one[0] = buff[i];
				buff_one[1] = '\0';
				conn(buff_new, buff_one, p); 
				strcpy(buff_new, p);
				
				if ((i+1) == n) {
					j = 0;
				}
			}
		}
	}
	p2->next = NULL;
	print(head);
	
	close(connect_fd);
	close(socket_fd);
	return 0;
}
