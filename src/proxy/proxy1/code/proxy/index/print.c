#include <stdio.h>

int main()
{
	printf("GET / HTTP/1.1\r\n");
	printf("HOST: www.baidu.com\r\n");
	printf("Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3\r\n");
	printf("Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n\r\n");
	return 1;
}
