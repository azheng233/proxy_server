#ifndef _RPC_USER_INFO_H_
#define _RPC_USER_INFO_H_

#include "jsonrpc.h"

int rpc_get_all_users_url_policy(json_t *params, json_t **result);

int rpc_get_user_url_policy(json_t *params, json_t **result);

#endif
