#ifndef _URL_FILTER_DATA_TYPE_H_
#define _URL_FILTER_DATA_TYPE_H_

#include <time.h>
#include <stdint.h>

//judge error
#define POLICY_JUDGE_PARAM_ERROR        -1
#define POLICY_JUDGE_ALLOWED            0
#define	POLICY_JUDGE_FORBIDDEN          1
#define	POLICY_JUDGE_IN_BLACKLIST       2
#define	POLICY_JUDGE_NOT_IN_WHITELIST   3
#define	POLICY_JUDGE_TIME_INVALID       4

#define FLAGS_BLOCK         0
#define	FLAGS_ALLOW         1

#define SN_MAX_LEN          128
#define USERINFO_MAX_LEN    256

struct time_slice {
    struct time_slice *next;
    //time slice begin
    uint8_t begin_hour;
    uint8_t begin_minute;
    //time slice end
    uint8_t end_hour;
    uint8_t end_minute;
};

struct conninfo {
    struct conninfo *next;
    uint32_t ipport;
};

struct daytime {
    uint8_t weekday;
    uint8_t count;
    struct time_slice *slices;
};

struct url_policy {
    struct url_policy *next;
    //time info
    struct daytime *daytimes[7];
    //url
    size_t url_len;
    char url[1];
};

enum policy_type {
    PLCY_WHITELIST = 1,
    PLCY_BLACKLIST = 2
};

struct user_policy {
    enum policy_type type;
    time_t update_time;
    unsigned int plcy_num;
    struct url_policy *policy;
};

struct policy_node {
    int snlen;
    uint8_t sn[SN_MAX_LEN];
    char snstr[SN_MAX_LEN * 2 + 1];
    char userinfo[USERINFO_MAX_LEN + 1];

    int isforbidden;

    struct conninfo *conns;
    struct user_policy *whitelist;
    struct user_policy *blacklist;
};

#endif
