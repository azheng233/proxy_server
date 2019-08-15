#include "user_entry.h"
#include "url_filter_data_type.h"
#include "policy_cache.h"
#include "url_policy.h"
#include "iprbtree.h"
#include "msg_policy.h"
#include "hexstr.h"
#include "log.h"
#include <string.h>

int policy_cache_userlogin(uint8_t *sn, uint8_t snlen, uint32_t ipport)
{
    int ret;
    char snstr[SN_MAX_LEN * 2 + 1];
    int slen = SN_MAX_LEN * 2;
    struct policy_node *pnode;

    if (!sn || (snlen <= 0) || (ipport <= 0)) {
        log_error("user login params error, snlen:%u, ipport:%u", snlen, ipport);
        return -1;
    }

    hex2str(sn, snlen, snstr, slen);
    pnode = find_policy_in_rbtree(ipport);
    if (pnode) {
        if (0 == memcmp(pnode->sn, sn, snlen)) {
            log_info("user already login, sn: %s, ipport: %u", snstr, ipport);
            if (!pnode->whitelist && !pnode->blacklist) {
                log_info("user policy is null, request policy for user sn: %s, ipport: %u", snstr, ipport);
                ret = plcy_request_one(sn, snlen);
                if (ret != 0) {
                    log_error("request policy for user sn: %s failed, ipport: %u, err: %d, will request at next login time", snstr, ipport, ret);
                }
            }
            return 0;
        } else {
            log_warn("different user login from same ipport, will apply to new user, sn: %s, new user sn: %s, ipport: %u", pnode->snstr, snstr, ipport);
            close_all_conns_same_ipport(ipport, pnode->snstr);
            delete_policy_from_rbtree(ipport);
            policy_node_remove_conn(pnode, ipport);
        }
    }

    pnode = policy_node_find_from_hash_bysn(sn, snlen);
    if (!pnode) {
        log_info("create new policy node for user sn: %s, ipport: %u", snstr, ipport);
        pnode = policy_node_create(sn, snlen);
        if (!pnode) {
            log_error("policy node create failed, sn: %s, ipport: %u", snstr, ipport);
            return -1;
        }

        ret = policy_node_insert_to_hash(pnode);
        if (ret) {
            log_error("insert policy node to hash table failed, sn: %s, ipport: %u", snstr, ipport);
            return -1;
        }
    } else {
        log_info("find policy node for user sn: %s, ipport: %u", snstr, ipport);
    }

    policy_node_add_conn(pnode, ipport);

    ret = insert_policy_to_rbtree(ipport, pnode);
    if (ret) {
        log_error("insert policy node to rbtree failed, sn: %s, ipport: %d", snstr, ipport);
        policy_node_find_then_destory(sn, snlen);
        return -1;
    }

    if (!pnode->whitelist && !pnode->blacklist) {
        log_info("user policy is null, request policy for user sn: %s, ipport: %u", snstr, ipport);
        ret = plcy_request_one(sn, snlen);
        if (ret != 0) {
            log_error("request policy for user sn: %s failed, ipport: %u, err: %d, will request at next login time", snstr, ipport, ret);
        }
    }

    return 0;
}

int policy_cache_userlogout(uint8_t *sn, uint8_t snlen, uint32_t ipport)
{
    struct policy_node *pnode;
    char snstr[SN_MAX_LEN * 2 + 1];
    int slen = SN_MAX_LEN * 2;

    hex2str(sn, snlen, snstr, slen);
    log_info("user policy release interration with connections, sn: %s, ipport: %u", snstr, ipport);

    delete_policy_from_rbtree(ipport);

    pnode = policy_node_find_from_hash_bysn(sn, snlen);
    if (pnode) {
        policy_node_remove_conn(pnode, ipport);
    }

    return 0;
}