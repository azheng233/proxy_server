/** 
 * @file buffer.c
 * @brief 缓冲区实现
 * @author sunxp
 * @version 0.1
 * @date 2010-07-08
 */

#include "buffer.h"
#include <stdlib.h>
#include <string.h>


buffer_t *buffer_new( const size_t size )
{
    int l = size % BUFFER_SIZE_MIN;
    size_t real = l==0 ? size : size-l+BUFFER_SIZE_MIN;

    unsigned char *buf = malloc( real );
    if( NULL == buf )
        return NULL;

    buffer_t *bf = malloc( sizeof(buffer_t) );
    if( NULL == bf )
    {
        free( buf );
        return NULL;
    }

    queue_init( &(bf->queue) );
    bf->size = real;
    bf->buf = buf;
    bf->data = buf;
    bf->used = 0;
    bf->flags = 0;
    return bf;
}


void buffer_del( buffer_t *buf )
{
    free( buf->buf );
    free( buf );
}


void buffer_setflag( buffer_t *buffer, unsigned short flag )
{
    buffer->flags |= flag;
}

void buffer_unsetflag( buffer_t *buffer, unsigned short flag )
{
    buffer->flags &= ~flag;
}

int buffer_testflag( buffer_t *buffer, unsigned short flag )
{
    return buffer->flags & flag;
}


void buffer_reset( buffer_t *buf )
{
    //bf->queue
    buf->data = buf->buf;
    buf->used = 0;
    buf->flags = 0;
}


int buffer_resize( buffer_t *buf, size_t newsize )
{
    int offset = buf->data - buf->buf;
    int i = 0;
    if( buf->size >= newsize )
    {
        if( offset > 0 )
        {
            for( i=0; i<buf->used; i++ )
                buf->buf[i] = buf->data[i];
            buf->data = buf->buf;
        }
        return 0;
    }

    unsigned char *b = malloc( newsize );
    if( NULL == b )
        return -1;
    memcpy( b, buf->data, buf->used );
    buf->data = b;
    free( buf->buf );
    buf->buf = b;
    buf->size = newsize;
    return 0;
}

int buffer_cmp( buffer_t *buf1, buffer_t *buf2 )
{
	if( buf1==NULL ) 
	{
		if( buf2==NULL ) return 0;
		else return 1;
	}
	else
	{
		if( buf2==NULL ) return 1;
	}

	if( buf1->used!=buf2->used ) return 1;
	if( buf1->used==0 ) return 0;
	if( memcmp(buf1->data,buf2->data,buf1->used)==0 ) return 0;
	else return 1;
}

int buffer_copy( buffer_t *from, buffer_t *to )
{
	if( from==NULL ) return -1;
	if( to==NULL ) return -1;
	if( from->used>to->size ) return -1;
	if( to->data!=to->buf) to->data = to->buf;
	memcpy( to->data, from->data, from->used );
	to->used = from->used;
	return 0;
}
