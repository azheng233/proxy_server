#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main()
{
	int index = 0, len;
	char *command;
	unsigned  int hash = 5381;
	command = malloc(10);
	strcpy(command, "asdfgsdsw");
	command[10] = '\0';
        if (command == NULL) {
                return -1;
        }

        while (*command) {
                hash += (hash << 5) + (*command++);
        }
	printf("%d\n",(hash & 0x7FFFFFFF)%6);
       	return 0;
}

