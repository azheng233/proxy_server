/** 
 * @file conn_gateway_lsn.h
 * @brief 网关监听连接
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#ifndef _CONN_GATEWAY_LSN_H_
#define _CONN_GATEWAY_LSN_H_

/**
 * @defgroup gateway_listen
 * @brief 监听并接受网关的连接
 * @{
 */

/// 同时接受的最大连接数
#define GATEWAY_LSN_NUM  32

int gateway_lsn_init();

void gateway_lsn_close();

void gateway_connections_destroy();

/** @} */
#endif
