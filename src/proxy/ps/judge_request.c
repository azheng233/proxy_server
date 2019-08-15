#include "https.h"
#include "sort_request.h"
#include "recvline.h"

char *getip(char *dns, char *target_ip)
{
        char *ptr, **pptr;
        struct hostent *hptr;

        ptr = malloc(100);
        sprintf(ptr, "%s", dns);
        hptr = gethostbyname(ptr);
        if (hptr == NULL) {
                fprintf(stderr, "getname error: %s: %s\n", ptr, hstrerror(h_errno));
		target_ip[0] = '\0';
		free(ptr);
		ptr = NULL;
                return target_ip;
        }

/*      fprintf(stderr, "official hostname: %s\n", hptr->h_name);

        for (pptr = hptr->h_aliases; *pptr != NULL; pptr++) {
                fprintf(stderr, "\talias: %s\n", *pptr);
        }
*/
        switch (hptr->h_addrtype) {

        case AF_INET:
                pptr = hptr->h_addr_list;
		inet_ntop(hptr->h_addrtype, *pptr, target_ip, 46);
//		fprintf(stderr, "\taddress: %s\n", target_ip);
                break;
        default:
                fprintf(stderr, "unknown address type\n");
		target_ip;
                break;
        }
	
	free(ptr);
	ptr = NULL;

        return target_ip;
}

int judge_line1(start *line1, char **target_ip, char **target_port)
{
	int n, err = 0;
	char *dns;
	char *Target_ip, *Target_port;

	Target_ip = *target_ip;
	Target_port = *target_port;

	dns = malloc(100);
	n = find_word(line1->host, ":");
	if (n) {
		err = snprintf(dns, n, "%s", line1->host);
		if (err < 0) {
			free(dns);
			dns = NULL;
			return 500;
		}
		
		err = sprintf(Target_port, "%s", line1->host + n);
	} else {
		err = sprintf(dns, "%s", line1->host);
		if (err <= 0) {
			return -1;
		}
		
		err = sprintf(Target_port, "80");
		if (err <= 0) {
			return -1;
		}
	}
	
	Target_ip = getip(dns, Target_ip);
	if (Target_ip[0] == '\0') {
		free(dns);
		dns = NULL;
		return 105;
	}

	free(dns);
	dns = NULL;
	return 0;
}

int judge_conn_method(table *hash)
{
	first *conn_method;
	int n;

	conn_method = find_hash(hash, "Connection");    
        if (conn_method == NULL)
                conn_method = find_hash(hash, "Proxy-Connection");

        if (conn_method) {
                n = strcmp(conn_method->value, "keep-alive");
                if (n == 0)
                        fprintf(stderr, "client request keep alive.\n");
                else {
                        fprintf(stderr, "client request shork connection.\n");
			n = 1;
		}
        } else {
                fprintf(stderr, "No connection was found.\n\n");
		n = 1;
        }

	return n;
}

char *get_http_url(start *line1)
{
	char *url;
	int pos;

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
	struct link *data;
	char *url;
	char s[2];

	s[1] = '\0';
	s[0] = ' ';
	url = malloc(4096);
	if (url == NULL) {
		return NULL;
	}
	data = resolve(ssl_line1, s);
	data = data->next;
	sprintf(url, "https://%s%s", line1->host, data->str);

fprintf(stderr, "url: %s\n", url);

	destory(data);
	return url;
}

static int contrast(char *p, char *url)
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
		if (*(p+1) == '\n')
			return 1;
	}
	return 0;
	
}

static int lookup_whitelist(struct link *whitelist, char *url)
{
	int sym;
	struct link *p;

	p = whitelist->next;
	while (p) {
		sym = contrast(p->str, url);

		if (sym)
			return 1;
		p = p->next;
	}
	
	return 0;
}

int judge_url(start *line1, struct link *whitelist, char *ssl_line1)
{
	char *url;
	int i;
	
	if (!whitelist) {
		fprintf(stderr, "No find whitelist\n");
		return 1;
	}
	
	if (ssl_line1) {
		url = get_https_url(line1, ssl_line1);
	} else {
		url = get_http_url(line1);
	}
	if (url == NULL)
		return 0;
	i = lookup_whitelist(whitelist, url);
	free(url);
	url = NULL;

	return i;
}

