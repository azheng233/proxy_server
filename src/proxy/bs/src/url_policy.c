#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "url_policy.h"
#include "url_filter_data_type.h"
#ifdef _USE_SWHASH
#include "swhash.h"
#include "policy_cache.h"
#else
#include "hash_table.h"  //for int hash_visit( hash_ex_func_ex set_flags, void *pflags ).
#endif
#include "compare2url.h"
#include "log.h"
#include "hexstr.h"

static int(*url_cmp)(char *purl, size_t url_len, char *purl_target, size_t url_target_len) = is2urlsame_slash;

void url_judge_set_method(const char *method)
{
    switch (method[0])
    {
    case 's': case 'S':
        switch (method[1])
        {
        case 't': case 'T':
            url_cmp = is2urlsame_strcmp;
            return;
        case 'l': case 'L':
            url_cmp = is2urlsame_slash;
            return;
        default:
            break;
        }
        break;
    case 'r': case 'R':
        //TODO change compare method
        url_cmp = is2urlsame_regex;
        return;
    default:
        break;
    }
    log_info("unknown url_cmp_method %s", method);
}

struct time_slice *time_slice_new()
{
    struct time_slice *ptime;

    ptime = (struct time_slice *)malloc(sizeof(struct time_slice));
    if (!ptime) {
        return NULL;
    }

    ptime->next = NULL;
    return ptime;
}

struct daytime *daytime_new()
{
    struct daytime *pday;

    pday = (struct daytime *)malloc(sizeof(struct daytime));
    if (!pday) {
        return NULL;
    }

    pday->count = 0;
    pday->slices = NULL;
    return pday;
}

struct url_policy *url_policy_new(size_t url_len)
{
    int i = 0;
    struct url_policy *purl;

    if (url_len <= 0) {
        return NULL;
    }

    purl = (struct url_policy *)malloc(sizeof(struct url_policy) + url_len + 1);
    if (!purl) {
        return NULL;
    }

    for (i = 0; i < 7; i++) {
        purl->daytimes[i] = NULL;
    }

    purl->next = NULL;
    purl->url_len = 0;
    purl->url[0] = '\0';
    return purl;
}

struct user_policy *user_policy_new()
{
    struct user_policy *puser;

    puser = (struct user_policy *)malloc(sizeof(struct user_policy));
    if (!puser) {
        return NULL;
    }
    memset(puser, 0, sizeof(struct user_policy));

    puser->type = PLCY_WHITELIST;
    puser->plcy_num = 0;
    puser->policy = NULL;
    return puser;
}

void time_slice_del(struct time_slice *ptime)
{
    if (ptime) {
        time_slice_del(ptime->next);
        free(ptime);
    }
}

void daytime_del(struct daytime *pday)
{
    if (pday) {
        time_slice_del(pday->slices);
        free(pday);
    }
}

void url_policy_del(struct url_policy *purl)
{
    int i;

    if (purl) {
        for (i = 0; i < 7; i++) {
            if (purl->daytimes[i]) {
                daytime_del(purl->daytimes[i]);
                purl->daytimes[i] = NULL;
            }
        }
        url_policy_del(purl->next);
        free(purl);
    }
}

void conninfo_del(struct conninfo *pconninfo)
{
    struct conninfo *ptmp;

    while (pconninfo)
    {
        ptmp = pconninfo->next;
        free(pconninfo);
        pconninfo = ptmp;
    }
}

void user_policy_del(struct user_policy *puser)
{
    if (puser) {
        url_policy_del(puser->policy);
        free(puser);
    }
}

void user_policy_del_void(void *puser)
{
    user_policy_del((struct user_policy *)puser);
}

struct policy_node *policy_node_create(uint8_t *sn, uint8_t snlen)
{
    char snstr[SN_MAX_LEN * 2 + 1];
    int slen = SN_MAX_LEN * 2;
    struct policy_node *pnode;

    if (!sn || (snlen <= 0)) {
        log_error("policy node create failed, params error, snlen: %u", snlen);
        return NULL;
    }

    hex2str(sn, snlen, snstr, slen);
    pnode = (struct policy_node *)malloc(sizeof(struct policy_node));
    if (!pnode) {
        log_error("malloc for struct policy_node failed, sn: %s", snstr);
        return NULL;
    }
    memset(pnode, 0, sizeof(struct policy_node));

