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

struct link {
        char num[4096];
        struct link *next;
};

struct link *create(void)
{
        struct link *head;

        head = (struct link *)malloc(sizeof (struct link) );
        head->next = NULL;
	strcpy(head->num, "0");
        return head;
}

char buff_one[4096], buff_new[4096], buff[4096];
char *receive(int connect_fd)
{

	char *p;
	int n;	

        n = recv(connect_fd, buff_one, sizeof(buff_one), 0);
        if (n == -1) {
                printf("recv error\n");
        }
        buff_one[n] = '\0';
//        printf("recv:%s\n", buff_one);
        strcpy(buff_new, buff_one);

        while (buff_one[0] != '\r' && buff_one[1] != '\n') {
                p = buff;

                n = recv(connect_fd, buff_one, sizeof(buff_one), 0);
                if (n == -1) {
                        printf("recv error\n");
                }
                if (buff_one[0] == '\r' && buff_one[1] == '\n') {
                        break;
                }
      	 	buff_one[n] = '\0';
//		printf("recv:%s\n", buff_one);

                conn(buff_new,buff_one,p);

                strcpy(buff_new, p);
        }
	return buff_new;
}

int link_length(struct link *head)
{
        int length = 0;
        struct link *tmp;

        tmp = head->next;

        while (tmp != NULL) {
                length++;
                tmp = tmp->next;
        }

        return length;
}

struct link *insert(struct link *head, int place, char *new_num)
{
        int i;
        struct link *new_node;
        struct link *behind;
        struct link *front;

        new_node = (struct link *)malloc(sizeof(struct link));
        if (NULL == new_node) {
                printf ("内存分配失败\n");
                exit (1);
        }

        strcpy(new_node->num, new_num);

        behind = head->next;

        if (place != 1) {
                for (i=1; i<place; i++) {
                        front = behind;
                        behind = front->next;
                }

                new_node->next = behind;
                front->next = new_node;

        } else {
                head->next = new_node;
                new_node->next = behind;
        }

        return head;
}

void delete (struct link *head, int place_del)
{
        int i;
        struct link *del;
        struct link *front;

        del = head->next;

        if (place_del != 1) {
                for (i=1; i<place_del; i++) {
                        front = del;
                        del = front->next;
                }

                front->next = del->next;
        } else {
                head->next = del->next;
        }

        free(del);

}

int locate (struct link *head, char *e)
{
        int i = 0;

        while (head != NULL) {
                if (0 == strcmp(head->num, e)) {
                        return i;
                } else {
                        head = head->next;
                        i = i+1;
                }
        }

}

void print(struct link *head)
{
        struct link *p;

        p = head->next;

        while (p != NULL) {
                printf("%s ", p->num);

                p = p->next;
        }
}


int main ()
{
	int socket_fd, connect_fd;
	struct sockaddr_in seraddr;
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

/***************************************************************************/
        struct link *head, *p1, *p2;
        int length;

        head = create();
        p1 = create();
        if (NULL == head || NULL == p1) {
                printf ("内存分配失败");
                exit (1);
        }

       	strcpy(p1->num,receive(connect_fd));
        printf("p1->num:%s\n", p1->num);

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
		
		strcpy(p1->num,receive(connect_fd));
                printf("p1->num:%s\n", p1->num);
	

	}
	
	printf("结束\n");	

        p2->next = NULL;

        if (head) {
                print(head);
        } else {
                printf("单向链表生成失败\n");
                exit(1);
        }

        length = link_length(head);
        printf("\n链表长度为：%d", length);
/*********************************************************************/
	int place;
	char new_num[4096];
        
	printf ("\n插入位置:");
	place = atoi(receive(connect_fd));
	printf("%d\n", place);
        while (place <= 0 || place > length) {
                printf ("输入错误\n");
		place = atoi(receive(connect_fd));
		printf("插入位置:%s\n", new_num);
        }

        printf("插入数值:");
	strcpy(new_num, receive(connect_fd));
	printf("%s\n", new_num);
	
	head = insert(head, place, new_num);
        if (head) {
                print(head);
        } else {
                printf("插入失败");
                exit(1);
        }

        length = link_length(head);
        printf("\n链表长度为:%d", length);
/********************************************************************/
	int place_del;

        printf ("\n删除位置:");

        place_del = atoi(receive(connect_fd));
        printf("%d\n", place_del);
        while (place_del <= 0 || place_del > length) {
                printf ("输入错误\n");
                place_del = atoi(receive(connect_fd));
                printf("删除位置:%s\n", new_num);
        }

        delete(head, place_del);
        if (head) {
                print(head);
        } else {
                printf("\n删除失败");
                exit(1);
        }
/********************************************************************/
	int i;
	char e[4096];

	printf("\n查找节点：");
	strcpy(e, receive(connect_fd));
	printf("%s\n", e);

        i = locate(head, e);
        if (i == 0) {
                printf ("没有该数值");
        } else {
                printf("该节点在第%d\n节点", i);
	}
/*********************************************************************/

	close(connect_fd);
	close(socket_fd);

	return 0;
}
