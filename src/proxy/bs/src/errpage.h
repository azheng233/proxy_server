#ifndef _ERRPAGE_H_
#define _ERRPAGE_H_

#include "chunk.h"

enum http_code {
    HTTP_200_OK,
    HTTP_200_ESTABLISHED,
    HTTP_400_BAD_REQ,
    HTTP_403_FORBIDDEN,
    HTTP_407_PROXY_AUTH_REQ,
    HTTP_504_GW_TIMEOUT,
};

int putmsg_connect_ok( chunk_t *ck );
int putmsg_connect_established( chunk_t *ck );
int putmsg_err( chunk_t *ck, enum http_code code, char *msg, char *allowlink );
int putmsg( chunk_t *ck, char *title, char *msg );

#endif
