#include <stdio.h>

void a(int b, int c) 
{
	c = b;
	return;
}

int main ()
{
	int c, b = 1;
	a(b, c);
	printf("%d,%d\n", b, c);

	return 0;
}
