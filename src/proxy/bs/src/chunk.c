#include "chunk.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

/** 
 * @brief 创建一个chunk缓冲区
 * 
 * @param[in] limit 最大长度
 * 
 * @return chunk指针
 * @retval NULL 分配内存失败
 */
chunk_t *chunk_create( size_t limit )
{
    chunk_t *ck = malloc( sizeof(chunk_t) );
    if( NULL == ck )
        return NULL;

    ck->total_size = 0;
    ck->used_size = 0;
    ck->limit = limit;
    ck->num = 0;
    queue_init( &(ck->queue) );
    return ck;
}


/** 
 * @brief 销毁一个chunk缓冲区
 * 
 * @param[in] ck chunk指针
 */
void chunk_destroy( chunk_t *ck )
{
    queue_t *q = ck->queue.next;
    queue_t *n;
    buffer_t *buf;
    while( q != &(ck->queue) )
    {
        buf = queue_data( q, buffer_t, queue );
        n = q->next;
        queue_remove( q );
        buffer_del( buf );
        q = n;
    }
    free( ck );
}


/** 
 * @brief 将一个buffer加入chunk尾
 * 
 * @param[in] ck chunk
 * @param[in] buf buffer
 * 
 * @retval -1 参数有空指针
 * @retval 0 成功
 */
int buffer_put( chunk_t *ck, buffer_t *buf )
{
    if( NULL==ck || NULL==buf )
        return -1;
    queue_insert_tail( &(ck->queue), &(buf->queue) );
    ck->total_size += buf->size;
    ck->used_size += buffer_mused( buf );
    ck->num++;
    return 0;
}

int buffer_put_head( chunk_t *ck, buffer_t *buf )
{
    if( NULL==ck || NULL==buf )
        return -1;
    queue_insert_head( &(ck->queue), &(buf->queue) );
    ck->total_size += buf->size;
    ck->used_size += buffer_mused( buf );
    ck->num++;
    return 0;
}

/** 
 * @brief 从chunk头部取得一个buffer
 *
 * 将buffer从chunk中去掉
 * 
 * @param[in] ck chunk
 * 
 * @return buffer指针
 * @retval NULL 失败，chunk为空
 */
buffer_t *buffer_get( chunk_t *ck )
{
    if( NULL==ck || queue_isempty( &(ck->queue) ) )
        return NULL;
    queue_t *q = queue_head( &(ck->queue) );
    queue_remove( q );
    buffer_t *buf = queue_data( q, buffer_t, queue );
    ck->num--;
    ck->total_size -= buf->size;
    ck->used_size -= buffer_mused( buf );
    return buf;
}


/** 
 * @brief 获得chunk中第一个buffer
 *
 * 不从chunk中去掉
 * 
 * @param[in] ck chunk
 * 
 * @return buffer指针
 * @retval NULL 失败，chunk为空
 */
buffer_t *buffer_getfirst( chunk_t *ck )
{
    if( NULL==ck || queue_isempty( &(ck->queue) ) )
        return NULL;
    queue_t *q = queue_head( &(ck->queue) );
    return queue_data( q, buffer_t, queue );
}


/** 
 * @brief 获得chunk中最后一个buffer
 *
 * 不从chunk中去掉
 * 
 * @param[in] ck chunk
 * 
 * @return buffer指针
 * @retval NULL 失败，chunk为空
 */
buffer_t *buffer_getlast( chunk_t *ck )
{
    if( NULL==ck || queue_isempty( &(ck->queue) ) )
        return NULL;
    queue_t *q = queue_last( &(ck->queue) );
    return queue_data( q, buffer_t, queue );
}


/** 
 * @brief 获得当前buffer的下一个
 * 
 * 不从chunk中去掉
 * 
 * @param[in] ck chunk
 * @param[in] buf 当前buffer
 * 
 * @return 下一个buffer指针
 * @retval NULL 失败，已到尾部
 */
buffer_t *buffer_getnext( chunk_t *ck, buffer_t *buf )
{
    if( NULL == buf )
        return NULL;
    queue_t *q = queue_next( &(buf->queue) );
    if( &(ck->queue) == q )
        return NULL;
    return queue_data( q, buffer_t, queue );
}


/** 
 * @brief 获得当前buffer前下一个
 * 
 * 不从chunk中去掉
 * 
 * @param[in] ck chunk
 * @param[in] buf 当前buffer
 * 
 * @return 前一个buffer指针
 * @retval NULL 失败，已到头部
 */
