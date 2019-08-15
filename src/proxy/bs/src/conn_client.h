/** 
 * @file conn_client.h
 * @brief 客户端连接
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#ifndef _CONN_CLIENT_H_
#define _CONN_CLIENT_H_

#include <netinet/in.h>

/**
 * @defgroup client_connection
 * @brief 客户端连接
 *
 * @{
 */

int client_conn_create( int fd, struct sockaddr_in *addr );

/** @} */
#endif
