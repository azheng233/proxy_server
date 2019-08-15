#include  "ssl.h"
#include "log.h"
#include <errno.h>
#include "card.h"
#include "jit_card.h"

int  ssl_connection_index;
int  ssl_server_conf_index;
int  ssl_session_cache_index;

static int ssl_verify_callback(int ok, X509_STORE_CTX *x509_store);


//print error and exit
void err_exit( char *string){
    //fprintf(stderr,"%s\n",string);
    log_debug( "%s", string );
  }

/* Print SSL errors and exit*/
void berr_exit(char *string){
   // BIO_printf(stdout,"%s\n",string);
   log_debug( "%s", string );
    //exit(0);
  }



/********************************************************* 
	函数名称：ssl_init
	功能描述：加载SSL库函数，设置ssl所需环境变量
	参数列表：NULL
	返回结果：成功返???SSL_OK
			  失败返回 SSL_ERROR
*******************************************************/
int ssl_init()
{
//    OPENSSL_config(NULL);

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

//    ENGINE_load_builtin_engines();


    ssl_connection_index = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);

    if (ssl_connection_index == -1) {
        err_exit( "SSL_get_ex_new_index() failed");
		return SSL_ERROR;
    }

    
	ssl_server_conf_index = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL,
                                                         NULL);
    if (ssl_server_conf_index == -1) {
        err_exit( "SSL_CTX_get_ex_new_index() failed");
        return SSL_ERROR;
    }

    ssl_session_cache_index = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL,
                                                           NULL);
    if (ssl_session_cache_index == -1) {
        err_exit("SSL_CTX_get_ex_new_index() failed");
        return SSL_ERROR;
    }

    return SSL_OK;
}

void ssl_unload()
{
    CONF_modules_free();
}

/********************************************************* 
	函数名称：ssl_creat
	功能描述：生成ssl数据结构，并设置各项参数???	参数列表???	返回结果：成功返???SSL_CTX结构
		
*******************************************************/
SSL_CTX *ssl_create()
{	
	SSL_CTX *ctx;
    ctx = SSL_CTX_new(SSLv23_method());

    if (ctx == NULL) {
        err_exit("SSL_CTX_new() failed");
        return NULL;
    }



    /* client side options */

    SSL_CTX_set_options(ctx, SSL_OP_MICROSOFT_SESS_ID_BUG);
    SSL_CTX_set_options(ctx, SSL_OP_NETSCAPE_CHALLENGE_BUG);
    SSL_CTX_set_options(ctx, SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG);

    /* server side options */

    SSL_CTX_set_options(ctx, SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG);
    SSL_CTX_set_options(ctx, SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);

    /* this option allow a potential SSL 2.0 rollback (CAN-2005-2969) */
    SSL_CTX_set_options(ctx, SSL_OP_MSIE_SSLV2_RSA_PADDING);

    SSL_CTX_set_options(ctx, SSL_OP_SSLEAY_080_CLIENT_DH_BUG);
    SSL_CTX_set_options(ctx, SSL_OP_TLS_D5_BUG);
    SSL_CTX_set_options(ctx, SSL_OP_TLS_BLOCK_PADDING_BUG);

    SSL_CTX_set_options(ctx, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);

    SSL_CTX_set_options(ctx, SSL_OP_SINGLE_DH_USE);


    SSL_CTX_set_read_ahead(ctx, 1);


    SSL_CTX_set_mode( ctx, SSL_MODE_ENABLE_PARTIAL_WRITE );
    SSL_CTX_set_mode( ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER );

    return ctx;
}


static int get_card_cert( unsigned char *cert, int *len )
{
    int ret;

    ret = get_cert( cert, len );
    if( ret )
    {
        log_debug( "read cert from pci card success, len %d", *len );
        return 0;
    }

#ifndef NO_JIT
    ret = get_jit_cert( cert, len );
    if( RET_PAI_OK == ret )
    {
        log_debug( "read cert from jit card success, len %d", *len );
        return 0;
    }
#endif

    return -1;
}

