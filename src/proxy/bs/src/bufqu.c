/** 
 * @file buffer.c
 * @brief 缓冲队列实现
 * @author sunxp
 * @version 0001
 * @date 2010-03-23
 */

#include "bufqu.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

buffer_queue_t* buffer_queue_create( buffer_queue_t **head )
{
    buffer_queue_t *h = NULL;
    h = (buffer_queue_t *)malloc( sizeof(buffer_queue_t) );
    if( NULL == h )
        return NULL;

    h->sum = 0;
    h->queue.prev = &(h->queue);
    h->queue.next = &(h->queue);

    if( head )
        *head = h;
    return h;
}

int buffer_queue_destroy( buffer_queue_t *head )
{
    ///@todo 销毁时要释放队列中所有缓冲节点吗
    free( head );
    return 0;
}

int buffer_queue_empty( buffer_queue_t *head )
{
 	buffer_qu_t *buf;
 	if( NULL==head ) return -1;
 	while( (buf=buffer_get_head(head,NULL))!=NULL )
 	{
 		buffer_qu_del( buf );
 	}
 	return 0;
}

unsigned int buffer_queue_num( buffer_queue_t *head )
{
     return head->sum;
}

void buffer_qu_put_head( buffer_queue_t *head, buffer_qu_t *buf )
{
    queue_insert_head( &(head->queue), &(buf->queue) );
    head->sum++;
}

void buffer_put_tail( buffer_queue_t *head, buffer_qu_t *buf )
{
    queue_insert_tail( &(head->queue), &(buf->queue) );
    head->sum++;
}

buffer_qu_t* buffer_get_head( buffer_queue_t *head, buffer_qu_t **buf )
{
    if( NULL==head || 0==head->sum || queue_isempty( &(head->queue) ) || NULL==head->queue.next )
        return NULL;
    queue_t *p = queue_next( &(head->queue) );
    queue_remove( p );
    head->sum--;
    p->next = p->prev = NULL;

    buffer_qu_t *b = queue_data( p, buffer_qu_t, queue );
    if( buf )
        *buf = b;
    return b;
}

buffer_qu_t* buffer_get_tail( buffer_queue_t *head, buffer_qu_t **buf )
{
    if( NULL==head || 0==head->sum || queue_isempty( &(head->queue) ) || NULL==head->queue.prev )
        return NULL;

    queue_t *p = queue_prev( &(head->queue) );
    queue_remove( p );
    head->sum--;
    p->next = p->prev = NULL;

    buffer_qu_t *b = queue_data( p, buffer_qu_t, queue );
    if( buf )
        *buf = b;
    return b;
}


buffer_qu_t *buffer_queue_fetch_first( buffer_queue_t *head )
{
    if( NULL==head || 0==head->sum )
        return NULL;
    queue_t *q = head->queue.next;
    if( &(head->queue)==q )
        return NULL;
    return queue_data( q, buffer_qu_t, queue );
}

buffer_qu_t *buffer_queue_fetch_next( buffer_queue_t *head, buffer_qu_t *buf )
{
    if( NULL==head || NULL==buf || 0==head->sum )
        return NULL;
    queue_t *q = buf->queue.next;
    if( &(head->queue)==q || &(buf->queue)==q )
        return NULL;
    return queue_data( q, buffer_qu_t, queue );
}

buffer_qu_t *buffer_queue_fetch_prev( buffer_queue_t *head, buffer_qu_t *buf )
{
    if( NULL==head || NULL==buf || 0==head->sum )
        return NULL;
    queue_t *q = buf->queue.prev;
    if( &(head->queue)==q || &(buf->queue)==q )
        return NULL;
    return queue_data( q, buffer_qu_t, queue );
}

buffer_qu_t *buffer_queue_fetch_last( buffer_queue_t *head )
{
    if( NULL==head || 0==head->sum )
        return NULL;
    queue_t *q = head->queue.prev;
    if( &(head->queue)==q )
        return NULL;
    return queue_data( q, buffer_qu_t, queue );
}

buffer_qu_t *buffer_queue_remove( buffer_queue_t *head, buffer_qu_t *buf )
{
    if( NULL==head || NULL==buf )
        return NULL;
    queue_remove( &(buf->queue) );
    head->sum--;
    return buf;
}


buffer_qu_t* buffer_qu_new( size_t len )
{
    buffer_qu_t *buf = (buffer_qu_t *)malloc( sizeof(buffer_qu_t) );
    if( NULL == buf )
        return NULL;
    memset( buf, 0, sizeof(buffer_qu_t) );

    buf->buf = (unsigned char *)malloc( len );
    if( NULL == buf->buf )
    {
        free( buf );
        return NULL;
    }

    buf->sum_len = len;
	buf->used = 0;
	buf->r = 0;
	buf->w = 0;
    return buf;
}

int buffer_qu_del( buffer_qu_t *buf )
{
    free( buf->buf );
    free( buf );
    return 0;
}

int buffer_qu_realloc( buffer_qu_t *buf, size_t len )
{
    unsigned char *f;

    if( !buf )
        return -1;

    if( buf->sum_len >= len )
        return 0;

    f = (unsigned char *)realloc( buf->buf, len );
    if( !f )
        return -1;

    buf->buf = f;
    buf->sum_len = len;
    return 0;
}

inline unsigned char* buf_wptr( buffer_qu_t *buf )
{
	return buf->buf + buf->w;
}

inline size_t buf_wlen( buffer_qu_t *buf )
{
	return buf->sum_len - buf->w;
}

inline unsigned char* buf_rptr( buffer_qu_t *buf )
{
	return buf->buf + buf->r;
}

inline size_t buf_rlen( buffer_qu_t *buf )
{
	return buf->w - buf->r;
}

buffer_qu_t *create_buffer_with_value( unsigned char *value, size_t len )
{
    buffer_qu_t *buffer = (buffer_qu_t *)malloc( sizeof(buffer_qu_t) );
    if( NULL == buffer ) return NULL;

    buffer->buf = value;
    buffer->sum_len = len;
    buffer->used = len;
    buffer->r = 0;
    buffer->w = 0;

    return buffer;
}

