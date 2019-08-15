#include "ssl_common.h"
#include "ssl.h"
#include "fdevents.h"
#include "log.h"
#include "readconf.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

struct certificate ca = { NULL, 0 },
                   srv = { NULL, 0 },
                   key = { NULL, 0 };
extern int forcepfx;

int init_ssl( connection_t *conn )
{
    conn->ssl = NULL;
    int ret = 0;
    int mode;

    connection *ssl = malloc( sizeof(connection) );
    if( NULL == ssl )
    {
        log_debug( "malloc ssl failed, %s", strerror(errno) );
        return -1;
    }
    memset( ssl, 0, sizeof(connection) );
    
    ssl->ssl_con = malloc( sizeof(con_ssl) );
    if( NULL == ssl->ssl_con )
    {
        log_debug( "malloc ssl_con faile, %s", strerror(errno) );
        goto errssl;
    }
    memset( ssl->ssl_con, 0, sizeof(con_ssl) );

    SSL_CTX *ctx = ssl_create();
    if( NULL == ctx )
    {
        log_debug( "ssl_create failed" );
        goto errcon;
    }
    ssl->ssl_con->ctx = ctx;

    mode = mode_frontend ? 1 : forcepfx;
    ret = parse_cert_p12( ctx, mode, conf[SSL_SERVER_CERTIFICATE].value.str, conf[SSL_SERVER_KEY].value.str );
    if( SSL_OK != ret )
    {
        log_debug( "ssl certificate failed" );
        goto errctx;
    }

    ret = ssl_ca_certificate( ctx, CLIENT_AUTH_NONE, conf[SSL_CA_CERTIFICATE].value.str, CA_DEPTH );
    if( SSL_OK != ret )
    {
        log_debug( "ssl ca certificate failed" );
        goto errctx;
    }


    mode = mode_frontend ? SSL_SERVER : SSL_CLIENT;

    ssl->ssl_con->ssl = ssl_create_connection( ctx, conn->fd, mode );
    if( NULL == ssl->ssl_con->ssl )
    {
        log_debug( "ssl create connection failed" );
        goto errctx;
    }


    conn->ssl = ssl;
    return 0;

errctx:
    ssl_cleanup_ctx( ctx );
errcon:
    free( ssl->ssl_con );
errssl:
    free( ssl );
    return -1;
}


void del_ssl( connection_t *conn )
{
    connection *ssl = conn->ssl;
    if( NULL != ssl )
    {
        if( NULL != ssl->ssl_con )
        {
            if( NULL != ssl->ssl_con->ctx )
                ssl_cleanup_ctx( ssl->ssl_con->ctx );
            if( NULL != ssl->ssl_con->ssl )
                SSL_free( ssl->ssl_con->ssl );
            free( ssl->ssl_con );
        }
        free( ssl );
        conn->ssl = NULL;
    }
}


int ssl_handle_handshake( connection_t *conn )
{
    int ret = ssl_handshake( conn->ssl );
    if( SSL_OK == ret )
    {
        log_debug( "ssl handshake success, fd:%d", conn->fd );
        return CONN_OK;
    }
    else if( SSL_AGAIN_WANT_READ == ret )
    {
        ret = ev_modfd( conn->fd, (conn->lsnevents & ~FDEVENT_OUT) | FDEVENT_IN, conn );
        if( FDEVENT_OK != ret )
            return CONN_ERR;
        return CONN_AGN;
    }
    else if( SSL_AGAIN_WANT_WRITE == ret )
    {
        ret = ev_modfd( conn->fd, (conn->lsnevents & ~FDEVENT_IN) | FDEVENT_OUT, conn );
        if( FDEVENT_OK != ret )
            return CONN_ERR;
        return CONN_AGN;
    }
    else
    {
        log_debug( "ssl handshake failed, fd:%d, err:%d", conn->fd, ret );
        return CONN_ERR;
    }
}


int ssl_writev( connection_t *conn, const struct iovec *iov, int iovcnt )
{
    int written = 0;
    int n = 0;
    int i = 0;
    connection *ssl = conn->ssl;

    for( i=0; i<iovcnt; i++ )
    {
        n = ssl_write( ssl->ssl_con, iov[i].iov_base, iov[i].iov_len );
        if( n <= 0 )
            break;
        written += n;
        if( n < iov[i].iov_len )
            break;
    }

    if( written > 0 )
        return written;
    return n;
}


int loadcert( const char *file, struct certificate *cert )
{
    int fd = open( file, O_RDONLY );
    if( -1 == fd )
    {
        log_debug( "open cert file %s failed, %s", file, strerror(errno) );
        return -1;
    }

    struct stat fs;
    int ret = fstat( fd, &fs );
    if( -1 == ret )
    {
        log_debug( "fstat file %s failed, %s", file, strerror(errno) );
        goto errclose;
    }

    unsigned char *m = mmap( NULL, fs.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0 );
    if( MAP_FAILED == m )
    {
        log_debug( "mmap file %s failed, %s", file, strerror(errno) );
        goto errclose;
    }
    close( fd );

    cert->cert = m;
    cert->len = fs.st_size;

    return 0;

errclose:
    close( fd );
    return -1;
}


int loadallcert()
{
    int ret = 0;
    ret = loadcert( conf[SSL_CA_CERTIFICATE].value.str, &ca );
    if( ret != 0 )
    {
        log_debug( "load ca cert failed" );
        return -1;
    }

    ret = loadcert( conf[SSL_SERVER_CERTIFICATE].value.str, &srv );
    if( ret != 0 )
    {
        log_debug( "load server cert failed" );
        return -1;
    }

    ret = loadcert( conf[SSL_SERVER_KEY].value.str, &key );
    if( ret != 0 )
    {
        log_debug( "load server key failed" );
        return -1;
    }

    return 0;
}


int ssl_toread( connection_t *conn )
{
    connection *sc = conn->ssl;
    if( NULL == sc )
        return 0;
    SSL *ssl = sc->ssl_con->ssl;

    return SSL_pending( ssl );
}
