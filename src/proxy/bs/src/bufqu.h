#ifndef _MEM_BUFQU_H_
#define _MEM_BUFQU_H_
/** 
 * @file
 * @brief 缓冲队列声明
 * @note 使用方法参见样例文件@ref test.c
 * @author sunxp
 * @version 0001
 * @date 2010-03-23
 */
/**
 * @mainpage
 * - 定义和接口 @ref buffer.h
 * - 使用示例 @ref test.c
 *
 * @example test.c
 * 缓冲区队列使用示例
 */
#include "queue.h"
#include <stddef.h>

/// 缓冲区结构
typedef struct buffer_qu_st {
    unsigned char *buf; ///< 实际缓冲区
    size_t sum_len;     ///< 总长度
	size_t used;		///< 长度
    size_t r;           ///< 读偏移
    size_t w;           ///< 写偏移
    queue_t queue;      ///< 链表节点，用以链接带缓冲区队列中
} buffer_qu_t;

/// 缓冲区队列头
typedef struct buffer_queue_st {
    unsigned int sum;   ///< 队列中缓冲区数目
    queue_t queue;
} buffer_queue_t;


/** 
 * @brief 创建一个缓冲队列
 * 
 * @param[out] head 缓冲队列头
 * 
 * @return 缓冲区结构指针
 * @retval NULL 失败
 */
buffer_queue_t* buffer_queue_create( buffer_queue_t **head );

/** 
 * @brief 销毁缓冲队列
 * 
 * @param[in] head 缓冲队列头
 * 
 * @retval -1 失败
 * @retval 0 成功
 */
int buffer_queue_destroy( buffer_queue_t *head );

/** 
 * @brief 清空缓冲队列
 * 
 * @param[in] head 缓冲队列头
 * 
 * @retval -1 失败
 * @retval 0 成功
 */
int buffer_queue_empty( buffer_queue_t *head );

/** 
 * @brief 得到队列中缓冲区个数
 * 
 * @param[in] head 队列头
 * 
 * @return 缓冲区个数
 */
 unsigned int buffer_queue_num( buffer_queue_t *head );

/** 
 * @brief 将缓冲节点插入对头
 * 
 * @param[in] head 队列头
 * @param[in] buf  缓冲节点
 */
void buffer_qu_put_head( buffer_queue_t *head, buffer_qu_t *buf );

/** 
 * @brief 将缓冲节点插入对尾
 * 
 * @param[in] head 队列头
 * @param[in] buf  缓冲节点
 */
void buffer_put_tail( buffer_queue_t *head, buffer_qu_t *buf );

/** 
 * @brief 从队列头中取出一个节点
 * 
 * @param[in] head 队列头
 * @param[out] buf  缓冲节点
 * 
 * @return 缓冲区结构指针
 * @retval NULL 失败或队列为空
 */
buffer_qu_t* buffer_get_head( buffer_queue_t *head, buffer_qu_t **buf );

/** 
 * @brief 从队列尾中取出一个节点
 * 
 * @param[in] head 队列头
 * @param[out] buf  缓冲节点
 * 
 * @return 缓冲区结构指针
 * @retval NULL 失败或队列为空
 */
buffer_qu_t* buffer_get_tail( buffer_queue_t *head, buffer_qu_t **buf );


buffer_qu_t *buffer_queue_fetch_first( buffer_queue_t *head );
buffer_qu_t *buffer_queue_fetch_next( buffer_queue_t *head, buffer_qu_t *buf );
buffer_qu_t *buffer_queue_fetch_prev( buffer_queue_t *head, buffer_qu_t *buf );
buffer_qu_t *buffer_queue_fetch_last( buffer_queue_t *head );

buffer_qu_t *buffer_queue_remove( buffer_queue_t *head, buffer_qu_t *buf );


/** 
 * @brief 获取指定长度的缓冲区
 * 
 * @param[in] len 缓冲区长度
 * 
 * @return 缓冲区结构指针
 * @retval NULL 失败
 */
buffer_qu_t* buffer_qu_new( size_t len );


/** 
 * @brief 释放缓冲区到内存池
 * 
 * @param[in] buf 缓冲区
 * 
 * @retval -1 失败
 * @retval 0 成功
 */
int buffer_qu_del( buffer_qu_t *buf );

/**
 * @brief 扩展缓冲区大小
 *
 * @param buf
 *
 * @return 
 */
int buffer_qu_realloc( buffer_qu_t *buf, size_t len );

/** 
 * @brief 得到缓冲区写指针
 * 
 * @param buf 缓冲区
 * 
 * @return 写指针
 */
inline unsigned char* buf_wptr( buffer_qu_t *buf );
/** 
 * @brief 得到缓冲区可写长度
 * 
 * @param buf 缓冲区
 * 
 * @return 可写长度
 */
inline size_t buf_wlen( buffer_qu_t *buf );
/** 
 * @brief 得到缓冲区读指针
 * 
 * @param buf 缓冲区
 * 
 * @return 读指针
 */
inline unsigned char* buf_rptr( buffer_qu_t *buf );
/** 
 * @brief 得到缓冲区可读长度
 * 
 * @param buf 缓冲区
 * 
 * @return 可读长度
 */
inline size_t buf_rlen( buffer_qu_t *buf );

buffer_qu_t *create_buffer_with_value( unsigned char *value, size_t len );

#endif
