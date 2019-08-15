#ifndef _RPC_H_
#define _RPC_H_

#include "connection.h"

int rpc_handle(connection_t *c, uint8_t *msg, int len);

#endif
