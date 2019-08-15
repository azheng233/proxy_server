#include<stdio.h>
#include<stdlib.h>

struct link
{
	int num;
	struct link *next;
};

struct link *creat(void)
{
	struct link *p0,*p1,*p2;

	p0=(struct link *)malloc(sizeof(struct link));
	if(NULL == p0)
	{
		printf("内存分配失败\n");
		exit(1);
	}

	p0->num = 0;
	p0->next = NULL;

	p1 = (struct link *)malloc(sizeof(struct link));
	if(NULL == p1)
        {
                printf("内存分配失败\n");
                exit(1);
        }

	scanf("%d",&p1->num);

	while(p1->num!=0)
	{
		if(p0->next == NULL)
		{	
			 p0->next = p1;	
		}
		else
		{
			 p2->next = p1;
		}
		p2 = p1;	
	
		p1 = (struct link *)malloc(sizeof(struct link));
		if(NULL == p1)
      		{
        	        printf("内存分配失败\n");
	                exit(1);
	        }
	
		scanf("%d",&p1->num);
	}

	p2->next = NULL;
	return p0;
}

int  linklength(struct link *p0)
{
	int length = 0;
	struct link *len;
	
	len = p0->next;
	while(len != NULL)
	{
		length++;
		len = len->next;
	}
	
	return length;
}

struct link *insert(struct link *p0,int place,int newnum)
{
	int i,length;

	struct link *pnew;
	struct link *pplace;
	struct link *pfront;
	
	pnew = (struct link *)malloc(sizeof(struct link));
        if(NULL == pnew)
        {
                printf("内存分配失败\n");
                exit(1);
        }

	pnew->num = newnum;	
	
	pplace = p0->next;

	if(place != 1)
	{
		for(i=1;i<place;i++)
		{	
			pfront = pplace;
			pplace = pfront->next;
		}	
	
		if(pfront->next != NULL)
		{
			pnew->next = pplace;
			pfront->next = pnew;
		}
		else
		{
			pnew = pfront->next;
			pnew = NULL;
		}
	}
	
	else
	{
		p0->next = pnew;
		pnew->next = pplace;
	}

	return p0;
	
}

void delete(struct link *p0,int place2)
{
	int j;

	struct link *pplace2;
	struct link *pfront2;

	pplace2 = p0->next;

	if(place2 != 1)
	{
		for(j=1;j<place2;j++)
		{
			pfront2 = pplace2;
			pplace2 = pfront2->next;
		}
	
		pfront2->next = pplace2->next;
	}
	else
	{
		p0->next = pplace2->next;
	}
		free(pplace2);
	
}

void destroy(struct link *p0)
{
	struct link *ptmp;
		
	while(p0)
	{
		ptmp = p0->next;
		free(p0);
		p0=ptmp;
	}
}

void print(struct link *p0)
{
	struct link *p;
	p = p0->next;

	while(p!= NULL)
	{
		printf("%d ",p->num);
		p = p->next;

	}
}

int locate(struct link *p0,int e)
{
	int i=0;
	while(p0 != NULL)
	{
		if(p0->num == e) return i;
		else
		{
		 	p0 = p0->next;
			i = i+1;
		}
	}
}
int main()
{
	struct link *p0;
	p0 = creat();

	if(p0)	print(p0);
	else printf("插入失败\n");
/***********************************************/
	int length;
	length = linklength(p0);
	printf("\n节点长度为：%d",length);
/***********************************************/
	int place,newnum;
	printf("\n插入位置与插入数值：");
	scanf("%d%d",&place,&newnum);
	p0 = insert(p0,place,newnum);

	if(p0)	print(p0);
	else printf("插入失败\n");

	length = linklength(p0);
	printf("\n节点长度为：%d",length);
/***********************************************/
	printf("\n删除节点位置：");
	int place2;
	scanf("%d",&place2);
	delete(p0,place2);

	if(p0)	print(p0);
	else printf("插入失败\n");

	length = linklength(p0);
	printf("\n节点长度为：%d\n",length);
/***********************************************/
	printf("查找数值：");
	int e;
	scanf("%d",&e);
	printf("%d",locate(p0,e));
/***********************************************/
	destroy(p0);
}
