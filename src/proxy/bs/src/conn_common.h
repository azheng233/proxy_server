/** 
 * @file conn_common.h
 * @brief 通用的连接操作
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#ifndef _CONN_COMMON_H_
#define _CONN_COMMON_H_

#include "connection.h"

int setsockreuse( int fd );

int setnonblocking( int fd );

int setnonblocking_notsockfd( int fd );

int conn_listen( connection_t *conn, int listenum );

int conn_connect( connection_t *conn );

int conn_signal_close( connection_t *conn );

int backend_handle_read( connection_t *conn );

int backend_conn_connect( connection_t *conn, in_addr_t ip );

#endif
