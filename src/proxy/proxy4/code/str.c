#include "https.h"

int find_word (char *buf, char *word)
{
        int i = 0;

        while (buf[i] != word[0]) {
                if (buf[i] == '\0') {
                        return 0;
                }

                i++;
        }
        return i+1;
}

char *wstrcpy(char *b, char *c) 
{
	int i;
	for (i = 0; b[i] != '\0'; i++)
		c[i] = b[i];

	c[i] = '\0';

	return c;
}

int main()
{
	char *a = "aaaaabbbcdcccc";
	char b[20] = "aaabbbccc";
	char c[20];

	*c = wstrcpy(c, b);
	printf("%s\n", c);

	int pos;
	pos = find_word(a+4, "d");

	printf("%d\n", pos);

	return 0;
}
