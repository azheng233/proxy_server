/** 
 * @file chunk.h
 * @brief 非连续的缓冲区
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#ifndef _CHUNK_H_
#define _CHUNK_H_

#include "buffer.h"
#include "queue.h"

/**
 * @defgroup chunk
 * @brief buffer链表形式的缓冲区
 *
 * @{
 */

/// chunk结构，即缓冲区链表头
typedef struct chunk_st {
    size_t total_size;          ///< 缓冲区总内存大小
    size_t used_size;           ///< 缓冲区已使用内存
    size_t limit;               ///< 最大长度限制
    unsigned int num;           ///< buffer数目
    queue_t queue;
} chunk_t;

chunk_t *chunk_create( size_t limit );
void chunk_destroy( chunk_t *ck );

int buffer_put( chunk_t *ck, buffer_t *buf );
int buffer_put_head( chunk_t *ck, buffer_t *buf );
buffer_t *buffer_get( chunk_t *ck );

int chunk_delbuffer( chunk_t *ck, buffer_t *buf );

buffer_t *buffer_getfirst( chunk_t *ck );
buffer_t *buffer_getlast( chunk_t *ck );
buffer_t *buffer_getnext( chunk_t *ck, buffer_t *buf );
buffer_t *buffer_getprev( chunk_t *ck, buffer_t *buf );

int chunk_isempty( chunk_t *ck );

size_t chunk_getfreesize( chunk_t *ck );

/** 
 * @brief 从缓冲链头部开始，删除size长度的数据
 *
 * 会销毁包含数据的缓冲区
 *
 * @param[in] ck 缓冲链头
 * @param[in] size 数据长度
 * 
 * @retval 1 删除后可能还有数据
 * @retval 0 缓冲链为空或删除后为空
 */
int chunk_erase( chunk_t *ck, size_t size );

int chunk_merge_buffer( chunk_t *ck, buffer_t *buf, size_t size );

buffer_t *chunk_split_buffer( chunk_t *ck, buffer_t *buf, size_t offset );

/** @} */
#endif