/*****************************************************
??????:parse_cert_p12
????????:????p12֤???Ĺ?Կ֤????˽Կ
????: [in1]unsigned char *p12_cert_path (p12 format cert path)
          
             [in2]char *psd_path (password pathfor p12)
             * @param[in] mode 1:从文件读，前端机用；0:读卡证书，后端机用
????ֵ??:success 0 ,failed -1

*****************************************************/
int parse_cert_p12( SSL_CTX *ctx, int mode, char *p12_cert_path, char *psd_path )
{
    static int first = 1;
    static X509 *cert = NULL;
    static EVP_PKEY *ckey = NULL;

    char password[64];
    int ret;

    FILE *f;//??P12֤??????ȡ˽Կ		
    FILE *filep;

    PKCS12 *p12;

    if( first )
    {
        if( mode )
        {
            f = fopen(p12_cert_path,"rb");
            if (f == NULL)
            {
                err_exit("open p12 cert failed");
                return -1;
            }
            p12 = d2i_PKCS12_fp(f, NULL);
            fclose(f);
        }
        else
        {
            unsigned char cert[8192];
            //const unsigned char *p = cert;
            int len = sizeof(cert);

            ret = get_card_cert( cert, &len );
            if( ret != 0 )
            {
                log_error( "read cert from card failed" );
                return -1;
            }
            log_debug("read cert from card len %d", len);
            //d2i_PKCS12( &p12, &p, len );
            f = fopen("/tmp/tmp.key","wb+");
            if (f == NULL)
            {
                log_error("open for write tmp cert failed");
                return -1;
            }
            ret = fwrite(cert, 1, len, f);
            if (ret != len)
            {
                log_error("write tmp cert failed");
                fclose(f);
                remove("/tmp/tmp.key");
                return -1;
            }
            fclose(f);
            f = fopen("/tmp/tmp.key","rb");
            if (f == NULL)
            {
                log_error("open tmp cert failed");
                remove("/tmp/tmp.key");
                return -1;
            }
            p12 = d2i_PKCS12_fp(f, NULL);
            fclose(f);
            remove("/tmp/tmp.key");
        }

        if(p12 == NULL)
        {
            err_exit("read p12 certifcate error\n");
            return -1;
        }

        if( mode )
        {
            filep = fopen(psd_path,"rb");
            if( NULL == filep )
                return -1;
            //ret = fread(password,1,64,filep);
            ret = fscanf(filep, "%s", password);
            if(ret <= 0)
            {
                err_exit("read p12 certifcate password error\n");
                fclose(filep);
                return -1;
            }
/*            else
            {
                if(password[ret-1]=='\n')
                {
                    password[ret-1]='\0';
                }
            }*/
            fclose(filep);
        }
        else strncpy( password, "xdja1619", 64 );

        ret = PKCS12_parse(p12,password,&ckey,&cert,NULL);
        if( !ret )
        {
            EVP_PKEY_free(ckey);
            X509_free(cert);
            PKCS12_free(p12);
            log_error( "parse pkcs12 failed" );
            return -1;
        }

        PKCS12_free(p12);
        first = 0;
    }

    if( SSL_CTX_use_certificate(ctx, cert ) != 1)
    {
        err_exit("SSL_CTX_use_certificate_file failed");
        return -1;
    }

    if (SSL_CTX_use_PrivateKey(ctx,ckey)!= 1)
    {
        err_exit("SSL_CTX_use_PrivateKey_file failed");
        return -1;
    }

    return 0;
}

/********************************************************* 
	函数名称：ssl_certificate
	功能描述：设置ssl所用本地证书和私钥
	参数列表：param1-- SSL_CTX结构
			  param2--本地证书
			  param3--与证书配对的私钥
	返回结果：成功返???SSL_OK
			  失败返回 SSL_ERROR
*******************************************************/
int ssl_certificate(SSL_CTX *ctx, char *cert,char *key)
{
  
    //if (SSL_CTX_use_certificate_chain_file(ctx, cert)== 0)
    if( SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM ) <= 0)
    {
        err_exit("SSL_CTX_use_certificate_file failed");
        return SSL_ERROR;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx,key, SSL_FILETYPE_PEM)== 0)
    {
       err_exit("SSL_CTX_use_PrivateKey_file failed");
        return SSL_ERROR;
    }

    return SSL_OK;
}

