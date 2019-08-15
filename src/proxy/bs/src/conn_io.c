#include "conn_io.h"
#include "conn_peer.h"
#include "chunk.h"
#include "fdevents.h"
#include "waitconn.h"
#include "conn_io.h"
#include "timehandle.h"
#include "log.h"
#include "forward.h"
#include "ssl_common.h"
#include "ssl.h"
#include "job.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static int chunk_io_read( connection_t *conn )
{
    // 系统缓冲区有多少可读
    int buf_sys = 0;
    if( ioctl( conn->fd, FIONREAD, &buf_sys ) < 0 )
    {
        log_error( "errnet, ioctl failed, %s", strerror(errno) );
        return IOERR_OTHER;
    }
    log_trace( "data in system buffer is %d", buf_sys );
    if( 0 == buf_sys )
        return IOERR_CLOSE;

    // buffer space
    chunk_t *ckr = conn->readbuf;
    int this_size = ckr->limit - ckr->used_size;
    log_trace( "this connection %d has %d spaces", conn->fd, this_size );
    // peer space
    connection_t *peer = conn->peer;
    int peer_size = 0;
    if( NULL != peer )
    {
        chunk_t *ckw = peer->writebuf;
        peer_size = ckw->limit - ckw->used_size;
        log_trace( "peer connection %d has %d spaces", peer->fd, peer_size );
    }

    // 缓冲区还有多少空间，包括本端读缓冲和对端写缓冲
    int buf_wsize = peer_size + this_size;
    // 控制流量，防止过大缓冲占用内存
    if( buf_wsize <= 0 )
        return IOERR_FULL;

    int toread = buf_sys < buf_wsize ? buf_sys : buf_wsize;
    log_trace( "data to read is %d", toread );

    buffer_t *buf = buffer_new( toread );
    if( NULL == buf )
    {
        log_warn( "alloc new read buffer failed, size %u", toread );
        return IOERR_MEM;
    }

    unsigned char *rpos = buffer_getwpos( buf );
    int rsize = buffer_spare( buf );

    // read
    int readn = read( conn->fd, rpos, rsize );
    log_debug( "read %d in %d", readn, toread );
    if( readn <= 0 )
    {
        int errnum = errno;
        buffer_del( buf );
        if( readn == 0 )
            return IOERR_CLOSE;
        if( EAGAIN==errnum || EINTR==errnum )
            return IOERR_AGN;
        return IOERR_OTHER;
    }

    buf->used += readn;
    conn->upsize += readn;
    conn->mlog_up_flow += readn;
    buffer_put( conn->readbuf, buf );

    if( readn < buf_sys )
        return IOERR_MOR;
    return IOOK;
}

/*
 *ypf modified
 */
static int chunk_ssl_read( connection_t *conn )
{
    // buffer space
    chunk_t *ckr = conn->readbuf;
    int this_size = ckr->limit - ckr->used_size;
    log_trace( "this connection %d has %d spaces", conn->fd, this_size );
    // peer space
    connection_t *peer = conn->peer;
    int peer_size = 0;
    if( NULL != peer )
    {
        chunk_t *ckw = peer->writebuf;
        peer_size = ckw->limit - ckw->used_size;
        log_trace( "peer connection %d has %d spaces", peer->fd, peer_size );
    }

    // 缓冲区还有多少空间，包括本端读缓冲和对端写缓冲
    // int buf_wsize = peer_size + this_size;
    // 控制流量，防止过大缓冲占用内存
    // if( buf_wsize <= 0 )
    //     return IOERR_FULL;

    buffer_t *buf;
    uint8_t *rpos;
    int readn;
    int total;
    int retn;
    connection* sslconn = conn->ssl;
    con_ssl *ssl = sslconn->ssl_con;

    buf = NULL;
    total = 0;

    for(;;)
    {
        if( NULL == buf )
        {
            buf = buffer_new( SSL_READ_BUFFER_MIN_SIZE );
            if( NULL == buf )
            {
                log_warn( "alloc new buffer for ssl read failed, size %d", SSL_READ_BUFFER_MIN_SIZE );
                return IOERR_MEM;
            }
        }

        rpos = buf->data;
        retn = IOOK;

        readn = ssl_recv( ssl, rpos, SSL_READ_BUFFER_MIN_SIZE );
        if( readn <= 0 )
        {
            log_debug( "ssl read err return %d", readn );
            if( SSL_AGAIN_WANT_READ == readn )
            {
                retn = IOERR_AGN;
                goto sslrecv_err;
            }
            if( SSL_AGAIN_WANT_WRITE == readn )
            {
                sslconn->wait = read_waiton_write;
                retn = IOERR_RW;
                goto sslrecv_err;
            }
            if( SSL_CLOSE == readn )
            {
                retn = IOERR_CLOSE;
                goto sslrecv_err;
            }
            retn = IOERR_OTHER;
            goto sslrecv_err;
        }

sslrecv_err:
        if( retn != IOOK )
        {
            buffer_del( buf );
            log_debug( "ssl read len %d", total );
            return retn;
        }

        total += readn;
        buf->used = readn;
        // append to read buffer chain
        conn->upsize += readn;
        conn->mlog_up_flow += readn;
        buffer_put( conn->readbuf, buf );
        buf = NULL;
    }
}

