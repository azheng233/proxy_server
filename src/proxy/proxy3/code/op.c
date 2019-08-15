#include "http.h"

int main()
{
	char *p, *url;
	p = malloc(100);
	url = malloc(100);
	sprintf(p, "*://*.baidu.com/\n");
	sprintf(url, "http://tieba.baidu.com:443/index.html\n");

	if (*p == '*') {
		p++;
		while(*url != '/' && *url != '.' && *url != ':' &&\
				*url != '?' && *url != '#' && *url != '&')
			url++;
	}
	while (*p == *url) {
		printf("%c    %c\n", *p, *url);
		url++;
		p++;
		if (*p == '*') {
			p++;
			while(*url != '/' && *url != '.' && *url != ':' &&\
					*url != '?' && *url != '#' && *url != '&')
				url++;
		}
		if (*p == '\n') {
			return 0;
		}
		if (*url == ':' && *p == '/') {
			while (*url != '/')
				url++;
		}
	}

	printf("*p:%c *url%c\n", *p, *url);
	return 1;
}
