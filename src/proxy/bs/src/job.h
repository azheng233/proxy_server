/** 
 * @file job.h
 * @brief job机制处理事件
 * @author sunxp
 * @version 01
 * @date 2010-07-21
 */
#ifndef _JOB_H_
#define _JOB_H_

#include "queue.h"
#include "connection.h"
/** 
 * @defgroup job
 * @brief job
 *
 * 额外的事件处理机制，主要处理发送事件
 * @{
 */

#define JOB_OK  0
#define JOB_ERR -1

/// job队列头
typedef struct jobhead_st {
    unsigned int num;
    queue_t queue;
} jobhead_t;

/// job结构
typedef struct job_st {
    connection_t *conn;         ///< 对应连接
    int events;                 ///< 响应事件
    unsigned char deld;         ///< 删除标记
    queue_t queue;
} job_t;


int job_init();

void job_destroy();

void job_clear();


int job_addconn( int events, connection_t *conn );

int job_delconn( connection_t *conn );

/** 
 * @brief 删除一个job结构
 *
 * 只做标记，并不真正删除
 * @see job_clear
 * 
 * @param[in] job
 */
#define job_del( job ) \
    ( (job)->deld = 1 )

/** 
 * @brief 检测一个job结构是否被删除
 * @param[in] job
 */
#define job_isdel( job ) \
    ( (job)->deld )

job_t* job_getfirst();

job_t* job_getnext( job_t *job );

job_t* job_getlast();

/**  @} */
#endif
