#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>


int main ()
{

	FILE *stream;
        char buff_one[1024];
        int n, len = 0, i = 0;

	struct stat a;
	stat("t.txt", &a);
	printf("a:%d\n", a.st_size);

        if ((stream = fopen("t.txt", "r")) == NULL) {
                fprintf(stderr,"Can not open output file.\n");
                return 0;
        }
               // fseek(stream, i, SEEK_SET);
        while(1) {
		memset(buff_one, 0, 1025);
        	n = fread(buff_one, 1, 10, stream);

		buff_one[n] = '\0';
		len += n;

       		printf("%s     ",buff_one);
       		printf("n:%d\n",n);
		if (n == 0) 
			break;
	}
        fclose(stream);
}