    pnode->snlen = snlen;
    memcpy(pnode->sn, sn, snlen);
    strncpy(pnode->snstr, snstr, slen);

    pnode->isforbidden = 0;

    pnode->conns = NULL;
    pnode->whitelist = NULL;
    pnode->blacklist = NULL;

    return pnode;
}

void policy_node_free(struct policy_node *pnode)
{
    if (pnode) {
        policy_node_remove_all_conns(pnode);
        user_policy_del(pnode->whitelist);
        user_policy_del(pnode->blacklist);
        free(pnode);
    }
}

void policy_node_free_void(void *node)
{
    policy_node_free((struct policy_node *)node);
}

int policy_node_add_conn(struct policy_node *pnode, uint32_t ipport)
{
    struct conninfo *conn, *new_conn;

    if (!pnode || ipport <= 0) {
        return -1;
    }

    new_conn = (struct conninfo *)malloc(sizeof(struct conninfo));
    if (!new_conn) {
        log_error("malloc for conninfo for add to policy node failed, sn: %s, ipport: %u", pnode->snstr, ipport);
        return -1;
    }
    new_conn->next = NULL;
    new_conn->ipport = ipport;

    conn = pnode->conns;
    if (!conn) {
        pnode->conns = new_conn;
    } else {
        while (conn->next) {
            if (conn->ipport == ipport) {
                log_debug("conn info already added to policy node, sn: %s, ipport: %u", pnode->snstr, ipport);
                return 0;
            }
            conn = conn->next;
        }
        conn->next = new_conn;
    }
    
    return 0;
}

void policy_node_remove_conn(struct policy_node *pnode, uint32_t ipport)
{
    struct conninfo *conn, *prev;

    if (!pnode || ipport <= 0) {
        return;
    }

    conn = pnode->conns;
    if (conn) {
        if (conn->ipport == ipport) {
            pnode->conns = conn->next;
            free(conn);
        } else {
            while (conn->next) {
                prev = conn;
                conn = conn->next;
                if (conn->ipport == ipport) {
                    prev->next = conn->next;
                    free(conn);
                }
            }
        }
    }
}

void policy_node_remove_all_conns(struct policy_node *pnode)
{
    struct conninfo *conn, *next;

    if (pnode) {
        conn = pnode->conns;
        while (conn) {
            next = conn->next;
            free(conn);
            conn = next;
        }
        pnode->conns = NULL;
    }
}

int user_policy_update(struct user_policy *pold, struct user_policy *pnew)
{
    if (!pold || !pnew) {
        log_error("user policy update failed, param error");
        return -1;
    }

    if (pold->type != pnew->type) {
        log_error("user policy type mismatch, can't update, pold->type: %d, pnew->type: %d", pold->type, pnew->type);
        return -1;
    }

    pold->update_time = pnew->update_time;

    pold->plcy_num = pnew->plcy_num;
    pnew->plcy_num = 0;

    url_policy_del(pold->policy);
    pold->policy = pnew->policy;
    pnew->policy = NULL;

    return 0;
}

int time_slice_append(struct daytime *pday, struct time_slice *ptime)
{
    struct time_slice *ptmp;

    if (!pday || !ptime) {
        return -1;
    }

    ptmp = pday->slices;
    if (!ptmp) {
        pday->slices = ptime;
    } else {
        while (ptmp->next) {
            ptmp = ptmp->next;
        }
        ptmp->next = ptime;
    }

    pday->count++;
    return 0;
}

int daytime_append(struct url_policy *purl, struct daytime *pday)
{
    if (!purl || !pday) {
        return -1;
    }

    if ((pday->weekday < 1) || (pday->weekday > 7)) {
        return -1;
    }

    if (purl->daytimes[pday->weekday - 1]) {
        daytime_del(purl->daytimes[pday->weekday - 1]);
    }

    purl->daytimes[pday->weekday - 1] = pday;
    return 0;
}

int url_policy_append(struct user_policy *puser, struct url_policy *purl)
{
    struct url_policy *ptmp;

    if ((!puser) || (!purl)) {
        return -1;
    }

    ptmp = puser->policy;
    if (!ptmp) {
        puser->policy = purl;
    } else {
        while (ptmp->next) {
            ptmp = ptmp->next;
        }
        ptmp->next = purl;
    }

    puser->plcy_num++;
    return 0;
}

