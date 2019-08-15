#ifndef _MSG_RPC_H_
#define _MSG_RPC_H_

#include "connection.h"

int rpc_msg_reslove( connection_t *c );
int rpc_msg_parse( connection_t *c );

#endif
