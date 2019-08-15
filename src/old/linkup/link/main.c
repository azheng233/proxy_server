#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "server.h"

#define PORT 8000
#define MAXLINE 4096

int main ()
{
        int socket_fd, connect_fd;
        struct sockaddr_in seraddr;
        int on = 1;
        struct link *head, *p1, *p2;//head为头指针，p1为新节点，p2为尾节点
        int length;	//链表节点个数
	int place;	//插入节点位置
        int place_del;	//删除节点位置
	int i;		//查找节点所在位置
	char e[4096];	//查找的节点内容
        char new_num[4096];	//插入节点的
	char number[4096], number_del[4096], buff_new[4096];

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
	while (1) {
	        printf("======waiting for client's request======\n");
	
	        connect_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL);
	        if (connect_fd < 0) {
	                perror("accept error");
	                exit(1);
	        } else {
	                printf("welcome\n");
		}
		while (1) { 	
			//开始创建列表
	        	head = create();
	        	p1 = create();
	        	if (NULL == head || NULL == p1) {
	        	        printf ("内存分配失败");
	        	        exit (1);
	        	}
	
	        	strcpy(p1->num,receive(connect_fd, buff_new));
			printf("p1->num:%s\n", p1->num);
                        if (0 == strcmp(p1->num, "Client-side disconnection")) {
                                goto end;
                        }
	
	        	while (strcmp(p1->num, "0" )) {
	        	        if (head->next == NULL)
	        	                head->next = p1;
	        	        else
	        	                p2->next = p1;
	
	        	        p2 = p1;
	
	        	        p1 = create();
	        	        if (NULL == p1) {
	        	                printf ("内存分配失败");
	        	                exit (1);
	        	        }
	
	        	        strcpy(p1->num,receive(connect_fd, buff_new));
			        printf("p1->num:%s\n", p1->num);
	                        if (0 == strcmp(p1->num, "Client-side disconnection")) {
                                	goto end;
                        	}
	        	}
	
	        	printf("结束\n");
	
	        	p2->next = NULL;
	
	        	if (head) {
	        	        print(head);
	        	} else {
	        	        printf("单向链表生成失败\n");
	        	        exit(1);
	        	}
			//获取链表长度
	 		length = link_length(head);
	        	printf("\n链表长度为：%d\n", length);
			if (!length) {
				exit(0);
			}
	
			//插入新节点
	        	printf ("插入位置:\n");

			strcpy(number, receive(connect_fd, buff_new));
	      		if (0 == strcmp(number, "Client-side disconnection")) {
				printf("Client-side disconnection\n");
                         	goto end;
                        }

	        	place = atoi(number);
	        	printf("%d\n", place);
	        	while (place <= 0 || place > length) {
	        	        printf ("输入错误\n");
				memset(number, 0, sizeof(number));

				strcpy(number, receive(connect_fd, buff_new));
	      			if (0 == strcmp(number, "Client-side disconnection")) {
					printf("Client-side disconnection\n");
                                	goto end;
                        	}
	        	       
				place = atoi(receive(connect_fd, buff_new));
	        	        printf("插入位置:%d\n", &place);
			}
				
			printf("插入数值:\n");
	        
			strcpy(new_num, receive(connect_fd, buff_new));
			while (new_num[0] == '\r' && new_num[1] == '\n') {
	        	        printf("节点无效\n");
	        	        strcpy(new_num,receive(connect_fd, buff_new));
	        	}

	                if (0 == strcmp(new_num, "Client-side disconnection")) {
                                printf("Client-side disconnection\n");
                                goto end;
                        }
	
			printf("%s\n", new_num);
	
	        	head = insert(head, place, new_num);
	        	if (head) {
	        	        print(head);
	        	} else {
	        	        printf("插入失败\n");
	        	        exit(1);
	        	}
	
	        	length = link_length(head);
	        	printf("\n链表长度为:%d\n", length);
	
			//删除节点
	        	printf ("删除位置:\n");

			strcpy(number_del, receive(connect_fd, buff_new));
	      		if (0 == strcmp(number_del, "Client-side disconnection")) {
                                printf("Client-side disconnection\n");
                                goto end;
                        }
	
	        	place_del = atoi(number_del);
	        	printf("%d\n", place_del);
	        	while (place_del <= 0 || place_del > length) {
	        	        printf ("输入错误\n");
				
				strcpy(number_del, receive(connect_fd, buff_new));
	      			if (0 == strcmp(number_del, "Client-side disconnection")) {
                                	printf("Client-side disconnection\n");
                               	 	goto end;
                        	}
					
	        		place_del = atoi(number_del);
	        	        printf("%d\n", &place_del);
	        	}
	
	        	delete(head, place_del);
	        	if (head) {
	        	        print(head);
	        	} else {
	        	        printf("删除失败\n");
	        	        exit(1);
	        	}
	
	        	length = link_length(head);
	        	printf("\n链表长度为:%d\n", length);
	        	if (!length) {
	        	        exit(0);
	        	}
	
			//查找节点
	        	printf("查找节点:\n");
	        	strcpy(e, receive(connect_fd, buff_new));
	        	while (e[0] == '\r' && e[1] == '\n') {
	        	        printf("节点无效\n");
	        	        strcpy(e,receive(connect_fd, buff_new));
	        	}

                        if (0 == strcmp(e, "Client-side disconnection")) {
                                printf("Client-side disconnection\n");
                                goto end;
                        }
	
	        	printf("%s\n", e);
	
	        	i = locate(head, e);
	        	if (i == 0) {
	        	        printf ("没有该数值\n");
	        	} else {
	        	        printf("该节点在第%d节点\n", i);
	        	}
	                        length = link_length(head);
                        printf("链表长度为：%d\n", length);

			printf("按任意键继续\n");
                        int n;
                        n = recv(connect_fd, buff_new, sizeof(buff_new),0);
                        if(n == 0) {
                                close(connect_fd);
                                break;
                        } else {
                                printf("Restart the main\n");
                        }
		}
		end: printf("close connection\n");
	}

        close(connect_fd);
        close(socket_fd);

        return 0;
}
                    