/********************************************************* 
	函数名称：ssl_client_certificate
	功能描述：设置客户端的认证方式，ca证书，以及CA证书链的级数
	参数列表：param1-- SSL_CTX结构
			  param2--对客户端的认证方式，
						CLIENT_AUTH_NONE：不认真客户端证???						CLIENT_AUTH_REQUEST：需要客户端发送证???						CLIENT_AUTH_REQUIRE：只需要客户端发送一???							证书，重新协商是不用再发送客户端证书???			  param3--CA证书???			  param4--证书链的级数
	返回结果：成功返???SSL_OK
			  失败返回 SSL_ERROR
*******************************************************/
int ssl_ca_certificate( SSL_CTX *ctx,int client_auth, char *cert, int depth)
{
    STACK_OF(X509_NAME)  *list;

  
	   switch(client_auth){
	  case CLIENT_AUTH_NONE:
		SSL_CTX_set_verify(ctx,SSL_VERIFY_NONE ,ssl_verify_callback);
        break;
      case CLIENT_AUTH_REQUEST:
        SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER ,ssl_verify_callback);
        break;
      case CLIENT_AUTH_REQUIRE:
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE,ssl_verify_callback);
        break;
      case CLIENT_AUTH_REHANDSHAKE:
       
        break;
    }

    SSL_CTX_set_verify_depth(ctx, depth);	

    if (strlen(cert) == 0) {
        return SSL_OK;
    }

    if (SSL_CTX_load_verify_locations(ctx, cert, NULL)== 0)
    {
      err_exit( "SSL_CTX_load_verify_locations failed");
        return SSL_ERROR;
    }

    list = SSL_load_client_CA_file(cert);

    if (list == NULL) {
      err_exit("SSL_load_client_CA_file failed");
        return SSL_ERROR;
    }

    /*
     * before 0.9.7h and 0.9.8 SSL_load_client_CA_file()
     * always leaved an error in the error queue
     */

    ERR_clear_error();

    SSL_CTX_set_client_CA_list(ctx, list);

    return SSL_OK;
}


/********************************************************* 
	函数名称：ssl_crl
	功能描述：关联crl
	参数列表：param1-- SSL_CTX结构
			  param2-- crl
				
	返回结果：成功返???SSL_OK
			  失败返回 SSL_ERROR
*******************************************************/
int ssl_crl(SSL_CTX *ctx, char *crl)
{
    X509_STORE   *store;
    X509_LOOKUP  *lookup;

    if (strlen(crl) == 0) {
        return SSL_OK;
    }

    store = SSL_CTX_get_cert_store(ctx);

    if (store == NULL) {
     err_exit("SSL_CTX_get_cert_store  failed");
        return SSL_ERROR;
    }

    lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file());

    if (lookup == NULL) {
     err_exit("X509_STORE_add_lookup() failed");
        return SSL_ERROR;
    }

    if (X509_LOOKUP_load_file(lookup,  crl, X509_FILETYPE_PEM) == 0)
    {
		err_exit("X509_LOOKUP_load_file failed");
        return SSL_ERROR;
    }

    X509_STORE_set_flags(store,X509_V_FLAG_CRL_CHECK|X509_V_FLAG_CRL_CHECK_ALL);

    return SSL_OK;
}


/*认证的回调函??*/

