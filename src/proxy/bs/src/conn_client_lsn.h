/** 
 * @file conn_client_lsn.h
 * @brief 客户端监听
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#ifndef _CONN_CLIENT_LSN_H_
#define _CONN_CLIENT_LSN_H_

/**
 * @defgroup client_listen
 * @brief 客户端监听
 * @{
 */

/// 同时接受的最大连接数
#define CLIENT_LSN_NUM  128

int client_lsn_init();

void client_lsn_close();

/** @} */
#endif
