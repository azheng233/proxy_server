#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "server.h"

#define PORT 8000
#define MAXLINE 4096


struct link *create(void)
{
        struct link *head;

        head = (struct link *)malloc(sizeof (struct link) );
        head->next = NULL;
        strcpy(head->num, "0");
        return head;
}



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

char *receive(int connect_fd, char *buff_new)
{
	char buff_one[4096], buff[4096];
        char *p;
        int n;
	
	memset(buff_one, 0, sizeof(buff_one));
        n = recv(connect_fd, buff_one, sizeof(buff_one), 0);
        if (n == -1) {
                printf("recv error\n");
		exit(0);
        } else if (n == 0) {
                strcpy(buff_one, "Client-side disconnection");
        }

        if (buff_one[0] == '\r' && buff_one[1] == '\n') {
                printf("节点不得以回车开头");
                n = recv(connect_fd, buff_one, sizeof(buff_one), 0);
                if (n == -1) {
                        printf("recv error");
                } else if (n == 0) {
                        strcpy(buff_one, "Client-side disconnection");
                }
        }

        strcpy(buff_new, buff_one);

        while (buff_one[0] != '\r' && buff_one[1] != '\n') {
                p = buff;

                n = recv(connect_fd, buff_one, sizeof(buff_one), 0);
                if (n == -1) {
                        printf("recv error\n");
			exit(0);
                } else if (n == 0) {
			break;
        	}

                if (buff_one[0] == '\r' && buff_one[1] == '\n') {
                        break;
                }
                buff_one[n] = '\0';

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


