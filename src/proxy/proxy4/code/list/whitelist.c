#include "https.h"
//#include "sort_request.h"
#include "recvline.h"
/*
char *get_url()
{
	char *url;
	int n;

	n = strlen(line1->host) + strlen(line1->resource);
	url = malloc(n + 50);

	if (find_word(line1->host, ":")) {
		if (strcmp(line1->method, "CONNECT"))
			n = sprintf(url, "http://%s%s", line1->host, line1->resource);
		else 
			n = sprintf(url, "https://%s%s", line1->host, line1->resource);
	} else {
		if (strcmp(line1->method, "CONNECT")) 
			n = sprintf(url, "http://%s:80%s", line1->host, line1->resource);
		else 
			n = sprintf(url, "https://%s:443%s", line1->host, line1->resource);
	}
	fprintf(stderr, "%s\n", url);

	return url;
}
*/
int main()
{
	struct link *whitelist, *p1, *p2;
	struct stat failinfo;
	int err, len, n, return_pos;
	FILE *fd;
	char *buf;
	whitelist = create();


	err = stat("config", &failinfo);
	if (err) {
		return -1;
	}
	buf = malloc(failinfo.st_size + 1);

	fd = fopen("config", "r");
	if (fd == NULL) {
		fprintf(stderr, "白名单加载失败");
		return -1;
	}

	len = fread(buf, 1, failinfo.st_size, fd);
	if (ferror(fd)) {
		fprintf(stderr, "fread error\n");
		return -1;
	}
	buf[len] = '\0';

	while (1) {
		p1 = create();

		return_pos = find_word(buf, "\n");
		if (return_pos) {
			strncpy(p1->str, buf + n, return_pos);
			p1->str[return_pos] = '\0';

			if (whitelist->next == NULL) 
				whitelist->next = p1;
			else
				p2->next = p1;

			p2 = p1;
			n += return_pos;
		} else {
			strcpy(p1->str, buf);
			p1->str[len] = '\0';
			if (whitelist->next == NULL) 
				whitelist->next = p1;
			else
				p2->next = p1;
			break;
			p2 = p1;
		}

		if (len == n) 
			break;
	}		

	p2->next = NULL;	
	print(whitelist);
	return 0;
}
