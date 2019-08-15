#include "http.h"
static int a = 0;

void fun()
{
	a = 10;
}

void fun1()
{
	a = a+10;
}

int main()
{
	fun();
	printf("%d\n", a);
	fun1();
	printf("%d\n", a);
	return 0;
}
