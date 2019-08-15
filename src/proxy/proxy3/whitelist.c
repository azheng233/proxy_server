#include "https.h"
#include "sort_request.h"
#include "recvline.h"

char *get_http_url(start *line1)
{
	char *url;

	url = malloc(4096);
	pos = find_word(line1->host, ":");
	if (pos)
		sprintf(url, "http://%s%s", line1->host, line1->resource);
	else
		sprintf(url, "http://%s:80%s", line1->host, line1->resource);

fprintf(stderr, "url: %s\n", url);
	return url;
}

char *get_https_url(start *line1, char *ssl_line1)
{
	char s[] = " ";
	struct link *data;

	data = resolve(data, &s);
	data = data->next
	sprintf(url, "https://%s%s", line1->host, data->str);

fprintf(stderr, "url: %s\n", url);

	return url;
}

struct link *load_whitelist()
{
	struct link *whitelist, *p1, *p2;
	struct stat failinfo;
	int err, len, return_pos;
	int n = 0;
	FILE *fd;
	char *buf, *tail = NULL;
	whitelist = create();


	err = stat("config", &failinfo);
	if (err) {
		return -1;
	}

	fprintf(stderr, "stat:%d\n", failinfo.st_size);
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
		if (tail) {
			sprintf(buf, "%s", tail);
			free(tail);
			tail = NULL;
		}
		return_pos = find_word(buf, "\n");
		if (return_pos) {
			strncpy(p1->str, buf, return_pos);
			p1->str[return_pos] = '\0';

			if (whitelist->next == NULL) 
				whitelist->next = p1;
			else
				p2->next = p1;

			p2 = p1;
			n += return_pos;
		} else {
			break;
		}

		if (len == n) 
			break;
		tail = malloc(len - n);
		sprintf(tail, "%s", buf + n);
	}		

	p2->next = NULL;	
	print(whitelist);
	return whitelist;
}
int contrast(struct link *p, char *url)
{
	if (*p == '*') {
		p++;
		while(*url != '/' && *url != '.' && *url != ':' &&\
				*url != '?' && *url != '#' && *url != '&')
			url++;
	}
	while (*p == *url) {
		url++;
		p++;
		if (*p == '*') {
			p++;
			while(*url != '/' && *url != '.' && *url != ':' &&\
					*url != '?' && *url != '#' && *url != '&')
				url++;
		}

		if (*p == '\n')
			return 1;
	}
	return 0;
	
}

int lookup_whitelist(struct link *whitelist, char *url)
{
	int sym;
	struct link *p;
	p = whitelist->next
	while (p) {
		sym = contrast(p, url);
		if (sym)
			return 1;
		p = p->next;
	}
	
	return 0;
}