static int ssl_verify_callback(int ok, X509_STORE_CTX *x509_store)
{
#if (_DEBUG)
    char              *subject, *issuer;
    int                err, depth;
    X509              *cert;
    X509_NAME         *sname, *iname;

    ssl_conn = X509_STORE_CTX_get_ex_data(x509_store,
                                          SSL_get_ex_data_X509_STORE_CTX_idx());

    cert = X509_STORE_CTX_get_current_cert(x509_store);
    err = X509_STORE_CTX_get_error(x509_store);
    depth = X509_STORE_CTX_get_error_depth(x509_store);

    sname = X509_get_subject_name(cert);
    subject = sname ? X509_NAME_oneline(sname, NULL, 0) : "(none)";

    iname = X509_get_issuer_name(cert);
    issuer = iname ? X509_NAME_oneline(iname, NULL, 0) : "(none)";

    log_debug( "verify:%d, error:%d, depth:%d, \n subject:\"%s\",issuer: \"%s\"",
                   ok, err, depth, subject, issuer);

    if (sname) {
        OPENSSL_free(subject);
    }

    if (iname) {
        OPENSSL_free(issuer);
    }
#endif

    return 1;
}

/********************************************************* 
	函数名称：ssl_set_cipher
	功能描述：设置加密套???	参数列表：param1-- SSL_CTX结构
			  param2-- 加密套件
				
	返回结果：成功返???SSL_OK
			  失败返回 SSL_ERROR
*******************************************************/
int ssl_set_cipher(SSL_CTX *ctx,char *list){
	
	if(SSL_CTX_set_cipher_list(ctx,list)!=SSL_OK){
		err_exit("SSL_CTX_set_cipher_liset failed");
		return SSL_ERROR;
	}

	return SSL_OK;
}

/********************************************************* 
	函数名称：ssl_generate_rsa512_key
	功能描述：设置ssl结构???12位的RSA密钥
	参数列表：param1-- SSL_CTX结构
				
	返回结果：成功返???SSL_OK
			  失败返回 SSL_ERROR
*******************************************************/
int ssl_generate_rsa512_key(SSL_CTX *ctx)
{
    RSA  *key;

    if (SSL_CTX_need_tmp_RSA(ctx) == 0) {
        return SSL_OK;
    }

    key = RSA_generate_key(512, RSA_F4, NULL, NULL);

    if (key) {
        SSL_CTX_set_tmp_rsa(ctx, key);

        RSA_free(key);

        return SSL_OK;
    }

     log_warn("RSA_generate_key(512) failed");

    return SSL_ERROR;
}

