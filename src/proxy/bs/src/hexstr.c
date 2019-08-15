#include "hexstr.h"
#include <stdio.h>

int str2hex(const char *str, unsigned char *hex, int *hlen)
{
    int j;
    int len = *hlen;
    const char *s;
    unsigned char x[2];
    for (*hlen = 0; *hlen < len; (*hlen)++) {
        s = str + 2 * (*hlen);
        for (j = 0; j < 2; j++) {
            if (s[j] == 0) return 0;
            else if (s[j] < '0') return -1;
            else if (s[j] <= '9') x[j] = s[j] - '0';
            else if (s[j] < 'A') return -1;
            else if (s[j] <= 'F') x[j] = s[j] - 55;
            else if (s[j] < 'a') return -1;
            else if (s[j] <= 'f') x[j] = s[j] - 87;
            else return -1;
        }
        hex[*hlen] = x[0] * 16 + x[1];
    }
    return 0;
}

int hex2str(const unsigned char *hex, int len, char *str, int slen)
{
    int i;
    char *pos = str;

    if (slen < 2 * len)
        return -1;

    pos[0] = 0;
    for (i = 0; i < len; i++)
        pos += sprintf(pos, "%02x", hex[i]);

    return 0;
}