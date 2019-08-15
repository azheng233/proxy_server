#ifndef _CONN_DNS_H_
#define _CONN_DNS_H_

#define DNS_SOCK_PATH  "/tmp/dns_server_socket"

int dns_conn_create();

void dns_conn_close();

#endif
