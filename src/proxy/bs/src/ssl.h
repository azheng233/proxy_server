/******************************************/
	/*描述：本文件为https策略过滤服务器的
	ssl模块实现ssl的协商和会话保持。提供异
	步读写方法	作者：廖正赟*/
    
/******************************************/
#ifndef		_SSL_H_
#define		_SSL_H_
#include    <openssl/ssl.h>
#include    <openssl/lhash.h>
#include    <openssl/bio.h>
#include    <openssl/err.h>
#include    <openssl/engine.h>
#include    <openssl/dh.h>
#include    <openssl/conf.h>
#include    <openssl/pkcs12.h>
#include    <openssl/pem.h>
#include    <openssl/evp.h>
#include    <openssl/x509.h>




#define  SSL_OK					   0
#define  SSL_ERROR                -1
#define  SSL_AGAIN_WANT_READ      -2
#define  SSL_AGAIN_WANT_WRITE     -3
#define  SSL_BUSY                 -4
#define  SSL_DONE                 -5
#define  SSL_DECLINED             -6
#define  SSL_ABORT                -7
#define  SSL_CLOSE                -8

#define	 SSL_SERVER 0
#define  SSL_CLIENT 1

#define  SSLv2 2
#define  SSLv3 4
#define  TLSv1 8

#define CLIENT_AUTH_NONE  0
#define CLIENT_AUTH_REQUEST 1
#define CLIENT_AUTH_REQUIRE 2
#define CLIENT_AUTH_REHANDSHAKE 3


#define LF (unsigned char)10


typedef struct{
	void *data;
	int timeout;
}events;

typedef void (* fd_handler)(events *ev);

 typedef struct {
	int ready;
	fd_handler handler;
	events *ev;
	int error;
	int timeout;
	
}fdevent;

 typedef struct {
	SSL *ssl;
	SSL_CTX *ctx;
	//fdevent *read;
	//fdevent *write;
	fd_handler saved_read_handler;
	fd_handler saved_write_handler;
	int handshaked;
	int no_wait_shutdown;
	int no_send_shutdown;
	int renegotiation;
	int last;
	void *c;
 }con_ssl;

 typedef int(* iodata)(con_ssl *con,unsigned char *buf,int size);

enum waiton {
    none_wait = 0,
    read_waiton_write,
    write_waiton_read
};

typedef struct {
	
	iodata recv;
	iodata send;
	fd_handler handler;  /*初始化的时候handler=ssl_handshake_handler*/

	con_ssl *ssl_con;
        enum waiton wait;
	
	int timeout;
	
}connection;

int ssl_init();
void ssl_unload();
SSL_CTX *ssl_create();
int ssl_certificate(SSL_CTX *ctx, char *cert,char *key);
int ssl_ca_certificate( SSL_CTX *ctx,int client_auth, char *cert, int depth);
int ssl_crl(SSL_CTX *ssl, char *crl);
int ssl_generate_rsa512_key(SSL_CTX *ctx);
int ssl_set_cipher(SSL_CTX *ctx,char *list);
int ssl_dhparam(SSL_CTX *ssl, char *file);
SSL *ssl_create_connection(SSL_CTX *ctx,int fd, int flags);
int ssl_set_session(SSL *ssl, SSL_SESSION *session);
int ssl_handshake(connection *c);
int ssl_recv(con_ssl *ssl_con, unsigned char *buf, int size);
int ssl_write(con_ssl *ssl_con, unsigned char *data, int size);
int ssl_shutdown(connection *c);
void ssl_cleanup_ctx(SSL_CTX *ctx);
int ssl_get_protocol(connection *c, char *s);
int ssl_get_cipher_name(connection *c, char *s);
int ssl_get_session_id(connection *c, char *s);
int ssl_get_raw_certificate(connection *c, char **s,int *length);
int ssl_get_certificate(connection *c, char *s, int *length);
int ssl_get_subject_dn(connection *c, char *s,int *length);
int ssl_get_issuer_dn(connection *c,  char *s ,int *length);
int ssl_get_serial_number(connection *c,  char *s ,int *length);
int ssl_get_client_verify(connection *c,char *s);
int parse_cert_p12( SSL_CTX *ctx, int mode, char *p12_cert_path, char *psd_path );


#endif

