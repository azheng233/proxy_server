#include <stdio.h>
#include <stdlib.h>

struct link {
	int num;
	struct link *next;
};

struct link *create(void)
{
	struct link *head, *p1, *p2;

	head = (struct link *)malloc(sizeof (struct link) );
	if (NULL == head){
		printf ("内存分配失败");
		exit (1);
	}
	
	head->num = 0;
	head->next = NULL;

	p1 = (struct link *)malloc(sizeof(struct link) );
	if (NULL == p1){
		printf ("内存分配失败\n");
		exit (1);
	}

	if (1 != scanf("%d", &p1->num)) {
		printf ("输入错误\n");
		exit (1);
	}

	while (p1->num != 0){
		if (head->next == NULL)
			head->next = p1;
		else
			p2->next = p1;
	
		p2 = p1;
	
		p1 = (struct link *) malloc (sizeof (struct link) );
		if (NULL == p1) {
			printf ("内存分配失败\n");
			exit (1);
		}
		
		if (1 != scanf("%d",&p1->num)) {
			printf ("输入错误\n");
			exit (1);
		}
	}

	p2->next = NULL;

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

int main()
{
	/*从创建列表开始*/
	struct link *head;
	int length;
	int place, new_num;
	int place_del;
	int e, i;

	head = create();
	if (head) {
		print (head);
	} else {
		printf("单向链表生成失败\n");
		exit(1);
	}

	length = link_length(head);
	printf("\n链表长度为：%d", length);

	//插入节点
	printf ("\n插入位置:");

	scanf ("%d", &place);
	if (place <= 0 || place > length) {
		printf("超出范围");
		exit(1);
	}
	
	printf("插入数值:");
	if (1 == scanf("%d", &new_num)) {
		head = insert(head, place, new_num);
	} else {
		printf("输入错误");
		exit(1);
	}

	if (head) {
		print(head);
	} else {
		printf("插入失败");
		exit(1);
	}

	length = link_length(head);
	printf("\n链表长度为:%d", length);

	//删除节点

	printf("\n删除节点位置:");

	scanf ("%d", &place_del);
	if (place_del > 0 && place_del <= length) {
		delete(head, place_del);
	} else {
		printf("超出范围");
		exit(1);
	}

	if (head) {
		print(head);
	} else {
		printf("\n插入失败");
		exit(1);
        }

	//查找链表元素    

	printf("\n输入查找数值:");
	scanf("%d", &e);

	i = locate(head, e);
	if (i == 0)
		printf ("没有该数值");
	else
		printf("该数值在第%d节点\n", locate(head,e));

	//销毁链表
	destroy(head);

}