/*
static int chunk_ssl_read( connection_t *conn )
{
    // buffer space
    chunk_t *ckr = conn->readbuf;
    int this_size = ckr->limit - ckr->used_size;
    log_trace( "this connection %d has %d spaces", conn->fd, this_size );
    // peer space
    connection_t *peer = conn->peer;
    int peer_size = 0;
    if( NULL != peer )
    {
        chunk_t *ckw = peer->writebuf;
        peer_size = ckw->limit - ckw->used_size;
        log_trace( "peer connection %d has %d spaces", peer->fd, peer_size );
    }

    // 缓冲区还有多少空间，包括本端读缓冲和对端写缓冲
    // int buf_wsize = peer_size + this_size;
    // 控制流量，防止过大缓冲占用内存
    // if( buf_wsize <= 0 )
    //     return IOERR_FULL;

    int toread = ssl_toread( conn );
    log_trace( "data in ssl buffer is %d", toread );
    if( toread < SSL_READ_BUFFER_MIN_SIZE )
        toread = SSL_READ_BUFFER_MIN_SIZE;

    buffer_t *buf = NULL;
    uint8_t *rpos;
    int rsize;
    int readn;
    int total = 0;
    connection* sslconn = conn->ssl;
    con_ssl *ssl = sslconn->ssl_con;

    for(;;)
    {
        if( NULL == buf )
        {
            buf = buffer_new( toread );
            if( NULL == buf )
            {
                log_warn( "alloc new buffer for ssl read failed, size %u", toread );
                return IOERR_MEM;
            }
        }
        rpos = buf->data;
        rsize = toread;

        readn = ssl_recv( ssl, rpos, rsize );
        log_debug( "ssl read %d of %d", readn, rsize );
        if( readn <= 0 )
        {
            buffer_del( buf );
            if( total > 0 )
                return IOOK;
            if( SSL_AGAIN_WANT_READ == readn )
                return IOERR_AGN;
            if( SSL_AGAIN_WANT_WRITE == readn )
            {
                sslconn->wait = read_waiton_write;
                return IOERR_RW;
            }
            if( SSL_CLOSE == readn )
                return IOERR_CLOSE;

            return IOOK;
            //return IOERR_OTHER;
        }

        total += readn;
        buf->used += readn;
        // append to read buffer chain
        conn->upsize += readn;
        conn->mlog_up_flow += readn;
        buffer_put( conn->readbuf, buf );
        buf = NULL;

//        if( total >= buf_wsize )
//        {
//            log_debug( "total %d bigger than buffer size %d", total, buf_wsize );
//            return IOERR_FULL;
//        }

        toread = ssl_toread( conn );
    }
}
*/

int conn_handle_in( connection_t *conn, fun_handle_parse parse_handle )
{
    if( CONSTAT_WORKING != conn->status && CONSTAT_HTTPCONNECT != conn->status )
    {
        log_warn( "conn status %d, fd:%d", conn->status, conn->fd );
        goto errstat;
    }
    if( NULL == conn->readbuf )
    {
        log_warn( "conn readbuf is NULL, fd:%d", conn->fd );
        goto errbuf;
    }

    connection_t *peer;
    int ret;
    int isssl = ( CONTYPE_SSL==conn->type || CONTYPE_SSL_BACKEND==conn->type ) \
                && CONSTAT_HTTPCONNECT != conn->status;

#ifndef _DEBUG_NOSSL
    if( isssl && !ssl_use_client_cert )
        ret = chunk_ssl_read( conn );
    else
#endif
        ret = chunk_io_read( conn );
    switch( ret )
    {
        case IOERR_FULL:    //FIXME 其他连接
            peer = conn->peer;
            if( NULL != peer )
            {
                ret = waitconn_add( peer, conn, FDEVENT_IN, WAITEVENT_CANWRITE );
                log_debug( "connection %d wait %d, return %d", conn->fd, peer->fd, ret );
                if( ! isssl || ssl_use_client_cert )
                {
                    ret = ev_modfd( conn->fd, conn->lsnevents & ~FDEVENT_IN, conn );
                    log_debug( "del eventin from epoll, connection %d, return %d", conn->fd, ret );
                }
            }
            else log_debug( "connection %d has no peer", conn->fd );
            break;

        case IOOK:
        case IOERR_MOR:
        case IOERR_AGN:
        case IOERR_RW:
            ret = ev_modfd( conn->fd, conn->lsnevents | FDEVENT_IN, conn );
            break;
/*
        case IOERR_AGN:
            ret = ev_modfd( conn->fd, conn->lsnevents | FDEVENT_IN, conn );
            goto errok;

        case IOERR_RW:
            ret = ev_modfd( conn->fd, (conn->lsnevents & ~FDEVENT_IN) | FDEVENT_OUT, conn );
            log_debug( "ssl connection %d want to write, ret:%d", conn->fd, ret );
            goto errok;;
*/
        case IOERR_CLOSE:
            goto errclose;
        default:
            goto err;
    }

    ret = parse_handle( conn );
    if( ret != 0 )
    {
	    log_debug( "parse return %d, fd:%d", ret, conn->fd );
	    goto err;
    }

    return CONN_OK;

errstat:
errbuf:
errclose:
err:
    return CONN_ERR;
}