static int get_wday_value_on_my_style(struct tm *tm)
{
    if (0 == tm->tm_wday)    return 7;
    else return tm->tm_wday;
}

static int get_wday_index_on_my_style(struct tm *tm)
{
    if (0 == tm->tm_wday)    return 6;
    else return (tm->tm_wday - 1);
}

int is_policy_valid_time(struct url_policy *purl)
{
    struct time_slice *ptimeslice;
    time_t now;
    struct tm tm;
    int nowwday;
    int dayindex;

    now = time(NULL);
    localtime_r(&now, &tm);

    nowwday = get_wday_value_on_my_style(&tm);
    dayindex = get_wday_index_on_my_style(&tm);

    if (NULL == purl->daytimes[dayindex]) {
        return 0;
    }
    if (nowwday != purl->daytimes[dayindex]->weekday) {
        return 0;
    }

    ptimeslice = purl->daytimes[dayindex]->slices;
    while (ptimeslice)
    {
        if (((tm.tm_hour * 60 + tm.tm_min) > (ptimeslice->begin_hour * 60 + ptimeslice->begin_minute)) \
            && ((tm.tm_hour * 60 + tm.tm_min) < (ptimeslice->end_hour * 60 + ptimeslice->end_minute)))
        {
            return 1;
        }
        ptimeslice = ptimeslice->next;
    }

    return 0;
}

static struct url_policy *get_target_policy(struct user_policy *puserplcy, char *purl, size_t url_len)
{
    struct url_policy *ptarget_policy;

    if (!puserplcy) {
        return NULL;
    }

    ptarget_policy = puserplcy->policy;
    while (NULL != ptarget_policy)
    {
        if (1 == url_cmp(ptarget_policy->url, ptarget_policy->url_len, purl, url_len))
            return ptarget_policy;

        if (ptarget_policy == ptarget_policy->next)
        {
            ptarget_policy->next = NULL;
            ptarget_policy = NULL;
            break;
        }

        ptarget_policy = ptarget_policy->next;
    }

    return NULL;
}

int policy_judge(struct policy_node *pnode, char *url, size_t len)
{
    struct url_policy *purl;

    if (!pnode || !url || len <= 0) {
        log_error("policy judge params error");
        return POLICY_JUDGE_PARAM_ERROR;
    }

    if (pnode->isforbidden) {
        log_warn("user has been forbidden, can't access to url: %s, user info: %s, sn: %s", url, pnode->userinfo, pnode->snstr);
        return POLICY_JUDGE_FORBIDDEN;
    }

    purl = get_target_policy(pnode->blacklist, url, len);
    if (purl) {
        log_warn("matched url policy in blacklist, url: %s, user info: %s, sn: %s", url, pnode->userinfo, pnode->snstr);
        if (is_policy_valid_time(purl)) {
            log_warn("access url: %s at valid time of blacklist, user info: %s, sn: %s", url, pnode->userinfo, pnode->snstr);
            return POLICY_JUDGE_IN_BLACKLIST;
        }
    }

    purl = get_target_policy(pnode->whitelist, url, len);
    if (!purl) {
        log_warn("no matched url policy in whitelist, url: %s, user info: %s, sn: %s", url, pnode->userinfo, pnode->snstr);
        return POLICY_JUDGE_NOT_IN_WHITELIST;
    }

    if (!is_policy_valid_time(purl)) {
        log_warn("access url: %s at not valid time of whitelist, user info: %s, sn: %s", url, pnode->userinfo, pnode->snstr);
        return POLICY_JUDGE_TIME_INVALID;
    }

    log_info("access url: %s at valid time of whitelist, user info: %s, sn: %s", url, pnode->userinfo, pnode->snstr);
    return POLICY_JUDGE_ALLOWED;
}

int is_whitelist_policy_allow(struct policy_node *pnode, struct url_policy *purl)
{
    struct url_policy *tmp;

    if (!pnode || !purl) {
        return 0;
    }

    if (purl->url_len <= 0) {
        return 0;
    }

    tmp = get_target_policy(pnode->blacklist, purl->url, purl->url_len);
    if (tmp) {
        if (is_policy_valid_time(tmp)) {
            return 0;
        }
    }

    if (!is_policy_valid_time(purl)) {
        return 0;
    }

    return 1;
}