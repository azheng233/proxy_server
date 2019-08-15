#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main ()
{
	char *a;
	a = (char *)malloc(5);
	strcpy(a,"abc");
	a[1] = '\0';
	strcat(a, "dfg");
	
	printf("%s\n",a);
	
	return 0;
}
