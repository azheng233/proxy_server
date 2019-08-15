#include "rpc_user_info.h"
#include "url_policy.h"
#ifdef _USE_SWHASH
#include "swhash.h"
#include "policy_cache.h"
#else
#include "hash_table.h"
#endif
#include "log.h"
#include "hexstr.h"
#include <jansson.h>
#include <string.h>
#include <time.h>

static int gen_user_url_policy(struct user_policy *puser, json_t **result)
{
    struct tm tm;
    char update_time[32];
    const char *policy_type;
    json_t *urls;
    struct url_policy *purl;

    if (!puser) {
        log_debug("gen url policy failed, user policy null");
        return -1;
    }

    localtime_r(&(puser->update_time), &tm);
    strftime(update_time, 32, "%Y-%m-%d %H:%M:%S", &tm);

    if (puser->type == PLCY_WHITELIST) {
        policy_type = "whitelist";
    }
    if (puser->type == PLCY_BLACKLIST) {
        policy_type = "blacklist";
    }

    urls = json_array();
    purl = puser->policy;
    while (purl) {
        if (purl->url_len) {
            json_array_append_new(urls, json_string(purl->url));
        }
        purl = purl->next;
    }

    *result = json_pack("{s:s, s:s, s:o}",
        "plolicy_type", policy_type,
        "update_time", update_time,
        "urls", urls);
    if (!*result) {
        log_error("pack user url policy urls failed");
        json_decref(urls);
        return -1;
    }

    return 0;
}

static int pack_user_policy(struct policy_node *pnode, json_t **policy)
{
    int ret;
    json_t *userinfo, *whitelist, *blacklist;

    if (!pnode || !policy) {
        log_error("pack user policy failed, params null");
        return -1;
    }

    log_info("pack user url policy, sn: %s, info: %s, forbidden: %d", pnode->snstr, pnode->userinfo, pnode->isforbidden);
    userinfo = json_pack("{s:s, s:s, s:b}",
        "sn", pnode->snstr,
        "info", pnode->userinfo,
        "forbidden", pnode->isforbidden);
    if (!userinfo) {
        log_error("rpc gen user url policy failed, sn: %s", pnode->snstr);
        return -1;
    }

    if (pnode->whitelist) {
        ret = gen_user_url_policy(pnode->whitelist, &whitelist);
        if (ret) {
            log_warn("rpc gen user url whitelist policy failed, sn: %s, err: %d", pnode->snstr, ret);
            goto erru;
        }
    } else {
        log_debug("pack user url whitelist policy null, sn: %s", pnode->snstr);
        whitelist = json_object();
    }

    if (pnode->blacklist) {
        ret = gen_user_url_policy(pnode->blacklist, &blacklist);
        if (ret) {
            log_warn("rpc gen user url blacklist policy failed, sn: %s, err: %d", pnode->snstr, ret);
            goto errw;
        }
    } else {
        log_debug("pack user url blacklist policy null, sn: %s", pnode->snstr);
        blacklist = json_object();
    }

    *policy = json_pack("{s:o, s:o, s:o}",
        "userinfo", userinfo,
        "whitelist", whitelist,
        "blacklist", blacklist);
    if (!*policy) {
        log_error("rpc gen user url policy failed, sn: %s", pnode->snstr);
        goto errb;
    }

    return 0;

errb:
    json_decref(blacklist);
errw:
    json_decref(whitelist);
erru:
    json_decref(userinfo);
    return -1;
}

static void pack_user_policy_in_array(void *node, void *result)
{
    int ret;
    json_t *policy;
    json_t *policys;
    struct policy_node *pnode;

    if (!node || !result) {
        log_error("pack user policy in array failed, params null");
        return;
    }

    pnode = (struct policy_node *)node;
    ret = pack_user_policy(pnode, &policy);
    if (ret) {
        log_warn("pack user policy failed, sn: %s, err: %d", pnode->snstr, ret);
        return;
    }

    policys = (json_t *)result;
    json_array_append_new(policys, policy);
}

int rpc_get_all_users_url_policy(json_t *params, json_t **result)
{
    if (!result) {
        log_error("rpc get all users url policy failed, param result null");
        return -1;
    }
    log_info("rpc get all users url policy");

    *result = json_array();
    if (!*result) {
        log_error("rpc get all users url policy gen result array failed");
        return -1;
    }

#ifdef _USE_SWHASH
    sw_hash_visit(g_plcy_hasht, pack_user_policy_in_array, (void *)(*result));
#else
    hash_visit(pack_user_policy_in_array, (void *)(*result));
#endif

    return 0;
}

int rpc_get_user_url_policy(json_t *params, json_t **result)
{
    int ret;
    const char *snstr;
    uint8_t sn[SN_MAX_LEN];
    int snlen = SN_MAX_LEN;
    struct policy_node *pnode;

    snstr = json_string_value(json_object_get(params, "sn"));
    if (!snstr) {
        log_error("rpc get user url policy failed, sn null");
        return -1;
    }
    log_info("rpc get user url policy, sn: %s", snstr);

    ret = str2hex(snstr, sn, &snlen);
    if (ret) {
        log_error("rpc get user url policy failed, sn conv error, sn: %s", snstr);
        return -1;
    }

    pnode = policy_node_find_from_hash_bysn(sn, snlen);
    if (!pnode) {
        log_warn("rpc get user url policy failed, no user policy found, sn: %s", snstr);
        *result = json_object();
        return 0;
    }

    ret = pack_user_policy(pnode, result);
    if (ret) {
        log_error("rpc gen user url policy failed, sn: %s", snstr);
        return -1;
    }

    return 0;
}