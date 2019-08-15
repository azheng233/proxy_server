/** 
 * @file job.c
 * @brief job操作和处理函数
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#include "job.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>

/**
 * @ingroup job
 * @brief job队列头
 */
jobhead_t job_head;


static void jobqueue_init( jobhead_t *jh )
{
    queue_init( &(jh->queue) );
    jh->num = 0;
}

/** 
 * @brief 初始化job队列
 * 
 * @return JOB_OK
 */
int job_init()
{
    jobqueue_init( &job_head );
    return JOB_OK;
}


void jobqueue_clear( jobhead_t *jh )
{
    job_t *job;
    queue_t *q = jh->queue.next;
    while( q != &(jh->queue) )
    {
        job = queue_data( q, job_t, queue );
        q = q->next;

        queue_remove( &(job->queue) );
        jh->num--;
        free( job );
    }
}

/** 
 * @brief 销毁job队列
 */
void job_destroy()
{
    log_debug( "destroy %u jobs", job_head.num );
    job_t *job;
    queue_t *q = job_head.queue.next;
    while( q != &(job_head.queue) )
    {
        job = queue_data( q, job_t, queue );
        q = q->next;

        queue_remove( &(job->queue) );
        job_head.num--;
        free( job );
    }
    if( job_head.num > 0 )
        log_debug( "%u jobs remain", job_head.num );
}

/** 
 * @brief 清空job队列
 *
 * 真正删除被标记的job
 */
void job_clear()
{
    if( job_head.num )
        log_trace( "%u jobs total", job_head.num );

    job_t *job;
    queue_t *q = job_head.queue.next;
    while( q != &(job_head.queue) )
    {
        job = queue_data( q, job_t, queue );
        q = q->next;

        if( job->deld )
        {
            queue_remove( &(job->queue) );
            job_head.num--;
            free( job );
        }
    }

    if( job_head.num )
        log_debug( "%u jobs remain", job_head.num );
}


job_t* job_new( int events, connection_t *conn )
{
    if( NULL == conn )
        return NULL;

    job_t *job = malloc( sizeof(job_t) );
    if( NULL == job )
        return NULL;

    job->events = events;
    job->conn = conn;
    job->deld = 0;
    queue_init( &(job->queue) );
    return job;
}

int jobqueue_add( jobhead_t *head, job_t *job )
{
    queue_insert_tail( &(head->queue), &(job->queue) );
    head->num++;
    return JOB_OK;
}

/** 
 * @brief 将一个连接加入job，响应事件
 *
 * 创建一个新的job结构并加入队列，在该job被调度时连接 \a conn 将响应事件 \a events
 * 
 * @param[in] events 事件
 * @param[in] conn 连接
 * 
 * @retval JOB_ERR 分配内存失败
 * @retval JOB_OK 成功
 */
int job_addconn( int events, connection_t *conn )
{
    job_t *last = job_getlast();
    if (NULL != last)
    {
        if (conn == last->conn)
        {
            log_debug("connection %d is already in job, event %x, new event %x", conn->fd, last->events, events);
            if (job_isdel(last)) {
                last->deld = 0;
                last->events = events;
            } else {
                last->events |= events;
            }
            return JOB_OK;
        }
    }

    job_t *job = job_new( events, conn );
    if (NULL == job) {
        return JOB_ERR;
    }

    jobqueue_add( &job_head, job );
    return JOB_OK;
}


void jobqueue_del( jobhead_t *head, job_t *job )
{
    if( ! queue_isempty( &(head->queue) ) )
    {
        queue_remove( &(job->queue) );
        head->num--;
    }
}

/** 
 * @brief 从job中删除一个连接
 *
 * 删除 \a conn 在job队列中的所有事件
 * 
 * @param[in] conn 连接
 * 
 * @return JOB_OK
 */
int job_delconn( connection_t *conn )
{
    job_t *job;
    queue_t *q = job_head.queue.next;
    while( q != &(job_head.queue) )
    {
        job = queue_data( q, job_t, queue );
        q = q->next;

        if( job->conn == conn )
        {
            job_del( job );
        }
    }
    return JOB_OK;
}


/** 
 * @brief 获得job队列中第一个job
 *
 * 并不从队列中删除
 * 
 * @return job
 * @retval NULL 失败，队列为空
 */
job_t* job_getfirst()
{
    if( queue_isempty( &(job_head.queue) ) )
        return NULL;
    return queue_data( job_head.queue.next, job_t, queue );
}


/** 
 * @brief 获得当前job的下一个job
 * 
 * @param[in] job 当前job
 * 
 * @return job
 * @retval NULL 已到队尾，没有下一个
 */
job_t* job_getnext( job_t *job )
{
    queue_t *q = job->queue.next;
    if( &(job->queue)==q || &(job_head.queue)==q )
        return NULL;
    return queue_data( q, job_t, queue );
}


/** 
 * @brief 获得队列中最后一个job
 * 
 * @return job
 * @retval NULL 失败，队列为空
 */
job_t* job_getlast()
{
    if( queue_isempty( &(job_head.queue) ) )
        return NULL;
    return queue_data( job_head.queue.prev, job_t, queue );
}
