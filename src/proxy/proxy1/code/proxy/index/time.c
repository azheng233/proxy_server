#include "http.h"

char *main()
{
        time_t t;
        struct tm *gmt;
        char *timeinfo;
        size_t n;

	timeinfo = malloc(100);

        t = time(NULL);
        gmt = gmtime(&t);

        n = strftime(timeinfo, 100, "Date: %a, %d %b %Y  %X GMT\r\n", gmt);
        if (n <= 0) {
                timeinfo[0] = '\0';
        } else {
                timeinfo[n] = '\0';
        }   
	printf("%s", timeinfo);
	return timeinfo;
}
