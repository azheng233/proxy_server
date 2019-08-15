#include "http.h"

int main() 
{
	char *ptr, **pptr;
	char str[46];
	struct hostent *hptr;
printf("size: %d\n", sizeof(str));
	ptr = malloc(100);
	sprintf(ptr, "www.sina.com.cn");
	hptr = gethostbyname(ptr);
	if (hptr == NULL) {
		printf("getname error: %s: %s\n", ptr, hstrerror(h_errno));
		return 0;
	}

	printf("official hostname: %s\n", hptr->h_name);
	
	for (pptr = hptr->h_aliases; *pptr != NULL; pptr++) {
		printf("\talias: %s\n", *pptr);
	}

	switch (hptr->h_addrtype) {
	case AF_INET:
		pptr = hptr->h_addr_list;
	//	for (; *pptr != NULL; pptr++) {
			inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));
			printf("\taddress: %s\n", str);
	//	}
		break;
	
	default:
		printf("unknown address type\n");
		break;
	}
		
	return 1;
}
