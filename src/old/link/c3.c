#include <stdio.h>
#include <stdlib.h>

struct link {
        int num;
        struct link *next;
};
struct link *create(struct link * head, int n, int m, int number)
{
        struct link *p1, *p2;
        
	if ( m == 1) {
	        head = (struct link *)malloc(sizeof (struct link) );
		p1 = (struct link *)malloc(sizeof(struct link));

	        head->num = 0;
                head->next = p1;

		p1->num = number;
		p1->next = NULL;
	} else {	         
		head->next = p2;
		p1 = (struct link *)malloc(sizeof(struct link));
		p1->num = number;
        	p1->next = p2;
		p1 = head->next;
		
	}
	
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
	struct link *head;
	int n, m, number;
	
	printf("确定元素长度:");
	
	if (1 != scanf("%d", &n)) {
		printf("输入错误\n");
	}
	
	for (m = 1; m <= n; m++) {
		printf("第%d个元素", m);
		if (1 != scanf("%d", &number)) {
			printf("输入错误\n");
			exit(1);
		}
		
		head = create(head, n, m, number);
	}
	
	print(head);

}
