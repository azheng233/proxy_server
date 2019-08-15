/*#include <stdio.h>
#include <stdlib.h>

struct link {
        int num;
        struct link *next;
};

struct link *create(void)
{
        struct link *head;

        head = (struct link *)malloc(sizeof (struct link) );
        if (NULL == head){
                printf ("内存分配失败");
                exit (1);
        }

        head->num = 0;
        head->next = NULL;

        return head;
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

int main()
{
        struct link *head, *p1, *p2;
        int length;
        int place, new_num;
        int place_del;
        int e, i;

        head = create();

        p1 = create();

        while (1 != scanf("%d", &p1->num)) {
                printf ("输入错误\n");
        }

        while (p1->num != 0){
                if (head->next == NULL)
                        head->next = p1;
                else
                        p2->next = p1;

                p2 = p1;

                p1 = create();

                if (1 != scanf("%d",&p1->num)) {
                        printf ("输入错误\n");
                        exit (1);
                }
        }

        p2->next = NULL;

        if (head) {
                print (head);
        } else {
                printf("单向链表生成失败\n");
                exit(1);
        }


｝*/

#include<stdio.h>

int main()
{
	int a,b;
	b = scanf("%d", &a);
	setbuf(stdin,NULL);
	while (a != 1) {
		printf("F\n");
		setbuf(stdin,NULL);
		b = scanf("%d", &a);
		printf("%d", b);
	}

	printf("%d", a);	

	return 1;
}






