#include "fdrw.h"
#include "fdevents.h"
#include "connection.h"
#include "log.h"
#include "bufqu.h"
#include "queue.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define FDRW_WRITE_CNT  128

void rwbuf_free( connection_t *conn )
{
    if( conn->rbuf.buf ) free( conn->rbuf.buf );
    conn->rbuf.buf = NULL;
    if( conn->wbuf.bufqu ) 
    {
        buffer_queue_empty( conn->wbuf.bufqu );
        buffer_queue_destroy( conn->wbuf.bufqu );
    }
    conn->wbuf.bufqu = NULL;
}

int rwbuf_init( connection_t *conn, size_t rsize )
{
    conn->rbuf.buf = NULL;
    conn->wbuf.bufqu = NULL;

    conn->rbuf.buf = (unsigned char *)malloc( rsize );
    if( conn->rbuf.buf==NULL ) goto err0;
    conn->rbuf.used = 0;
    conn->rbuf.pos = 0;
    conn->rbuf.size = rsize;

    conn->wbuf.bufqu = buffer_queue_create( NULL );
    if( conn->wbuf.bufqu==NULL ) goto err1;

    return FDRW_OK;
err0:
    return FDRW_OTH;
err1:
    free( conn->rbuf.buf );
    conn->rbuf.buf = NULL;
    return FDRW_OTH;
}

static int readfd_common_pos( connection_t *conn, int pos )
{
    int toread;
    int readnum;	
    int bufleft;
    int isreadall;

    //参数检查
    if( conn==NULL ) goto errother;
    if( conn->status!=CONSTAT_WORKING ) goto errother;
    if( conn->rbuf.buf==NULL ) goto errother;

    //判断有多少可以读取
    toread = 0;	
    if( ioctl(conn->fd, FIONREAD, &toread)<0 ) goto errnet;
    log_trace("toread=%d, fd %d", toread, conn->fd);
    if( toread==0 ) goto errclose;
    if( toread<0 ) goto errnet;

    //内存前移
    if( conn->rbuf.pos!=0 && conn->rbuf.used!=0 )
    {
        log_debug("memory go prev");
        memcpy( conn->rbuf.buf+pos, conn->rbuf.buf+conn->rbuf.pos, conn->rbuf.used );
    }
    conn->rbuf.pos = pos;
    //计算接收缓存大小
    isreadall = 1;
    bufleft = conn->rbuf.size-conn->rbuf.used-pos;
    if( bufleft==0 ) goto errmore;
    if( bufleft<toread ) 
    {
        toread = bufleft;
        isreadall = 0;
    }

    //进行读取并判断
    readnum = read( conn->fd, conn->rbuf.buf+pos+conn->rbuf.used, toread );
    log_trace("readnum=%d, fd %d", readnum, conn->fd);

    if( readnum>0 )
    {
        conn->rbuf.used += readnum;//conn->rbuf.size;
        conn->upsize +=readnum;
    }
    else if( readnum<0 )
    {
        if (errno == EAGAIN) goto erragain;
        if (errno == EINTR) goto erreintr;
        goto errnet;
    }else if( readnum==0 )
    {
        goto errclose;
    }

    //处理返回值
    if( isreadall==1 ) return FDRW_OK;
    else return FDRW_MOR;//有更多的数据要读

    return FDRW_OK;
errmore:
    return FDRW_MOR;

errnet:
    log_warn("errnet");
    return FDRW_PCK;

erragain:
    log_warn("r erragain");
    return FDRW_AGN;

erreintr:
    log_warn("r erreintr");
    return FDRW_AGN;

errclose:
    log_warn("errclose");
    return FDRW_CLS;
errother:
    log_warn("errother");
    return FDRW_OTH;
}

int readfd_common( connection_t *conn )
{
    return readfd_common_pos( conn, 0 );
}

static int writefd_common( connection_t *conn, int pos )
{
    struct iovec iov[FDRW_WRITE_CNT];
    writebuf_t *wb;
    int bufcnt;
    int towrite;
    int i;
    queue_t *qu;
    buffer_qu_t *b;
    int writecnt;

    if( conn==NULL ) goto errother;
    if( conn->status!=CONSTAT_WORKING ) goto errother;
    if( conn->wbuf.bufqu==NULL ) goto errother;
    wb = &(conn->wbuf);
    bufcnt = buffer_queue_num( wb->bufqu );
    if( bufcnt<=0 ) goto errnodata;

    towrite = FDRW_WRITE_CNT;
    if( towrite>bufcnt ) towrite=bufcnt;

    qu = wb->bufqu->queue.next;
    for( i=0; i<towrite; i++ )
    {
        b = (queue_data( qu,buffer_qu_t, queue ));
        iov[i].iov_base = b->buf + pos;
        iov[i].iov_len = b->used - pos ;
        //log_packet( LOG_LEVEL_TRACE, "data send:", b->buf+pos,b->used-pos );
        qu = qu->next;
    }

    writecnt = writev( conn->fd, iov, towrite );

    log_trace("real write %d, fd %d", writecnt, conn->fd);
    if( writecnt<0 )
    {
        if (errno == EAGAIN)
            goto erragain;
        if (errno == EINTR)
            goto erreintr;
        log_error( "writev return %d, error %d, %s", writecnt, errno, strerror(errno) );
        goto errnet;
    }else if( writecnt==0 ) {
        goto erragain;
    }else if( writecnt>0 ) {
        conn->downsize +=writecnt;
        for( i=0; i<towrite; i++ )
        {
            qu = wb->bufqu->queue.next;
            b = (queue_data( qu,buffer_qu_t, queue ));
            if( (unsigned int)writecnt>=(b->used-pos) ) 
            {
                writecnt -= (b->used-pos);
                b = buffer_get_head( wb->bufqu, NULL );
                buffer_qu_del( b );
            }
            else
            {
                b->used -= writecnt;
                memcpy( (b->buf+pos), b->buf+writecnt, (b->used-pos) );
                goto errmore;
            }
        }
    }
    return FDRW_OK;
errnodata:
    return FDRW_NOD;

errnet:
    log_warn("errnet");
    return FDRW_PCK;

erragain:
    log_warn("r erragain");
    return FDRW_AGN;

erreintr:
    log_warn("r erreintr");
    return FDRW_AGN;

errother:
    log_warn("errother");
    return FDRW_OTH;
errmore:
    return FDRW_MOR;
}

static int writefd_handle_common_pos( connection_t *conn, int pos )
{
    int ret;
    ret = writefd_common( conn, pos );
    switch(ret)
    {
        case FDRW_AGN:
        case FDRW_MOR:
            ev_modfd( conn->fd, conn->lsnevents|FDEVENT_OUT, conn );
            break;
        case FDRW_NOD:
        case FDRW_OK:
            if( conn->lsnevents&FDEVENT_OUT )
                ev_modfd( conn->fd, conn->lsnevents^FDEVENT_OUT, conn );
            break;	
        default:
            goto err;
    }
    if( ret==FDRW_AGN) return FDRW_AGN;
    else return FDRW_OK;
err:
    return FDRW_OTH;
}

int writefd_handle_common( connection_t *conn )
{
    return writefd_handle_common_pos( conn, 0 );
}