static int chunk_io_write( connection_t *conn )
{
    struct iovec iov[CONN_WRITE_CNTMAX];
    int n = 0;
    int errnum;

    buffer_t *buf = buffer_getfirst( conn->writebuf );
    while( NULL != buf )
    {
        iov[n].iov_base = buf->data;
        iov[n].iov_len = buf->used;
        n++;
        if( n >= CONN_WRITE_CNTMAX )
            break;
        buf = buffer_getnext( conn->writebuf, buf );
    }
    if( 0 == n )
    {
        log_debug( "buffer is empty, nothing write" );
        return IOERR_EMPTY;
    }

    int written = 0;
    written = writev( conn->fd, iov, n );
    errnum = errno;
    log_trace( "connection %d written %d", conn->fd, written );
    if( written < 0 )
    {
        log_debug( "written %d, %s", written, strerror(errno) );
        if( EAGAIN == errnum )
            goto erragn;
        if( EINTR == errnum )
            goto erritr;
        goto err;
    }
    else if( 0 == written )
    {
        log_debug( "written %d, %s", written, strerror(errno) );
        goto erragn;
    }

    conn->downsize += written;
    conn->mlog_down_flow += written;
    do {
        buf = buffer_getfirst( conn->writebuf );
        if( NULL == buf )
            break;
        if( written >= buf->used )
        {
            buffer_get( conn->writebuf );
            written -= buf->used;
            buffer_del( buf );
        }
        else
        {
            buf->data += written;
            buf->used -= written;
            goto errmor;
        }
    } while( written > 0 );

    if( ! chunk_isempty( conn->writebuf ) )
        goto errmor;
    return IOOK;

erragn:
erritr:
    return IOERR_AGN;
errmor:
    return IOERR_MOR;
err:
    return IOERR_OTHER;
}
/*
 * ypf modified
 */