/********************************************************* 
	函数名称：ssl_dhparam
	功能描述：设置ssl结构dh初始???	参数列表：param1-- SSL_CTX结构
			  param2-- dh初始值文???	返回结果：成功返???SSL_OK
			  失败返回 SSL_ERROR
*******************************************************/
int ssl_dhparam(SSL_CTX *ctx, char *file)
{
    DH   *dh;
    BIO  *bio;

    /*
     * -----BEGIN DH PARAMETERS-----
     * MIGHAoGBALu8LcrYRnSQfEP89YDpz9vZWKP1aLQtSwju1OsPs1BMbAMCducQgAxc
     * y7qokiYUxb7spWWl/fHSh6K8BJvmd4Bg6RqSp1fjBI9osHb302zI8pul34HcLKcl
     * 7OZicMyaUDXYzs7vnqAnSmOrHlj6/UmI0PZdFGdX2gcd8EXP4WubAgEC
     * -----END DH PARAMETERS-----
     */

    static unsigned char dh1024_p[] = {
        0xBB, 0xBC, 0x2D, 0xCA, 0xD8, 0x46, 0x74, 0x90, 0x7C, 0x43, 0xFC, 0xF5,
        0x80, 0xE9, 0xCF, 0xDB, 0xD9, 0x58, 0xA3, 0xF5, 0x68, 0xB4, 0x2D, 0x4B,
        0x08, 0xEE, 0xD4, 0xEB, 0x0F, 0xB3, 0x50, 0x4C, 0x6C, 0x03, 0x02, 0x76,
        0xE7, 0x10, 0x80, 0x0C, 0x5C, 0xCB, 0xBA, 0xA8, 0x92, 0x26, 0x14, 0xC5,
        0xBE, 0xEC, 0xA5, 0x65, 0xA5, 0xFD, 0xF1, 0xD2, 0x87, 0xA2, 0xBC, 0x04,
        0x9B, 0xE6, 0x77, 0x80, 0x60, 0xE9, 0x1A, 0x92, 0xA7, 0x57, 0xE3, 0x04,
        0x8F, 0x68, 0xB0, 0x76, 0xF7, 0xD3, 0x6C, 0xC8, 0xF2, 0x9B, 0xA5, 0xDF,
        0x81, 0xDC, 0x2C, 0xA7, 0x25, 0xEC, 0xE6, 0x62, 0x70, 0xCC, 0x9A, 0x50,
        0x35, 0xD8, 0xCE, 0xCE, 0xEF, 0x9E, 0xA0, 0x27, 0x4A, 0x63, 0xAB, 0x1E,
        0x58, 0xFA, 0xFD, 0x49, 0x88, 0xD0, 0xF6, 0x5D, 0x14, 0x67, 0x57, 0xDA,
        0x07, 0x1D, 0xF0, 0x45, 0xCF, 0xE1, 0x6B, 0x9B
    };

    static unsigned char dh1024_g[] = { 0x02 };


    if (strlen(file) == 0) {

        dh = DH_new();
        if (dh == NULL) {
            err_exit("DH_new() failed");
            return SSL_ERROR;
        }

        dh->p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), NULL);
        dh->g = BN_bin2bn(dh1024_g, sizeof(dh1024_g), NULL);

        if (dh->p == NULL || dh->g == NULL) {
           err_exit("BN_bin2bn() failed");
            DH_free(dh);
            return SSL_ERROR;
        }

        SSL_CTX_set_tmp_dh(ctx, dh);

        DH_free(dh);

        return SSL_OK;
    }

   

    bio = BIO_new_file(file, "r");
    if (bio == NULL) { 
                err_exit("BIO_new_file failed");
        return SSL_ERROR;
    }

    dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
    if (dh == NULL) {
     err_exit("PEM_read_bio_DHparams(\"%s\") failed");
        BIO_free(bio);
        return SSL_ERROR;
    }

    SSL_CTX_set_tmp_dh(ctx, dh);

    DH_free(dh);
    BIO_free(bio);

    return SSL_OK;
}

/********************************************************* 
	函数名称：ssl_create_connection
	功能描述：创建一个SSL连接
	参数列表：param1-- SSL_CTX结构
			  param2-- 文件描述???			  param3-- 运行模式，是客户端还是服务端
	返回结果：成功返???SSL *
			  失败返回 NULL
*******************************************************/
SSL *ssl_create_connection(SSL_CTX *ctx,int fd, int flags)
{
   SSL *ssl = NULL;

	ssl = SSL_new(ctx);
	if(ssl == NULL){
		err_exit("SSL_new failed");
		return NULL;
	}
    if (SSL_set_fd(ssl, fd) == 0) {
        err_exit("SSL_set_fd  failed");
        return NULL;
    }

    if (flags & SSL_CLIENT) {
        SSL_set_connect_state(ssl);

    } else {
        SSL_set_accept_state(ssl);
    }


    return ssl;
}

/********************************************************* 
	函数名称：ssl_create_connection
	功能描述：将ssl连接和会话关联起???	参数列表：param1-- SSL_CTX结构
			  param2-- SSL_SESSION结构
	返回结果：成功返???SSL_OK
			  失败返回 SSL_ERROR
*******************************************************/
int ssl_set_session(SSL *ssl, SSL_SESSION *session)
{
    if (session) {
        if (SSL_set_session(ssl, session) == 0) {
            err_exit( "SSL_set_session failed");
            return SSL_ERROR;
        }
    }

    return SSL_OK;
}

