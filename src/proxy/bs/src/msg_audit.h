#ifndef _MSG_AUDIT_H_
#define _MSG_AUDIT_H_

#include "buffer.h"
#include <stdint.h>

/** 
 * @brief 批量日志采集初始化
 * 
 * @param[in] connection_count 在线客户端个数
 * 
 * @return buffer_t
 */
buffer_t * audit_log_flow_begin(unsigned int connection_count);

/** 
 * @brief 批量日志，添加一条用户记录
 * 
 * @param[in] buf 日志缓冲buffer_t
 * @param[in] sn  客户端SN
 * @param[in] snlen  SN长度 
 * @param[in] upload   UPLOAD
 * @param[in] download   DOWNLOAD
 * 
 * @return 0
 */
int audit_log_flow_append(buffer_t * buf,uint8_t *sn, uint8_t snlen, uint32_t upload, uint32_t download);

/** 
 * @brief 批量日志，发送日志
 * 
 * @param[in] buf 日志缓冲buffer_t
 * 
 * @return 0
 */
int audit_log_flow_send(buffer_t * buf);

/** 
 * @brief 当用户日志采集
 * 
 * @param[in] sn 用户SN
 * @param[in] snlen  SN长度
 * @param[in] upload  UPLOAD
 * @param[in] download  DOWNLOAD
 * 
 * @return CONN_OK
 */
int audit_log_flow(uint8_t *sn, uint8_t snlen, uint32_t upload, uint32_t download);


enum audit_log_level {
    AUDIT_LOG_NONE = 0,
    AUDIT_LOG_DENY = 1,
    AUDIT_LOG_ALL = 2
};

enum url_action {
    URL_DENY = 0,
    URL_PASS = 1
};

int audit_log_url( uint8_t *sn, uint8_t snlen, char *url, uint16_t urlen, enum url_action action );

#define audit_log_pass_url( sn, snlen, url, urlen ) \
    audit_log_url( (sn), (snlen), (url), (urlen), URL_PASS )

#define audit_log_deny_url( sn, snlen, url, urlen ) \
    audit_log_url( (sn), (snlen), (url), (urlen), URL_DENY )

#endif
