# include "http.h"

int main()
{
	int num, n;
	FILE *fp;
	char buf[20] = "abcde\nfgijk";
	fp = fopen("ace", "w");
	
	if (fp == NULL) {
		printf("fopen fail\n");
		exit(0);
	}
	
	num = strlen(buf);
	n = fwrite(buf, 1, num, fp);

	printf("%d\n", n);
	
	fclose(fp);
}
