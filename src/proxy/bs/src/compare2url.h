#ifndef _COMPARE2URL_H_
#define _COMPARE2URL_H_

#include "url_filter_data_type.h"

//************note****************
//purl_target is the point of a url string that to be compared,
//purl is the point of a url string that to be used to compare;
//the return value of these funcations will show that 
//is the url_target satisfied with the conditions of the url,
//if so, the value is 1, else 0.

int is2urlsame_strcmp(char *purl, size_t url_len, char *purl_target, size_t url_target_len);
int is2urlsame_slash(char *purl, size_t url_len, char *purl_target, size_t url_target_len);
int is2urlsame_regex(char *purl, size_t url_len, char *purl_target, size_t url_target_len);

#endif
