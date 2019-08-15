#ifndef __SSL_FORWARD_H_
#define __SSL_FORWARD_H_

int SSL_forward(start *line1, SSL *server_ssl, SSL *client_ssl, struct link *whitelist);

#endif
