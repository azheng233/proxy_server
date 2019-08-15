#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct a {
	char b[10];
	char c[10];
	struct a *next;
};

struct d {
	struct a *e[10];
};
int main () 
{
	struct d *s;
	struct a *x;
	
	s = malloc(sizeof(struct d));
	x = malloc(sizeof(struct a));
printf("2");
	s->e[1] = x;
printf("3");
	strcpy(x->b, "aaaaaaaaa");
printf("4");
//	x->b[10] = '\0';
	
	printf("%s",x->b);
	return 1;
}
