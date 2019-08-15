/**
 * @brief audit.h 定义审计日志处理函数及相关数据结构
 * 
 * @author	zzl
 * @date	2011-4-28
 * @version	0.1
 */
#ifndef _CONN_AUDIT_H_
#define _CONN_AUDIT_H_

#include "buffer.h"

/** 
 * @brief 创建与日志服务器的连接
 * 
 * @retval CONN_ERR 发生错误
 * @retval CONN_OK  成功
 */
int audit_conn_create();

/** 
 * @brief 关闭与策略服务器的连接
 */
void audit_conn_close();

void audit_check_alive();

/**
 * @brief 发送处理函数
 *
 * @param[in]	send_buf	发送缓冲区,类型为buffer_t
 *
 * @retval	-1	发送错误
 * @retval	0	发送成功
 */
int audit_send_buffer( buffer_t *buf );

#endif