buffer_t *buffer_getprev( chunk_t *ck, buffer_t *buf )
{
    if( NULL == buf )
        return NULL;
    queue_t *q = queue_prev( &(buf->queue) );
    if( &(ck->queue) == q )
        return NULL;
    return queue_data( q, buffer_t, queue );
}


/** 
 * @brief 检测chunk是否为空
 * 
 * @param[in] ck chunk
 * 
 * @return 结果，为空返回非0
 */
int chunk_isempty( chunk_t *ck )
{
    if( NULL==ck )
        return 1;
    return queue_isempty( &(ck->queue) );
}


size_t chunk_getfreesize( chunk_t *ck )
{
    if( NULL==ck )
        return 0;
    return ck->limit > ck->used_size ? ck->limit-ck->used_size : 0;
}


/** 
 * @brief 从chunk中删除一个buffer
 *
 * 从chunk中去掉任一个buffer
 * 
 * @param[in] ck chunk
 * @param[in] buf buffer
 * 
 * @retval -1 失败，chunk为空或参数有NULL指针
 * @retval 0 成功
 */
int chunk_delbuffer( chunk_t *ck, buffer_t *buf )
{
    if( NULL==ck || NULL==buf || chunk_isempty(ck) )
        return -1;
    queue_remove( &(buf->queue) );
    ck->num--;
    ck->total_size -= buf->size;
    //ck->used_size -= buffer_mused( buf );
    ck->used_size -= buf->used;
    return 0;
}

static int chunk_removebuffer( chunk_t *ck, buffer_t *buf )
{
    if( NULL==ck || NULL==buf || chunk_isempty(ck) )
        return -1;
    queue_remove( &(buf->queue) );
    ck->num--;
    return 0;
}


int chunk_erase( chunk_t *ck, size_t size )
{
    buffer_t *buf = buffer_getfirst( ck );
    while( NULL != buf )
    {
        if( buf->used <= size )
        {
            chunk_delbuffer( ck, buf );
            size -= buf->used;
            buffer_del( buf );
            buf = buffer_getfirst( ck );
        }
        else
        {
            buf->data += size;
            buf->used -= size;
            ck->used_size -= size;
            return 1;
        }
    }
    return 0;
}

int chunk_merge_buffer( chunk_t *ck, buffer_t *buf, size_t size )
{
    buffer_t *next = NULL;
    int ret = 0;
    int need = size - buf->used;
    int len;

    log_trace( "chunk_merge_buffer------>buffer used:%d, need size:%d", buf->used, size );
    while( need > 0 )
    {
        if( buf->used >= size )
            return 0;

        next = buffer_getnext( ck, buf );
        log_trace( "next buffer to merge %p", next );
        if( NULL == next )
            return -1;

        ret = buffer_resize( buf, size );
        if( 0 != ret )
            return -2;

        len = next->used < need ? next->used : need;
        log_trace( "merge memcpy %d bytes", len );
        memcpy( buf->data+buf->used, next->data, len );
        buf->used += len;
        need -= len;
        next->data += len;
        next->used -= len;
        if( next->used <= 0 )
        {
            chunk_removebuffer( ck, next );
            buffer_del( next );
            next = NULL;
        }
    }
    return 0;
}

buffer_t *chunk_split_buffer( chunk_t *ck, buffer_t *buf, size_t offset )
{
    if( NULL==ck || NULL==buf || offset<=0 )
        return NULL;
    if( offset >= buf->used )
        return buf;

    size_t size;
    uint8_t *start;
    buffer_t *new;

    if( offset <= buf->used/2 )
    {
        size = offset;
        start = buf->data;
    }
    else
    {
        size = buf->used - offset;
        start = buf->data + offset;
    }

    new = buffer_new( size );
    if( NULL == new )
    {
        log_error( "alloc new spolit buffer failed" );
        return NULL;
    }
    memcpy( new->data, start, size );
    new->used = size;

    if( start == buf->data )
    {
        queue_insert_before( &(buf->queue), &(new->queue) );
        buf->data += size;
        buf->used -= size;
        return new;
    }
    else
    {
        queue_insert_after( &(buf->queue), &(new->queue) );
        buf->used -= size;
        return buf;
    }
}

