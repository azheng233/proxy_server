#ifndef _USER_ENTRY_H_
#define _USER_ENTRY_H_

#include <stdint.h>

int policy_cache_userlogin(uint8_t *sn, uint8_t snlen, uint32_t ipport);

int policy_cache_userlogout(uint8_t *sn, uint8_t snlen, uint32_t ipport);

#endif