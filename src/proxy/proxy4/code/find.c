#include "http.h"

void find_word(char *a, char *b)
{
	int i = 0;
	
	while(a[i] != b[0]) {
		if (a[i] == '\0') 
			break;

		i++;
	}
	
	printf("%d\n", i);
}

int main()
{
	char *a;
	a = malloc(10);
	
	sprintf(a, "absdfgrku");
	
	find_word(a,"d");
	
	return 1;
}
