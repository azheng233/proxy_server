#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(void)
{
    	FILE *stream;
    	char buff_one[4096];
	int i = 0;

    	if ((stream = fopen("text.txt", "r")) == NULL) {
        	fprintf(stderr,"Can not open output file.\n");
        	return 0;
    	}
	while (1) {	
    		fseek(stream, i, SEEK_SET);
		fread(buff_one, 1, 1, stream);
		
		if (feof(stream) != 0) {
			break;
		}

		printf("%s", buff_one);
		i++;
    	}
	fclose(stream);
}
