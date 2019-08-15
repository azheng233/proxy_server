#ifndef _HEXSTR_H_
#define _HEXSTR_H_

int str2hex(const char *str, unsigned char *hex, int *hlen);

int hex2str(const unsigned char *hex, int len, char *str, int slen);

#endif
