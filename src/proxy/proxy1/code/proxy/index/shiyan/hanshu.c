#include "http.h"

void fun1(char *a)
{
	char b;
	b = *a;
	sprintf(b, "aaaaaaaaaa\n");
}

int main()
{
	char a[100];
	fun1(&a);		
	printf("%s", a);
	return 0;
}
