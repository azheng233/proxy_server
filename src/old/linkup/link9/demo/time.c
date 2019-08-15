#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int main ()
{
	time_t rawtime; 
	struct tm * timeinfo; 
	char message[4096], a[10];
	time ( &rawtime ); 
	timeinfo = localtime ( &rawtime ); 
	int n, len;
	printf ( "%s\n", asctime (timeinfo)); 

        n = sprintf(message, "HTTP/1.1 200 OK\r\nServer: weizheng\r\nDate: ");
        len = n;
        n = sprintf(message + len, asctime(timeinfo));
        len += n;
	--len;
        n = sprintf(message + len, "\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: ");
        len += n;
        n = sprintf(message + len, "%d", len);
	len += n;
        sprintf(message + len, "\r\nConnection: keep-alive\r\n\r\n");

	printf("%sssss",message);

	return 0;
}
