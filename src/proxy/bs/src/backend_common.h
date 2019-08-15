#ifndef _BACKEND_COMMON_H_
#define _BACKEND_COMMON_H_

#include "connection.h"

#define URL_NAME_MAXLEN  1024
struct backend_host {
    char name[URL_NAME_MAXLEN];
    unsigned short len;
};

int backend_set_host( connection_t *conn, const char *name, unsigned short len );

void backend_del_host( connection_t *conn );

#endif
