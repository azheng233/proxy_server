#include "policy_cache.h"
#include "url_filter_data_type.h"
#include "url_policy.h"
#ifdef _USE_SWHASH
#include "swhash.h"
#else
#include "hash_table.h"
#endif
#include "iprbtree.h"
#include "conn_gateway.h"
#include "log.h"
#include "hexstr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _USE_SWHASH
#include "swhash.h"
hash_t g_plcy_hasht = NULL;
#endif

int policy_cache_create(unsigned int num)
{
    if (num <= 0) {
        num = POLICY_CACHE_DEFAULT_SIZE;
    }

#ifdef _USE_SWHASH
    g_plcy_hasht = sw_hash_create(num, 0, BOB_HASH, NULL, NULL, policy_node_free_void, 0);
    if (!g_plcy_hasht) {
        return -1;
    }
#else
    if (0 != hash_create(num)) {
        return -1;
    }
#endif

    init_rbtree();
    return 0;
}

void policy_cache_destroy()
{
#ifdef _USE_SWHASH
    sw_hash_destroy(g_plcy_hasht, 1);
#else
	hash_destroy(policy_node_del_void);
#endif

	mid_destroy_rbtree(NULL);
}

int policy_node_insert_to_hash(struct policy_node *pnode)
{
    int ret;

    if (!pnode) {
        log_error("policy node insert to hash table failed, params error");
        return -1;
    }

#ifdef _USE_SWHASH
    ret = sw_hash_insert(g_plcy_hasht, pnode->sn, pnode->snlen, (void *)pnode);
#else
    ret = hash_add(pnode->sn, pnode->snlen, (void *)pnode);
#endif
    if (ret) {
        log_error("insert policy node to hash table failed, sn: %s, err: %d", pnode->sn, ret);
        return -1;
    }

    return 0;
}

void policy_node_find_then_destory(uint8_t *sn, uint8_t snlen)
{
    struct policy_node *pnode;
    struct conninfo *conn;

    if (!sn || (snlen <= 0)) {
        return;
    }

    pnode = policy_node_find_from_hash_bysn(sn, snlen);
    if (!pnode) {
        return;
    }

#ifdef _USE_SWHASH
    sw_hash_delete(g_plcy_hasht, sn, snlen, 0);
#else
    hash_del(sn, snlen);
#endif

    conn = pnode->conns;
    while (conn) {
        delete_policy_from_rbtree(conn->ipport);
        conn = conn->next;
    }

    policy_node_free(pnode);
}

struct policy_node *policy_node_find_from_hash_bysn(uint8_t *sn, uint8_t snlen)
{
    char snstr[SN_MAX_LEN * 2 + 1];
    int slen = SN_MAX_LEN * 2;
    struct policy_node *pnode;

    if (!sn || (snlen <= 0)) {
        return NULL;
    }

#ifdef _USE_SWHASH
    pnode = (struct policy_node *)sw_hash_lookup(g_plcy_hasht, sn, snlen);
#else
    pnode = (struct policy_node *)hash_search(sn, snlen);
#endif
    if (!pnode) {
        hex2str(sn, snlen, snstr, slen);
        log_debug("find policy node from hash failed, sn: %s", snstr);
        return NULL;
    }

    return pnode;
}

struct policy_node *policy_node_find_from_rbtree_byconn(connection_t *conn)
{
    uint8_t buf[32];
    uint8_t *ipport;
    char ipstr[32];
    struct policy_node *pnode;

    ipport = buf;
    *(uint32_t*)ipport = ((struct sockaddr_in*)conn->addr)->sin_addr.s_addr;
#ifndef WITH_POLICY_TEST
    if (gateway_ipcmp(ipport) == 0) {
        *(uint16_t *)ipport = 0;
        *(uint16_t *)(ipport + 2) = ((struct sockaddr_in*)conn->addr)->sin_port;
        sprintf(ipstr, "port: %hu", ntohs(*(uint16_t*)(ipport + 2)));
    } else {
        sprintf(ipstr, "ip: %hhu.%hhu.%hhu.%hhu", ipport[0], ipport[1], ipport[2], ipport[3]);
    }
#else
    sprintf(ipstr, "ip %hhu.%hhu.%hhu.%hhu", ipport[0], ipport[1], ipport[2], ipport[3]);
#endif

    pnode = find_policy_in_rbtree(*(uint32_t*)ipport);
    log_debug("find policy node by connection fd: %d %s, %s", conn->fd, pnode ? "success" : "failed", ipstr);

    return pnode;
}

static void user_policy_forbidden(void *node, void *params)
{
    struct policy_node *pnode = (struct policy_node *)node;

    if (pnode) {
        pnode->isforbidden = 1;
    }
}

int all_user_policy_forbidden()
{
#ifdef _USE_SWHASH
    sw_hash_visit(g_plcy_hasht, user_policy_forbidden, NULL);
    return 0;
#else
    if (0 == hash_visit(user_policy_forbidden, NULL))
        return 0;
    return -1;
#endif
}

static void user_policy_enable(void *node, void *params)
{
    struct policy_node *pnode = (struct policy_node *)node;

    if (pnode) {
        pnode->isforbidden = 0;
    }
}

int all_user_policy_enable()
{
#ifdef _USE_SWHASH
    sw_hash_visit(g_plcy_hasht, user_policy_enable, NULL);
    return 0;
#else
    if (0 == hash_visit(user_policy_enable, NULL))
        return 0;
    return -1;
#endif
}