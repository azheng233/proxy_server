#include "dns_parse.h"
#include "rbtree.h"
#include "http_parse.h"
#include "log.h"
#include "chunk.h"
#include "conn_peer.h"
#include "conn_common.h"
#include "job.h"
#include "fdevents.h"
#include "dnscache.h"
#include "backend_common.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

static rbtree_t dns_conn_tree;
static rbtree_node_t dns_tree_root = {0};
static uint32_t dns_conn_num = 0;
static uint32_t dns_query_key = 0;
extern connection_t *dns_conn;

static struct dns_readbuf_context dns_ctx = {{0},0};

int dns_wakeup_conn( connection_t *conn, unsigned char *ip )
{
    int ret = 0;
    struct backend_host *host = conn->context;
    if( NULL == host )
    {
        log_debug( "NULL backend host, fd:%d", conn->fd );
        return DNS_ERR;
    }

    in_addr_t ipaddr = *(in_addr_t *)ip;
    if( 0 == ipaddr )
    {
        log_debug( "dns echo 0, signal close to backend connection type:%d, fd:%d", conn->type, conn->fd );
        conn_signal_thisclose( conn );
        return DNS_OK;
    }

    //添加到dns缓存
    ret = dnscache_insert( host->name, host->len, ip );
    if( 0 != ret )
    {
        log_debug( "add dns cache failed, fd:%d", conn->fd );
    }

    ret = backend_conn_connect( conn, ipaddr );
    if( ret != CONN_OK )
    {
        log_debug( "backend connect failed, signal close to backend connection type:%d, fd:%d", conn->type, conn->fd );
        conn_signal_thisclose( conn );
        return DNS_ERR;
    }
    if (CONTYPE_SSL_BACKEND == conn->type) {
        if (!ssl_use_client_cert) {
            if (conn->status == CONSTAT_WORKING)
                conn->status = CONSTAT_HANDSHAKE;
        }
    }
    return DNS_OK;
}

int dns_handle_pkt( unsigned char *pkt )
{
    uint32_t key = *(uint32_t *)pkt;
    unsigned char *ip = pkt + sizeof(uint32_t);

    log_debug( "handle dns echo packet %u, %hhu.%hhu.%hhu.%hhu", key, ip[0], ip[1], ip[2], ip[3] );

    rbtree_node_t *node = rbtree_find( &dns_conn_tree, key );
    if( NULL == node )
    {
        log_debug( "connection not found, key:%u", key );
        return -1;
    }

    connection_t *conn = node->data;
    if( NULL == conn )
    {
        log_debug( " connection is NULL with key %u", key );
        return -1;
    }
    log_debug( "connection %d found, key:%u", conn->fd, key );

    int ret = dns_wakeup_conn( conn, ip );
    dns_delconn( conn );
    return ret;
}

int dns_parse_echo( connection_t *conn )
{
    int tmpleft = 0,
        cplen = 0;
    unsigned char *pkt;

    chunk_t *ck = conn->readbuf;
    buffer_t *buf = buffer_getfirst( ck );

    for( ; NULL != buf; buf = buffer_getfirst( ck ) )
    {
        if( dns_ctx.intmp )
        {
            tmpleft = DNS_ECHO_LEN - ( dns_ctx.tmpos - dns_ctx.tmp );
            cplen = tmpleft < buf->used ? tmpleft : buf->used;
            memcpy( dns_ctx.tmpos, buf->data, cplen );
            dns_ctx.tmpos += cplen;
            buf->data += cplen;
            buf->used -= cplen;
            if( buf->used <= 0 )
            {
                buffer_get( ck );
                buffer_del( buf );
            }
            if( dns_ctx.tmpos - dns_ctx.tmp >= DNS_ECHO_LEN )
            {
                dns_ctx.intmp = 0;
                pkt = dns_ctx.tmp;
                dns_handle_pkt( pkt );
            }
            else continue;
        }
        else
        {
            if( buf->used < DNS_ECHO_LEN )
            {
                memcpy( dns_ctx.tmp, buf->data, buf->used );
                dns_ctx.tmpos = dns_ctx.tmp + buf->used;
                dns_ctx.intmp = 1;
                buffer_get( ck );
                buffer_del( buf );
                continue;
            }
            else
            {
                pkt = buf->data;
                buf->data += DNS_ECHO_LEN;
                buf->used -= DNS_ECHO_LEN;
                dns_handle_pkt( pkt );
                if( buf->used <= 0 )
                {
                    buffer_get( ck );
                    buffer_del( buf );
                }
            }
        }
    }
    return 0;
}


int dns_tree_init()
{
    rbtree_init( &dns_conn_tree, &dns_tree_root );
    dns_query_key = 1;
    return 0;
}


static int dns_tree_addconn( connection_t *conn, uint32_t key )
{
    conn->rbnode = malloc( sizeof(rbtree_node_t) );
    if( NULL == conn->rbnode )
    {
        log_debug( "malloc dns tree node failed, %s, connection:%d", strerror(errno), conn->fd );
        return DNS_ERR;
    }

    conn->rbnode->key = key;
    conn->rbnode->left = NULL;
    conn->rbnode->right = NULL;
    conn->rbnode->parent = NULL;
    conn->rbnode->color = 0;
    conn->rbnode->data = conn;

    rbtree_insert( &dns_conn_tree, conn->rbnode );
    return DNS_OK;
}

static int dns_send_query( const char *host, uint16_t hostlen, uint32_t key )
{
    buffer_t *buf = buffer_new( 7+hostlen );
    if( NULL == buf )
    {
        log_debug( "alloc new dns query buffer failed" );
        return DNS_ERR;
    }
/*
typedef struct _server_name //需要解析的域名数据格式
{
        uint8                   type;
        uint32                  serial;
        uint16                  name_len;
        unsigned char           *name;
} server_name_t;
*/
    unsigned char *p = buf->data;
    *p++ = 0;
    *(uint32_t *)p = key;
    p += sizeof(uint32_t);
    *(uint16_t *)p = htons( hostlen );
    p += sizeof(uint16_t);
    memcpy( p, host, hostlen );
    p += hostlen;

    buf->used = p - buf->data;
    buffer_put( dns_conn->writebuf, buf );

    if( job_addconn( FDEVENT_OUT, dns_conn ) != JOB_OK )
    {
        buffer_del( buf );
        return DNS_ERR;
    }
    return DNS_OK;
}

int dns_query( connection_t *conn, const char *name, uint16_t namelen )
{
    if( NULL==conn || NULL==name || 0==name[0] || 0==namelen )
        return DNS_ERR;

    int ret = 0;
    ret = backend_set_host( conn, name, namelen );
    if( 0 != ret )
        return CONN_ERR;

    ret = dns_tree_addconn( conn, dns_query_key );
    if( DNS_OK != ret )
        goto err;

    ret = dns_send_query( name, namelen, dns_query_key );
    if( DNS_OK != ret )
        goto err;

    dns_conn_num++;
    dns_query_key++;
    if( unlikely( 0==dns_query_key ) )
        dns_query_key = 1;
    return DNS_OK;

err:
    dns_delconn( conn );
    return DNS_ERR;
}


void dns_delconn( connection_t *conn )
{
    if( NULL != conn->rbnode )
    {
        rbtree_delete( &dns_conn_tree, conn->rbnode );
        dns_conn_num--;
        free( conn->rbnode );
        conn->rbnode = NULL;
    }
    if( NULL != conn->context )
        backend_del_host( conn );
}
