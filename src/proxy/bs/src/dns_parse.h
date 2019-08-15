#ifndef _DNS_PARSE_H_
#define _DNS_PARSE_H_

#include "connection.h"

#define DNS_OK  0
#define DNS_ERR  -1
#define DNS_NOTFOUND  -2

#define DNS_ECHO_LEN  8

struct dns_readbuf_context {
    unsigned char tmp[DNS_ECHO_LEN];
    unsigned char *tmpos;
    unsigned char intmp;
};

int dns_parse_echo( connection_t *conn );


int dns_tree_init();

int dns_query( connection_t *conn, const char *name, uint16_t namelen );

void dns_delconn( connection_t *conn );


#endif