/********************************************************* 
	函数名称：ssl_handshake
	功能描述：ssl握手协商
	参数列表：param1-- 连接结构
	说明???本函数在使用的时候因为使用异步io的操作方式???			在注册异步事件和响应异步事件的操作不尽相同，
			在下面的注释位置加入相应的操作???			  
	返回结果：成功返???SSL_OK
			  失败返回 SSL_ERROR
*******************************************************/
int ssl_handshake(connection *c)
{
    int        n, sslerr;

    n = SSL_do_handshake(c->ssl_con->ssl);
	if ( n==1 ){
		return SSL_OK;


#if (_DEBUG)
        {
        char         buf[129], *s, *d;

        SSL_CIPHER  *cipher;

        cipher = SSL_get_current_cipher(c->ssl_con->ssl);

        if (cipher) {
            SSL_CIPHER_description(cipher, &buf[1], 128);

            for (s = &buf[1], d = buf; *s; s++) {
                if (*s == ' ' && *d == ' ') {
                    continue;
                }

                if (*s == LF || *s == CR) {
                    continue;
                }

                *++d = *s;
            }

            if (*d != ' ') {
                d++;
            }

            *d = '\0';

           log_debug("SSL: %s, cipher: \"%s\"",SSL_get_version(c->ssl_con->ssl), &buf[1]);

            if (SSL_session_reused(c->ssl_con->ssl)) {
               log_debug( "SSL reused session");
            }

        } else {
          log_debug("SSL no shared ciphers");
        }
        }
#endif

        c->ssl_con->handshaked = 1;

        c->recv = ssl_recv;
        c->send = ssl_write;
        
        /* initial handshake done, disable renegotiation  */
        if (c->ssl_con->ssl->s3) {
           c->ssl_con->ssl->s3->flags |= SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS;
        }

        return SSL_OK;
    }

    sslerr = SSL_get_error(c->ssl_con->ssl, n);

    log_debug( "SSL_get_error: %d", sslerr);

    if (sslerr == SSL_ERROR_WANT_READ) {
       

        return SSL_AGAIN_WANT_READ;
    }

    if (sslerr == SSL_ERROR_WANT_WRITE) {
      

        return SSL_AGAIN_WANT_WRITE;
    }

	if(sslerr == SSL_ERROR_SYSCALL){

    c->ssl_con->no_wait_shutdown = 1;
    c->ssl_con->no_send_shutdown = 1;

	}
    if (sslerr == SSL_ERROR_ZERO_RETURN || ERR_peek_error() == 0) {
        err_exit("peer closed connection in SSL handshake");

        return SSL_ERROR;
    }

   berr_exit( "SSL_do_handshake() failed");

    return SSL_ERROR;
}



/********************************************************* 
	函数名称：ssl_recv
	功能描述：ssl异步接收数据，读取指定长度的数据到缓冲区
	参数列表：param1-- ssl连接结构
			  param2--存数据缓???			  param3--读取数据的长???
	
	返回结果：读取到数据长度
*******************************************************/

int ssl_recv(con_ssl *ssl_con, unsigned char *buf, int size)
{
    if( NULL==ssl_con || NULL==buf )
        return SSL_ERROR;

    SSL *ssl = ssl_con->ssl;
    int n, readn = 0;
    int sslerr, err;

    for(;;)
    {
        n = SSL_read( ssl, buf, size );

        if( n > 0 )
        {
            readn += n;
            size -= n;
            if( size <= 0 )
                return readn;
            buf += n;
            continue;
        }

        sslerr = SSL_get_error( ssl, n );
        err = SSL_ERROR_SYSCALL==sslerr ? errno : 0;

        if( readn > 0 )
            return readn;

        log_debug( "SSL_read err %d, errno %d", sslerr, err );
        switch( sslerr )
        {
            case SSL_ERROR_WANT_READ:
                return SSL_AGAIN_WANT_READ;
            case SSL_ERROR_WANT_WRITE:
                return SSL_AGAIN_WANT_WRITE;
            case SSL_ERROR_ZERO_RETURN:
                return SSL_CLOSE;
            default:
                return SSL_ERROR;
        }
    }
}



/********************************************************* 
	函数名称：ssl_write
	功能描述：ssl异步发送数据，发送缓冲区内指定长度的数据
	参数列表：param1-- ssl连接结构
			  param2--存数据缓???			  param3--读取数据的长???
	
	返回结果：发送过的数据长???
	*******************************************************/


