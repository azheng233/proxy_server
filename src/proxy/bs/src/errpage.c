#include "errpage.h"
#include "options.h"
#include "timehandle.h"
#include <string.h>
#include <stdio.h>
/*
Server: nginx/0.8.47
Date: Mon, 09 Aug 2010 03:16:03 GMT
Content-Type: text/html
Content-Length: 173
Connection: close
<html>
<head><title>400 Bad Request</title></head>
<body bgcolor="white">
<center><h1>400 Bad Request</h1></center>
<hr><center>xdproxy/1.0</center>
</body>
</html>
*/

#define SERVER_INFO "BSAMS/" VERSION
#define CRLF "\r\n"

#define HTTP_MSG_OK \
    "HTTP/1.1 200 OK" CRLF      \
    "Server: " SERVER_INFO CRLF \
    "Date: %s" CRLF     \
    "Connection: keep-alive" CRLF       \
    CRLF

#define HTTP_MSG_ESTABLISHED \
    "HTTP/1.1 200 Connection Established" CRLF      \
    "Server: " SERVER_INFO CRLF \
    "Date: %s" CRLF     \
    "Connection: keep-alive" CRLF       \
    CRLF

#define HTTP_MSG_ERR_HEAD \
    "HTTP/1.1 %u %s" CRLF       \
    "Server: " SERVER_INFO CRLF \
    "Date: %s" CRLF     \
    "Content-Type: text/html;charset=UTF-8" CRLF        \
    "Content-Length: %d" CRLF   \
    "Connection: close" CRLF    \
    CRLF

//    "<center><a href=\"%s\"><button>获取可访问地址列表</button></a></center>"  
#define HTTP_MSG_ERR_CONTENT \
    "<html>"    \
    "<head>"    \
    "<title>%u %s</title>"      \
    "<head>"    \
    "<body bgcolor=\"white\">"  \
    "<center><h1>%u %s</h1></center>"   \
    "<p>%s</p>" \
    "<hr/>"     \
    "<center><input type=\"button\" value=\"获取当前可访问地址列表\" onclick=\"javascript:window.location.href='%s'\" /></center>"  \
    "<hr/>"     \
    "<center>" SERVER_INFO "</center>"  \
    "</body>"   \
    "</html>"   \
    CRLF

#define HTTP_MSG_HEAD \
    "HTTP/1.1 200 OK" CRLF       \
    "Server: " SERVER_INFO CRLF \
    "Date: %s" CRLF     \
    "Content-Type: text/html;charset=UTF-8" CRLF        \
    "Content-Length: %d" CRLF   \
    "Connection: close" CRLF    \
    CRLF

#define HTTP_MSG_CONTENT \
    "<html>"    \
    "<head>"    \
    "<title>%s</title>"      \
    "<head>"    \
    "<body bgcolor=\"white\">"  \
    "<center><h1>%s</h1></center>"   \
    "<p><center><table border=2>%s</table></center></p>" \
    "<hr/>"     \
    "<center>" SERVER_INFO "</center>"  \
    "</body>"   \
    "</html>"   \
    CRLF

static const struct http_code_desc {
    unsigned code;
    const char *desc;
} http_code_desc[] = {
    [HTTP_200_OK] = { 200, "OK" },
    [HTTP_200_ESTABLISHED] = { 200, "Connection Established" },
    [HTTP_400_BAD_REQ] = { 400, "Bad Request" },
    [HTTP_403_FORBIDDEN] = { 403, "Forbidden" },
    [HTTP_407_PROXY_AUTH_REQ] = { 407, "Proxy Authentication Required" },
    [HTTP_504_GW_TIMEOUT] = { 504, "Gateway Timeout" },
};

#define gen_ok_head( buf, max, tim ) \
    (buf)->used = snprintf( (char*)((buf)->data), max, HTTP_MSG_OK, tim )

#define gen_established_head( buf, max, tim ) \
    (buf)->used = snprintf( (char*)((buf)->data), max, HTTP_MSG_ESTABLISHED, tim )

#define gen_err_head( buf, max, cod, tim, len ) \
    (buf)->used = snprintf( (char*)((buf)->data), max, HTTP_MSG_ERR_HEAD, \
            http_code_desc[cod].code, http_code_desc[cod].desc, tim, len )

#define gen_err_body( buf, max, cod, tim, msg, allowlink ) \
    (buf)->used = snprintf( (char*)((buf)->data), max, HTTP_MSG_ERR_CONTENT, \
            http_code_desc[cod].code, http_code_desc[cod].desc, \
            http_code_desc[cod].code, http_code_desc[cod].desc, \
            msg, allowlink )

#define gen_msg_head( buf, max, tim, len ) \
    (buf)->used = snprintf( (char*)((buf)->data), max, HTTP_MSG_HEAD, tim, len )

#define gen_msg_body( buf, max, tim, title, msg ) \
    (buf)->used = snprintf( (char*)((buf)->data), max, HTTP_MSG_CONTENT, title, title, msg )

#define MSG_MAX_SIZE 2048
#define MSG_MAX_SIZE_ex 20480

int putmsg_connect_ok( chunk_t *ck )
{
    buffer_t *buf;
    const char *tm;

    tm = time_gmt();

    buf = buffer_new( MSG_MAX_SIZE );
    if( NULL==buf )
        return -1;

    gen_ok_head( buf, MSG_MAX_SIZE, tm );
    buffer_put( ck, buf );

    return 0;
}

int putmsg_connect_established( chunk_t *ck )
{
    buffer_t *buf;
    const char *tm;

    tm = time_gmt();

    buf = buffer_new( MSG_MAX_SIZE );
    if( NULL==buf )
        return -1;

    gen_established_head( buf, MSG_MAX_SIZE, tm );
    buffer_put( ck, buf );

    return 0;
}

int putmsg_err( chunk_t *ck, enum http_code code, char *msg, char *allowlink )
{
    buffer_t *head, *body;
    const char *tm;

    tm = time_gmt();

    head = buffer_new( MSG_MAX_SIZE );
    if( NULL==head )
        return -1;
    body = buffer_new( MSG_MAX_SIZE );
    if( NULL==body )
    {
        buffer_del( head );
        return -1;
    }

    gen_err_body( body, MSG_MAX_SIZE, code, tm, msg, allowlink );
    gen_err_head( head, MSG_MAX_SIZE, code, tm, (int)(body->used) );

    buffer_put( ck, head );
    buffer_put( ck, body );

    return 0;
}

int putmsg( chunk_t *ck, char *title, char *msg )
{
    buffer_t *head, *body;
    const char *tm;

    tm = time_gmt();

    head = buffer_new( MSG_MAX_SIZE );
    if( NULL==head )
        return -1;
    body = buffer_new( MSG_MAX_SIZE_ex );
    if( NULL==body )
    {
        buffer_del( head );
        return -1;
    }

    gen_msg_body( body, MSG_MAX_SIZE_ex, tm, title, msg );
    gen_msg_head( head, MSG_MAX_SIZE, tm, (int)(body->used) );

    buffer_put( ck, head );
    buffer_put( ck, body );

    return 0;
}

