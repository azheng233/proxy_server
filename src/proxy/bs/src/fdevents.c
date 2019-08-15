#include "fdevents.h"
#include "connection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define FDEVENTS_MAXFDS 60000
#define FDEVENTS_TIMEOUT 1000
#define FDEVENTS_SYS_FDNUM 65536

static struct epoll_event evs_buf[FDEVENTS_MAXFDS];
//connection_t *evs_con_buf[FDEVENTS_SYS_FDNUM];

static int evs_fd;//监听fd
static int evs_num;//当前发生的事件数量
static int evs_ind;//当前处理的索引


/** 
 * @brief 初始化事件监听
 * 
 * @retval FDEVENT_ERROR 失败
 * @retval FDEVENT_OK 成功
 */
int ev_init()
{
	evs_fd = epoll_create( FDEVENTS_MAXFDS );
	if( evs_fd<=0 ) return FDEVENT_ERROR;
	evs_num = 0;
	evs_ind = 0;
	return FDEVENT_OK;
}

/** 
 * @brief 销毁事件监听
 */
void ev_free()
{
	if( evs_fd>0 ) close( evs_fd );
	evs_fd = 0;
	evs_num = 0;
	evs_ind = 0;
}

/** 
 * @brief 加入连接 \a con，监听事件 \a events
 * 
 * @param[in] fd socket描述符
 * @param[in] events 监听事件
 * @param[in] con 连接
 * 
 * @retval FDEVENT_ERROR 失败
 * @retval FDEVENT_OK 成功
 */
int ev_addfd( int fd, int events, connection_t *con )
{
	struct epoll_event ev = {0};

	if( NULL == con )
            return FDEVENT_ERROR;

        con->lsnevents = events;

	if( events & FDEVENT_IN)  ev.events |= EPOLLIN;
	if( events & FDEVENT_OUT) ev.events |= EPOLLOUT;
        if( !( events & FDEVENT_LT ) )
            ev.events |= EPOLLET;

	ev.events |= EPOLLERR;
	ev.events |= EPOLLHUP;

	ev.data.ptr = con;

	if ( epoll_ctl(evs_fd,EPOLL_CTL_ADD,fd,&ev)!=0 ) 
	{
		return FDEVENT_ERROR;
	}

	//if( con!=NULL && fd<FDEVENTS_SYS_FDNUM ) evs_con_buf[fd] = con;
	return FDEVENT_OK;
}

/** 
 * @brief 改变一个连接监听的事件
 * 
 * @param[in] fd socket描述符
 * @param[in] events 监听事件
 * @param[in] con 连接
 * 
 * @retval FDEVENT_ERROR 失败
 * @retval FDEVENT_OK 成功
 */
int ev_modfd( int fd, int events, connection_t *con )
{
	struct epoll_event ev = {0};

	if( NULL == con )
            return FDEVENT_ERROR;

        con->lsnevents = events;

	if (events & FDEVENT_IN)  ev.events |= EPOLLIN;
	if (events & FDEVENT_OUT) ev.events |= EPOLLOUT;
        if( !( events & FDEVENT_LT ) )
            ev.events |= EPOLLET;

	ev.events |= EPOLLERR;
	ev.events |= EPOLLHUP;

	ev.data.ptr = con;

	if ( epoll_ctl(evs_fd,EPOLL_CTL_MOD,fd,&ev)!=0 ) 
	{
		return FDEVENT_ERROR;
	}

	return FDEVENT_OK;
}


/** 
 * @brief 从监听中删除一个连接
 * 
 * @param[in] fd socket描述符
 * @param[in] con 连接
 * 
 * @retval FDEVENT_ERROR 失败
 * @retval FDEVENT_OK 成功
 */
int ev_delfd( int fd, connection_t *con )
{
	if( con!=NULL ) con->lsnevents = 0;
	
	if ( epoll_ctl(evs_fd, EPOLL_CTL_DEL, fd, NULL) !=0 ) 
	{		
		return FDEVENT_ERROR;
	}

	//if( fd<FDEVENTS_SYS_FDNUM ) evs_con_buf[fd] = NULL;

	return FDEVENT_OK;
}

/** 
 * @brief 等待监听返回
 * 
 * @return 发生事件的连接数目
 * @retval -1 发送错误
 */
int ev_wait()
{
	evs_num = 0;
	evs_ind = 0;
	evs_num = epoll_wait(evs_fd, evs_buf, FDEVENTS_MAXFDS, FDEVENTS_TIMEOUT);
	return evs_num;
}

/** 
 * @brief 取的下一个发生事件的连接
 * 
 * @param[out] fd socket描述符
 * @param[out] events 发生的事件
 * @param[out] ppcon 连接
 * 
 * @retval FDEVENT_ERROR 失败
 * @retval FDEVENT_OK 成功
 */
int ev_getnext( int *fd, int *events, connection_t **ppcon )
{
        connection_t *conn;
	if( evs_ind<evs_num )
	{	
                conn = (connection_t *)(evs_buf[evs_ind].data.ptr);
                *fd = conn->fd;
		(*events) = 0;
		if ((evs_buf[evs_ind]).events&EPOLLIN)  (*events)|= FDEVENT_IN;
		if ((evs_buf[evs_ind]).events&EPOLLOUT) (*events)|= FDEVENT_OUT;
		if ((evs_buf[evs_ind]).events&EPOLLERR) (*events)|= FDEVENT_ERR;
		if ((evs_buf[evs_ind]).events&EPOLLHUP) (*events)|= FDEVENT_HUP;
		if( ppcon!=NULL )
                    (*ppcon) = conn;
		evs_ind++;	
	}
	else
	{
		return FDEVENT_ERROR;
	}
	return FDEVENT_OK;
}
