#include<stdio.h>
#include<stdlib.h>

struct link //定义结构体
{
	int num;
	struct link *next;
} ;
int n;
struct link *creat(void)//创建单向列表
{
	struct link *p0,*p1,*p2;//p0为表头，p1为新入节点，p2为尾节点
	n=0;
	p1=p2=(struct link *)malloc(sizeof(struct link));
	scanf("%d",&p1->num);	
	p0=NULL;
	while(p1->num!=0)//开始加节点
	{
		n=n+1;
		if(n==1) p0=p1;
		else (p2->next)=p1;
		p2=p1;
		p1=(struct link *)malloc(sizeof(struct link));
		scanf("%d",&p1->num);	
	}
	(p2->next)=NULL;
	return p0;
}
void print(struct link *p0)
{
	struct link *p;
	p=p0;
	if(p0!=NULL);
	do
	{
	printf("%d ",p->num);
	p=p->next;
	}while(p!=NULL);
}

struct link *insert(struct link *p0,int k,int number)
{
        int l;
	struct link *s;
        struct link *p5;
        struct link *p6;

        p5 = p0;
        p6 = p5;

        for(l=1; l<k; l++)
        {   
                p6 = p5;
                p5 = p6->next;
        }

 	s = (struct link *)malloc(sizeof(struct link));

	s->num = number;

	s->next = p5;
	p6->next = s;

	return p0;	
}


void delete(struct link *p0,int i)
{
	int j;
	struct link *p3;//临时
	struct link *p4;//p4为i-1节点
	p3=p0;
	p4=p3;
	for(j=1;j<i;j++)
	{
		p4=p3;
		p3=p4->next;
	}
	p4->next=p3->next;
	free(p3);
}

void destroy(struct link *p0)
{
	struct link *p7;
	while(p0)
	{
		p7=p0->next;
		free(p0);
		p0=p7;
	}
}

int main()
{
	struct link *p0;
	p0=creat();
	print(p0);

	int l ,number;
	printf("\n插入位置和数值\n");
	scanf("%d%d",&l,&number);
	p0=insert(p0,l,number);
	print(p0);	

	printf("\n 删除节点编号：");
	int i;
	scanf("%d",&i);
	delete(p0,i);
	print(p0);
	
	printf("\n销毁\n");
	destroy(p0);
	print(p0);
	return 0;		 
}
