#ifndef _URL_PARSE_H_
#define _URL_PARSE_H_
#include "connection.h"
#include "buffer.h"
#include <stdint.h>

//所有的HTTP方法
typedef enum {
	HTTP_METHOD_UNSET = -1,
	HTTP_METHOD_GET,
	HTTP_METHOD_POST,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_OPTIONS,
	HTTP_METHOD_PROPFIND,  /* WebDAV */
	HTTP_METHOD_MKCOL,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,
	HTTP_METHOD_COPY,
	HTTP_METHOD_MOVE,
	HTTP_METHOD_PROPPATCH,
	HTTP_METHOD_REPORT, /* DeltaV */
	HTTP_METHOD_CHECKOUT,
	HTTP_METHOD_CHECKIN,
	HTTP_METHOD_VERSION_CONTROL,
	HTTP_METHOD_UNCHECKOUT,
	HTTP_METHOD_MKACTIVITY,
	HTTP_METHOD_MERGE,
	HTTP_METHOD_LOCK,
	HTTP_METHOD_UNLOCK,
	HTTP_METHOD_LABEL,
	HTTP_METHOD_CONNECT
} http_method_t;

//一些值的最大长度限制
#define HTTP_METHOD_MAX 32
#define HTTP_URL_MAX 1024
#define HTTP_BODYLEN_MAX 64
#define HTTP_HOST_MAX 64
#define HTTP_PORT_MAX 32
#define HTTP_SCHEMA_MAX 32
#define HTTP_HEADNAME_MAX 64
#define HTTP_HEADVAL_MAX 64
#define HTTP_USERNAME_MAX 64
#define HTTP_PASSWORD_MAX 64


//是什么协议
#define HTTP_SCHEMA_HTTP 1
#define HTTP_SCHEMA_FTP 2
#define HTTP_SCHEMA_HTTPS 3
#define HTTP_SCHEMA_UNSET 9

//解析返回值
#define HTTP_PARSE_NULL -1
#define HTTP_PARSE_AGAIN 1
#define HTTP_PARSE_OK 2
#define HTTP_PARSE_HEADOVER 3
#define HTTP_PARSE_ERR 4
#define HTTP_PARSE_ERR_METHOD 5
#define HTTP_PARSE_ERR_URL 6
#define HTTP_PARSE_ERR_HOST 7
#define HTTP_PARSE_ERR_PORT 8
#define HTTP_PARSE_ERR_SCHEMA 9
#define HTTP_PARSE_ERR_HEADNAME 10
#define HTTP_PARSE_ERR_HEADVAL 11
#define HTTP_PARSE_ERR_HEAD 12
#define HTTP_PARSE_ERR_USERNAME 13
#define HTTP_PARSE_ERR_PASSWORD 14


//HTTP请求
typedef struct http_req_s
{
	int stat;
	buffer_t *parse_buf;
	int parse_pos;
	int parse_stat;
	//int parse_startpos;
	//int parse_startbuf;	
	//buffer_t *parse_startbuf;

	buffer_t *url;
	buffer_t *method;
	buffer_t *host;
	buffer_t *port;
	buffer_t *schema;
	buffer_t *curheadname;
	buffer_t *curheadval;
        buffer_t *username;
        buffer_t *password;

	buffer_t *host_prev;
	buffer_t *port_prev;

        buffer_t *host_ssl;
        buffer_t *port_ssl;

        buffer_t *head_host;
        buffer_t *url_prev;

	int method_type;
	int schema_type;//ftp http https
	int port_num;
	uint64_t content_sumlen;
    uint64_t content_curlen;
	int content_length_exist;//0 no;1 yes;default 0
	
}http_req_t;

int http_parse_command( connection_t *con );
int http_parse_head( connection_t *con );

int http_head_host_split( connection_t *c );
char *get_host_addr( connection_t *c );

#endif
