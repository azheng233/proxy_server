#include<stdio.h>
#include<stdlib.h>
#include"linklist.h"

int main()
{
        /*从创建列表开始*/
        struct link *head, *p1, *p2;
        char a[10000];
        int b, length;
        int place, new_num;
        int place_del;
        int e, i;

        head = create();
        p1 = create();
        if (NULL == head || NULL == p1) {
                printf ("内存分配失败");
                exit (1);
        }

        b = scanf("%d", &p1->num);
        setbuf(stdin, NULL);
        while (1 != b) {
                printf ("输入错误\n");
                b = scanf("%d", &p1->num);
                setbuf(stdin, NULL);
        }

        while (p1->num != 0){
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

                b = scanf("%d", &p1->num);
                setbuf(stdin, NULL);
                while (1 != b) {
                        printf ("输入错误\n");
                        b = scanf("%d", &p1->num);
                        setbuf(stdin, NULL);
                }

        }

        p2->next = NULL;

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

        b = scanf ("%d", &place);
       	setbuf(stdin, NULL);
        while (place <= 0 || place > length) {
     		printf ("输入错误\n");
     	        b = scanf("%d", &place);
                setbuf(stdin, NULL);
    	}

        printf("插入数值:");

        b = scanf("%d", &new_num);
        setbuf(stdin, NULL);
        while (1 != b) {
                printf ("输入错误\n");
                setbuf(stdin, NULL);
                b = scanf("%d", &new_num);
        }
	
	head = insert(head, place, new_num);
        if (head) {
                print(head);
        } else {
                printf("插入失败");
                exit(1);
        }

        length = link_length(head);
        printf("\n链表长度为:%d", length);

	//删除
        printf("\n删除节点位置:");

        scanf("%d", &place_del);
        setbuf(stdin, NULL);
        while (place_del < 0 && place_del >= length) {
                printf("超出范围");
       		scanf("%d", place_del);
	        setbuf(stdin, NULL);
                
        }

        delete(head, place_del);
        if (head) {
                print(head);
        } else {
                printf("\n插入失败");
                exit(1);
        }

	//查找
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

