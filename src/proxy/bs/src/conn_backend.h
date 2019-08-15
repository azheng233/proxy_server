/** 
 * @file conn_backend.h
 * @brief 与后端服务的连接
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#ifndef _CONN_BACKEND_H_
#define _CONN_BACKEND_H_

#include "connection.h"

/**
 * @defgroup backend_connection
 * @brief 与后端的连接
 *
 * 由客户端连接创建
 *
 * @{
 */

connection_t *backend_conn_create( connection_t *clntconn, struct sockaddr_in *addr );

connection_t *backend_conn_create_noip( connection_t *clntconn, struct sockaddr_in *addr );

/** @} */
#endif
