/** 
 * @file conn_policy.h
 * @brief 策略服务器连接
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#ifndef _CONN_POLICY_H_
#define _CONN_POLICY_H_

#include "buffer.h"
#include <stdint.h>

/**
 * @defgroup policy_connection
 * @brief 与策略服务器的连接
 * @{
 */

#include "connection.h"
extern connection_t *plcy_conn;
int policy_conn_create();

void policy_conn_close();

void policy_check_alive();

int policy_send_buffer( buffer_t *buf );

/** @} */
#endif
