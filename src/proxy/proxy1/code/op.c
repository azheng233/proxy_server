#include "http.h"

char *aaa()
{
	char s[] = "aaaaaa";
	return s;
}

int main()
{
	char *s;

	*s = aaa();
	printf("%s\n", s);

	return 0;
}
