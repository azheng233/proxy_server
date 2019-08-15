#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
int main () 
{
	FILE *p;
	char buff_one[4096];
	int i=0;
	
	p = fopen("aaa/a.txt", "r");
	if (p == NULL) {
		printf("aaa\n");
	}
	fread(buff_one, sizeof(buff_one),1,p);
	printf("%s", buff_one);
	
	fclose(p);
	return 0;
}
