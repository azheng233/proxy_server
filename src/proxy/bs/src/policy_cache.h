#ifndef _POLICY_CACHE_H_
#define _POLICY_CACHE_H_

#include "url_filter_data_type.h"
#include "connection.h"

#ifdef _USE_SWHASH
#include "swhash.h"
extern hash_t g_plcy_hasht;
#define POLICY_CACHE_DEFAULT_SIZE  1024*1024
#else
#define POLICY_CACHE_DEFAULT_SIZE  1024*4
#endif

int policy_cache_create(unsigned int num);

void policy_cache_destroy();

int policy_node_insert_to_hash(struct policy_node *pnode);

void policy_node_find_then_destory(uint8_t *sn, uint8_t snlen);

struct policy_node *policy_node_find_from_hash_bysn(uint8_t *sn, uint8_t snlen);

struct policy_node *policy_node_find_from_rbtree_byconn(connection_t *conn);

int all_user_policy_forbidden();

int all_user_policy_enable();

#endif
