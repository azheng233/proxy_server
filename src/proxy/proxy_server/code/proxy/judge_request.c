#include "http.h"
#include "sort_request.h"
#include "recvline.h"

char *getip(char *dns, char *target_ip)
{
        char *ptr, **pptr;
        struct hostent *hptr;

        ptr = malloc(100);
        sprintf(ptr, dns);
        hptr = gethostbyname(ptr);
        if (hptr == NULL) {
                printf("getname error: %s: %s\n", ptr, hstrerror(h_errno));
                return NULL;
        }

//        printf("official hostname: %s\n", hptr->h_name);

        for (pptr = hptr->h_aliases; *pptr != NULL; pptr++) {
//                printf("\talias: %s\n", *pptr);
        }

        switch (hptr->h_addrtype) {

        case AF_INET:
                pptr = hptr->h_addr_list;
		inet_ntop(hptr->h_addrtype, *pptr, target_ip, 46);
//		printf("\taddress: %s\n", target_ip);
                break;
        default:
                printf("unknown address type\n");
		target_ip = NULL;
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

	n = find_word(line1->host, ":");
	dns = malloc(100);
	if (n) {
		err = snprintf(dns, n, line1->host);
		if (err < 0) {
			free(dns);
			dns = NULL;
			return 500;
		}
		
		err = sprintf(Target_port, line1->host + n);
	} else {
		err = sprintf(dns, line1->host);
		if (err <= 0) {
			return -1;
		}
		
		err = sprintf(Target_port, "80");
		if (err <= 0) {
			return -1;
		}
	}
	
	Target_ip = getip(dns, Target_ip);
	if (Target_ip == NULL) 
		return 105;
	
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
                        printf("client request keep alive.\n");
                else {
                        printf("client request shork connection.\n");
			n = 1;
		}
        } else {
                printf("No connection was found.\n\n");
		n = 1;
        }

	return n;
}
