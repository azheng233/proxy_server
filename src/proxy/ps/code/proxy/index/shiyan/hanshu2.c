#include "http.h"

static char c[3];

int fun(int a)
{
	char b[3] = "f";
	if (a == 0) {
		sprintf(c, b);
		return 0;
	}
	if (a == 1)
		return 1;

}