static int chunk_ssl_write( connection_t *conn )
{
    int written = 0;
    int allwritten = 0;
    connection *ssl = conn->ssl;
    if( NULL == ssl )
        return IOERR_OTHER;

    buffer_t *buf = buffer_getfirst( conn->writebuf );
    if( NULL != buf )
    {
        if( 0 != chunk_merge_buffer( conn->writebuf, buf, ((chunk_t *)conn->writebuf)->used_size ) )
        {
            log_error( "merge error fd:%d", conn->fd );
            return IOERR_OTHER;
        }

        for(;allwritten<buf->used;)
        {
            written = ssl_write( ssl->ssl_con, buf->data+allwritten, buf->used-allwritten );
            if( written <= 0 )
            {
                written = ssl_write( ssl->ssl_con, buf->data+allwritten, buf->used-allwritten );
                if( written <= 0 )    break;
            }
            allwritten += written;
        }

        conn->downsize += allwritten;
        conn->mlog_down_flow += allwritten;
        log_debug( "data allwritten %d, fd:%d", allwritten, conn->fd );
        log_debug( "data res in writebuf %d before erase, fd:%d", ((chunk_t *)conn->writebuf)->used_size, conn->fd );
        chunk_erase( conn->writebuf, allwritten );
        log_debug( "data res in writebuf %d, fd:%d", ((chunk_t *)conn->writebuf)->used_size, conn->fd );

        if( written <= 0 )
        {
            log_debug( "ssl write return %d, fd:%d", written, conn->fd );
            if( SSL_AGAIN_WANT_WRITE == written )
                //goto erragn;
                goto errrw;
            if( SSL_AGAIN_WANT_READ == written )
            {
                ssl->wait = write_waiton_read;
                goto errrw;
            }
            if( SSL_CLOSE == written )
                goto errclose;
            goto err;
        }
    }

    if( ! chunk_isempty( conn->writebuf ) )
        goto errmor;
    return IOOK;

errmor:
    return IOERR_MOR;
errrw:
    return IOERR_RW;
errclose:
    return IOERR_CLOSE;
err:
    return IOERR_OTHER;
}
/*
static int chunk_ssl_write( connection_t *conn )
{
    int n = 0;
    int written = 0;
    connection *ssl = conn->ssl;
    if( NULL == ssl )
        return IOERR_OTHER;

    buffer_t *buf = buffer_getfirst( conn->writebuf );
    while( NULL != buf )
    {
        written = ssl_write( ssl->ssl_con, buf->data, buf->used );
        if( written <= 0 )
        {
            log_debug( "ssl write return %d, fd:%d", written, conn->fd );
            if( SSL_AGAIN_WANT_WRITE == written )
                goto erragn;
            if( SSL_AGAIN_WANT_READ == written )
            {
                ssl->wait = write_waiton_read;
                goto errrw;
            }
            if( SSL_CLOSE == written )
                goto errclose;
            goto err;
        }

        conn->downsize += written;
        conn->mlog_down_flow += written;
        n++;

        if( written < buf->used )
        {
            buf->data += written;
            buf->used -= written;
            goto errmor;
        }

        buffer_get( conn->writebuf );
        buffer_del( buf );

        if( n >= CONN_WRITE_CNTMAX )
            break;
        buf = buffer_getfirst( conn->writebuf );
    }
    if( 0 == n )
    {
        log_debug( "buffer is empty, nothing write" );
        return IOERR_EMPTY;
    }

    if( ! chunk_isempty( conn->writebuf ) )
        goto errmor;
    return IOOK;

erragn:
    return IOERR_AGN;
errmor:
    return IOERR_MOR;
errrw:
    return IOERR_RW;
errclose:
    return IOERR_CLOSE;
err:
    return IOERR_OTHER;
}
*/
int conn_handle_out( connection_t *conn )
{
    if( CONSTAT_WORKING != conn->status && \
            CONSTAT_PEERCLOSED != conn->status && \
            CONSTAT_CLOSING_WRITE != conn->status && \
            CONSTAT_HTTPCONNECT != conn->status )
    {
        log_warn( "conn status %d, fd:%d", conn->status, conn->fd );
        goto errstat;
    }
    if( NULL == conn->writebuf )
    {
        log_warn( "conn readbuf is NULL, fd:%d", conn->fd );
        goto errbuf;
    }

    int ret;
#ifndef _DEBUG_NOSSL
    if ((CONTYPE_SSL == conn->type || CONTYPE_SSL_BACKEND == conn->type))
        if (ssl_use_client_cert || CONSTAT_HTTPCONNECT == conn->status) {
            ret = chunk_io_write(conn);
        } else {
            ret = chunk_ssl_write(conn);
        } else
#endif
            ret = chunk_io_write(conn);
    log_trace( "chunk write return %d", ret );
    if( IOOK != ret )
    {
        switch( ret )
        {
            case IOERR_EMPTY:
                break;
            case IOERR_AGN:
            case IOERR_MOR:
                ret = ev_modfd( conn->fd, conn->lsnevents | FDEVENT_OUT, conn );
                log_debug( "mod connection %d, event 0x%x, return %d", conn->fd, conn->lsnevents, ret );
                return CONN_AGN;
            case IOERR_RW:
                //ret = ev_modfd( conn->fd, (conn->lsnevents & ~FDEVENT_OUT) | FDEVENT_IN, conn );
                ret = ev_modfd( conn->fd, conn->lsnevents | FDEVENT_OUT | FDEVENT_IN, conn );
                log_debug( "ssl connection %d want to read, ret:%d", conn->fd, ret );
                //TODO peerclose in handle
                return CONN_AGN;
            default:
                goto err;
        }
    }

    // 缓冲区数据全发完，从epoll中删除可写事件，防止接下来频繁触发
    if( conn->lsnevents & FDEVENT_OUT )
    {
        log_debug( "del eventout from epoll, connection %d", conn->fd );
        ev_modfd( conn->fd, conn->lsnevents & ~FDEVENT_OUT, conn );
    }
    return CONN_OK;

errstat:
errbuf:
err:
    return CONN_ERR;
}
