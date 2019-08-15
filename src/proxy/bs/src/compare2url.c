#include "compare2url.h"
#include "log.h"
#include <string.h>
#include <stdio.h>

int is2urlsame_strcmp(char *purl, size_t url_len, char *purl_target, size_t url_target_len)
{
    if ((NULL == purl) || (NULL == purl_target))
    {
        return 0;
    }
    if (0 == strcmp(purl, purl_target))
        return 1;
    return 0;
}

int is2urlsame_slash(char *purl, size_t url_len, char *purl_target, size_t url_target_len)
{
    size_t lastpos = 0;

    if (!purl || !purl_target) {
        return 0;
    }

    log_trace("policy url: %s, len: %zu, target url: %s, len: %zu", purl, url_len, purl_target, url_target_len);
    // 去掉访问url最后的/，当策略结尾不为/时允许
    if (purl_target[url_target_len - 1] == '/') {
        url_target_len--;
    }

    if (url_target_len < url_len - 1) {
        return 0;
    }

    lastpos = 0;
    while (purl[lastpos] == purl_target[lastpos]) {
        lastpos++;
        if (url_len - 1 == lastpos) {
            break;
        }
    }

    if (lastpos < url_len - 1) {
        return 0;
    }

    if (purl[lastpos] == '/') {
        if (url_target_len == url_len - 1) {
            return 1;
        }
        if (purl_target[lastpos] == '/') {
            return 1;
        }
        return 0;
    }

    if (url_target_len == url_len - 1) {
        return 0;
    }

    if (purl[lastpos] == purl_target[lastpos]) {
        if (url_target_len == url_len) {
            return 1;
        }

        if ((purl_target[lastpos + 1] == '?') || (purl_target[lastpos + 1] == '#')) {
            return 1;
        }

        return 0;
    }

    return 0;
}

int is2urlsame_regex(char *purl, size_t url_len, char *purl_target, size_t url_target_len)
{
    return 0;
}