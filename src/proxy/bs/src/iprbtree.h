#ifndef __IPRBTREE_H__
#define __IPRBTREE_H__

#include "rbtree.h"
#include "url_filter_data_type.h"
#include <stdint.h>

//*********note*********
//the return value -1 means error, 0 means ok;
//if the functions that the return value type is a point, NULL means error or not find.

int init_rbtree();

int insert_policy_to_rbtree(uint32_t ipport, struct policy_node *pnode);

int delete_policy_from_rbtree(uint32_t ipport);

struct policy_node *find_policy_in_rbtree(uint32_t ipport);

void mid_visit_rbtree();

void mid_destroy_rbtree(rbtree_ex_func free_data);

#endif