int ssl_write(con_ssl *ssl_con, unsigned char *data, int size)
{
    int        n, sslerr;

	log_debug("SSL to write: %d", size);

    n = SSL_write(ssl_con->ssl, data, size);

	log_debug( "SSL_write: %d", n);

    if (n > 0) {
        return n;
    }

    sslerr = SSL_get_error(ssl_con->ssl, n);
    log_debug( "ssl_write err %d", sslerr );
    switch( sslerr )
    {
        case SSL_ERROR_ZERO_RETURN:
            return SSL_CLOSE;
        case SSL_ERROR_WANT_READ:
            return SSL_AGAIN_WANT_READ;
        case SSL_ERROR_WANT_WRITE:
            return SSL_AGAIN_WANT_WRITE;
        default:
            return SSL_ERROR;
    }
}



/********************************************************* 
	函数名称：ssl_shutdown
	功能描述：关闭连???	参数列表：param1-- 连接结构
	
	返回结果：成功返???SSL_OK
			  失败返回 SSL_ERROR
*******************************************************/

int ssl_shutdown(connection *c)
{
    int        n, sslerr, mode;


    if (c->timeout) {
        mode = SSL_RECEIVED_SHUTDOWN|SSL_SENT_SHUTDOWN;

    } else {
        mode = SSL_get_shutdown(c->ssl_con->ssl);

        if (c->ssl_con->no_wait_shutdown) {
            mode |= SSL_RECEIVED_SHUTDOWN;
        }

        if (c->ssl_con->no_send_shutdown) {
            mode |= SSL_SENT_SHUTDOWN;
        }
    }

    SSL_set_shutdown(c->ssl_con->ssl, mode);

 
    n = SSL_shutdown(c->ssl_con->ssl);
	log_debug( "SSL_shutdown: %d", n);

    sslerr = 0;

    /* SSL_shutdown() never returns -1, on error it returns 0 */

    if (n != 1 && ERR_peek_error()) {
        sslerr = SSL_get_error(c->ssl_con->ssl, n);

        log_debug("SSL_get_error: %d", sslerr);
    }

    if (n == 1 || sslerr == 0 || sslerr == SSL_ERROR_ZERO_RETURN) {
        SSL_free(c->ssl_con->ssl);
        c->ssl_con->ssl = NULL;

        return SSL_OK;
    }

 
 

        if (sslerr == SSL_ERROR_WANT_READ) {
			return SSL_AGAIN_WANT_READ;
        }
		 if (sslerr == SSL_ERROR_WANT_WRITE) {
			return SSL_AGAIN_WANT_WRITE;
        }
        


    SSL_free(c->ssl_con->ssl);
    c->ssl_con->ssl = NULL;

    return SSL_ERROR;
}






void ssl_cleanup_ctx(SSL_CTX *ctx)
{

    SSL_CTX_free(ctx);
	ctx = NULL;
}


int ssl_get_protocol(connection *c, char *s)
{
    s = (char *) SSL_get_version(c->ssl_con->ssl);
    return SSL_OK;
}


int ssl_get_cipher_name(connection *c, char *s)
{
    s = (char *) SSL_get_cipher_name(c->ssl_con->ssl);
    return SSL_OK;
}




int ssl_get_raw_certificate(connection *c, char **s,int *length)
{
    int   len;
    BIO     *bio;
    X509    *cert;



    cert = SSL_get_peer_certificate(c->ssl_con->ssl);
    if (cert == NULL) {
        return SSL_OK;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        log_warn("BIO_new() failed");
        X509_free(cert);
        return SSL_ERROR;
    }

    if (PEM_write_bio_X509(bio, cert) == 0) {
        log_debug("PEM_write_bio_X509() failed");
        goto failed;
    }

    len = BIO_pending(bio);
    *length = len;

   * s = (char *)malloc(len);
    if (*s == NULL) {
        goto failed;
    }

    BIO_read(bio, *s, len);

    BIO_free(bio);
    X509_free(cert);

    return SSL_OK;

failed:

    BIO_free(bio);
    X509_free(cert);

    return SSL_ERROR;
}


