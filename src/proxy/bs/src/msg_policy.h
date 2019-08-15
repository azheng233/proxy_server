#ifndef _MSG_POLICY_H_
#define _MSG_POLICY_H_

#include "connection.h"

int policy_parse_data(connection_t *c);

void close_all_conns_same_ipport(uint32_t ipport, char *snstr);

int plcy_request_all();

int plcy_request_one(uint8_t *sn, uint8_t snlen);

#endif
