/** 
 * @file buffer.h
 * @brief 缓冲区
 * @author sunxp
 * @version 0.1
 * @date 2010-07-08
 */
#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "queue.h"

#define BUFFER_SIZE_DEFAULT  2048
#define BUFFER_SIZE_MIN  64

#define BUFFER_FLAG_PROCED  (1<<0)

#define BUFFER_FLAG_SPLIT   (1<<8)

typedef struct buffer_st {
    unsigned char *buf;
    size_t size;
    unsigned short flags;
    unsigned char *data;
    size_t used;
    queue_t queue;
} buffer_t;


buffer_t *buffer_new( const size_t size );
void buffer_del( buffer_t *buf );


void buffer_setflag( buffer_t *buf, unsigned short flag );
void buffer_unsetflag( buffer_t *buf, unsigned short flag );
int buffer_testflag( buffer_t *buf, unsigned short flag );


#define buffer_mused( buffer ) \
    ( (buffer)->data - (buffer)->buf + (buffer)->used )

#define buffer_spare( buffer ) \
    ( (buffer)->size - buffer_mused( buffer ) )

#define buffer_getwpos( buffer ) \
    ( (buffer)->data + (buffer)->used )

int buffer_resize( buffer_t *buf, size_t newsize );

void buffer_reset( buffer_t *buf );

int buffer_cmp( buffer_t *buf1, buffer_t *buf2 );

int buffer_copy( buffer_t *from, buffer_t *to );

#endif