int ssl_get_certificate(connection *c, char *s, int *length)
{
    char      *p;
    int       len;
    int    i;
    char   *cert;

    if (ssl_get_raw_certificate(c, &cert,length) != SSL_OK) {
        return SSL_ERROR;
    }
  

    len = *length - 1;

    for (i = 0; i < *length - 1; i++) {
        if (*(cert+i) == LF) {
            len++;
        }
    }

   *length = len;
    s = (char *)malloc(len);
    if (s == NULL) {
        return SSL_ERROR;
    }

    p = s;

    for (i = 0; i < *length; i++) {
        *p++ = *(cert+i);
        if (*(cert+i) == LF) {
            *p++ = '\t';
        }
    }
    free(cert);
    return SSL_OK;
}


int ssl_get_subject_dn(connection *c, char *s,int *length)
{
    char       *p;
    int      len;
    X509       *cert;
    X509_NAME  *name;

   
    cert = SSL_get_peer_certificate(c->ssl_con->ssl);
    if (cert == NULL) {
        return SSL_OK;
    }

    name = X509_get_subject_name(cert);
    if (name == NULL) {
        X509_free(cert);
        return SSL_ERROR;
    }

    p = X509_NAME_oneline(name, NULL, 0);

    for (len = 0; p[len]; len++) { /* void */ }

    *length = len;
    s = (char *)malloc( len);
    if (s == NULL) {
        OPENSSL_free(p);
        X509_free(cert);
        return SSL_ERROR;
    }

    memcpy(s, p, len);

    OPENSSL_free(p);
    X509_free(cert);

    return SSL_OK;
}


int ssl_get_issuer_dn(connection *c,  char *s ,int *length)
{
    char       *p;
    int      len;
    X509       *cert;
    X509_NAME  *name;

 

    cert = SSL_get_peer_certificate(c->ssl_con->ssl);
    if (cert == NULL) {
        return SSL_OK;
    }

    name = X509_get_issuer_name(cert);
    if (name == NULL) {
        X509_free(cert);
        return SSL_ERROR;
    }

    p = X509_NAME_oneline(name, NULL, 0);

    for (len = 0; p[len]; len++) { /* void */ }

    *length = len;
    s = (char *)malloc(len);
    if (s == NULL) {
        OPENSSL_free(p);
        X509_free(cert);
        return SSL_ERROR;
    }

    memcpy(s, p, len);

    OPENSSL_free(p);
    X509_free(cert);

    return SSL_OK;
}


int ssl_get_serial_number(connection *c,  char *s ,int *length)
{
    int   len;
    X509    *cert;
    BIO     *bio;


    cert = SSL_get_peer_certificate(c->ssl_con->ssl);
    if (cert == NULL) {
        return SSL_OK;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        X509_free(cert);
        return SSL_ERROR;
    }

    i2a_ASN1_INTEGER(bio, X509_get_serialNumber(cert));
    len = BIO_pending(bio);

    *length = len;
    s = (char *) malloc (len);
    if (s == NULL) {
        BIO_free(bio);
        X509_free(cert);
        return SSL_ERROR;
    }

    BIO_read(bio, s, len);
    BIO_free(bio);
    X509_free(cert);

    return SSL_OK;
}


int ssl_get_client_verify(connection *c,char *s)
{
    X509  *cert;

    if (SSL_get_verify_result(c->ssl_con->ssl) != X509_V_OK) {
        memcpy(s, "FAILED",strlen("FAILED"));
        return SSL_OK;
    }

    cert = SSL_get_peer_certificate(c->ssl_con->ssl);

    if (cert) {
        memcpy(s, "SUCCESS",strlen("SUCCESS"));

    } else {
        memcpy(s, "NONE",strlen("NONE"));
    }

    X509_free(cert);

    return SSL_OK;
}


