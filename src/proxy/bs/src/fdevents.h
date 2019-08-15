/** 
 * @file fdevents.h
 * @brief 网络监听事件操作
 * @author hps@xdja.com
 * @version 1.0
 * @date 2010-08-03
 */
#ifndef _FDEVENTS_H_
#define _FDEVENTS_H_

#include "connection.h"
/**
 * @defgroup fdevents
 * @brief 封装事件监听
 *
 * 使用epoll
 * @{
 */

/*
*能够监听事件的定义
*/
#define BV(x) (1<<(x))
#define FDEVENT_IN     BV(0)
#define FDEVENT_PRI    BV(1)
#define FDEVENT_OUT    BV(2)
#define FDEVENT_ERR    BV(3)
#define FDEVENT_HUP    BV(4)
#define FDEVENT_NVAL   BV(5)
#define FDEVENT_CLOSE      BV(6)
#define FDEVENT_PEERCLOSE  BV(7)
#define FDEVENT_LT     BV(8)

/*错误返回值*/
#define FDEVENT_OK 0
#define FDEVENT_ERROR -1


int ev_init();

int ev_addfd( int fd, int events, connection_t *con );

int ev_modfd( int fd, int events, connection_t *con );

int ev_delfd( int fd, connection_t *con );

int ev_wait();

int ev_getnext( int *fd, int *events, connection_t **ppcon );

void ev_free();

/** @} */
#endif
