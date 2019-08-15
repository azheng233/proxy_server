#ifndef _CONN_PEER_H_
#define _CONN_PEER_H_

#include "connection.h"

extern connhead_t connhead;

int conn_signal_peerclose( connection_t *conn );

int conn_signal_thisclose( connection_t *conn );

int conn_handle_peerclose( connection_t *conn );

int conn_handle_thisclose( connection_t *conn );

void connections_destroy();

#endif
