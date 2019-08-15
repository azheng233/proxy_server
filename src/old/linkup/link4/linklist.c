#include <stdio.h>
#include <stdlib.h>
#include "linklist.h"

struct link *create(void)
{
        struct link *head;

        head = (struct link *)malloc(sizeof (struct link) );

        head->num = 0;
        head->next = NULL;

        return head;
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

struct link *insert(struct link *head, int place, int new_num)
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

        new_node->num = new_num;

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

void destroy(struct link *head)
{
        struct link *tmp;

        while (head) {
                tmp = head->next;
                free (head);
                head = tmp;
        }
}

void print(struct link *head)
{
        struct link *p;

        p = head->next;

        while (p != NULL) {
                printf("%d ", p->num);

                p = p->next;
        }
}

int locate (struct link *head, int e)
{
        int i = 0;

        while (head != NULL) {
                if (head->num == e) {
                        return i;
                } else {
                        head = head->next;
                        i = i+1;
                }
        }

}

