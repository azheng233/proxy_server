#ifndef _URL_POLICY_H_
#define _URL_POLICY_H_

#include "url_filter_data_type.h"

struct policy_node *policy_node_create(uint8_t *sn, uint8_t snlen);
void policy_node_free(struct policy_node *pnode);
void policy_node_free_void(void *node);

int policy_node_add_conn(struct policy_node *pnode, uint32_t ipport);
void policy_node_remove_conn(struct policy_node *pnode, uint32_t ipport);
void policy_node_remove_all_conns(struct policy_node *pnode);


struct user_policy *user_policy_new();
void user_policy_del(struct user_policy *puser);
void user_policy_del_void(void *puser);


struct url_policy *url_policy_new(size_t url_len);
void url_policy_del(struct url_policy *purl);


struct time_slice *time_slice_new();
void time_slice_del(struct time_slice *ptime);


struct daytime *daytime_new();
void daytime_del(struct daytime *pday);


int time_slice_append(struct daytime *pday, struct time_slice *ptime);
int daytime_append(struct url_policy *purl, struct daytime *pday);
int url_policy_append(struct user_policy *puser, struct url_policy *purl);

int user_policy_update(struct user_policy *pold, struct user_policy *pnew);

void url_judge_set_method(const char *method);

int policy_judge(struct policy_node *pnode, char *url, size_t len);

int is_whitelist_policy_allow(struct policy_node *pnode, struct url_policy *purl);

#endif
