#include <stdio.h>
#include <stdlib.h>

#include "readconf.h"
#include "http_parse.h"
#include "connection.h"
#include "chunk.h"
#include "buffer.h"
#include "log.h"
#include <string.h>
#include <stdint.h>

#define BACKEND 1

//进行内存比较
int http_memcmp( char *p, char *q, int len )
{
	int i;
	for( i=0; i<len; i++ )
	{
		if( p[i]!=q[i] ) return 1;
	}
	if( i==len ) return 0;
	else return 1;
}

//根据将字符型method转为整形
int http_get_method( char *p, int len )
{
	switch (len)
	{
	case 3:
		if( http_memcmp( p, "GET", len )==0 ) return HTTP_METHOD_GET;
		if( http_memcmp( p, "POST", len )==0 ) return HTTP_METHOD_POST;
		break;
	case 4:
		if( http_memcmp( p, "POST", len )==0 ) return HTTP_METHOD_POST;
		if( http_memcmp( p, "HEAD", len )==0 ) return HTTP_METHOD_HEAD;
		if( http_memcmp( p, "COPY", len )==0 ) return HTTP_METHOD_COPY;
		if( http_memcmp( p, "LOCK", len )==0 ) return HTTP_METHOD_LOCK;
		if( http_memcmp( p, "MOVE", len )==0 ) return HTTP_METHOD_MOVE;
		break;
	case 5:
		if( http_memcmp( p, "MKCOL", len )==0 ) return HTTP_METHOD_MKCOL;
		if( http_memcmp( p, "LABEL", len )==0 ) return HTTP_METHOD_LABEL;
		if( http_memcmp( p, "MERGE", len )==0 ) return HTTP_METHOD_MERGE;

		break;
	case 6:
		if( http_memcmp( p, "DELETE", len )==0 ) return HTTP_METHOD_DELETE;
		if( http_memcmp( p, "REPORT", len )==0 ) return HTTP_METHOD_REPORT;
		if( http_memcmp( p, "UNLOCK", len )==0 ) return HTTP_METHOD_UNLOCK;

		break;
	case 7:
		if( http_memcmp( p, "CONNECT", len )==0 ) return HTTP_METHOD_CONNECT;
		if( http_memcmp( p, "OPTIONS", len )==0 ) return HTTP_METHOD_OPTIONS;
		if( http_memcmp( p, "CHECKIN", len )==0 ) return HTTP_METHOD_CHECKIN;

		break;
	case 8:
		if( http_memcmp( p, "PROPFIND", len )==0 ) return HTTP_METHOD_PROPFIND;
		if( http_memcmp( p, "CHECKOUT", len )==0 ) return HTTP_METHOD_CHECKOUT;
		break;
	case 9:
		if( http_memcmp( p, "PROPPATCH", len )==0 ) return HTTP_METHOD_PROPPATCH;
		break;
	case 10:
		if( http_memcmp( p, "MKACTIVITY", len )==0 ) return HTTP_METHOD_MKACTIVITY;
		if( http_memcmp( p, "UNCHECKOUT", len )==0 ) return HTTP_METHOD_UNCHECKOUT;
		break;
	case 15:
		if( http_memcmp( p, "VERSION-CONTROL", len )==0 ) return HTTP_METHOD_VERSION_CONTROL;
		break;
	}
	return HTTP_METHOD_UNSET;
}

int http_get_schema( char *p, int len )
{
	if( len==0 ) return HTTP_SCHEMA_HTTP;
	if( len==3 )
	{
		if( http_memcmp(p,"ftp",len)==0 ) return HTTP_SCHEMA_FTP;
	}

	if( len==4 )
	{
		if( http_memcmp(p,"http",len)==0 ) return HTTP_SCHEMA_HTTP;
	}

	if( len==5 )
	{
		if( http_memcmp(p,"https",len)==0 ) return HTTP_SCHEMA_HTTPS;
	}
	return HTTP_SCHEMA_UNSET;
}

int http_get_port( char *p, int len )
{
	int i;
	if( len==0 ) return 80;
	if( len<0 || len>5 ) return -1;
	for( i=0; i<len; i++ )
	{
		if( p[i]<'0' || p[i]>'9') return -1;
	}
	p[len] = '\0';
	i = atoi( p );
	if( i<=0 || i>65535 ) return -1;
	return i;
}



#define HTTP_PUSH_XX(x,xbuf,xmax,xerr) if( r->xbuf->used==xmax-1 ) return xerr; \
									r->xbuf->data[r->xbuf->used]=x;r->xbuf->used++;

#define HTTP_PUSH_METHOD(x) HTTP_PUSH_XX(x,method,HTTP_METHOD_MAX,HTTP_PARSE_ERR_METHOD)
#define HTTP_PUSH_URL(x) HTTP_PUSH_XX(x,url,HTTP_URL_MAX,HTTP_PARSE_ERR_URL)
#define HTTP_PUSH_HOST(x) HTTP_PUSH_XX(x,host,HTTP_HOST_MAX,HTTP_PARSE_ERR_HOST)
#define HTTP_PUSH_PORT(x) HTTP_PUSH_XX(x,port,HTTP_PORT_MAX,HTTP_PARSE_ERR_PORT)
#define HTTP_PUSH_SCHEMA(x) HTTP_PUSH_XX(x,schema,HTTP_SCHEMA_MAX,HTTP_PARSE_ERR_SCHEMA)
#define HTTP_PUSH_HEADNAME(x) HTTP_PUSH_XX(x,curheadname,HTTP_HEADNAME_MAX,HTTP_PARSE_ERR_HEADNAME)
#define HTTP_PUSH_USERNAME(x) HTTP_PUSH_XX(x,username,HTTP_USERNAME_MAX,HTTP_PARSE_ERR_USERNAME)
#define HTTP_PUSH_PASSWORD(x) HTTP_PUSH_XX(x,password,HTTP_PASSWORD_MAX,HTTP_PARSE_ERR_PASSWORD)
/*
#define HTTP_PUSH_HEADVAL(x) HTTP_PUSH_XX(x,curheadval,HTTP_HEADVAL_MAX,HTTP_PARSE_ERR_HEADVAL)
*/

#define HTTP_PUSH_HEADVAL(x) if( r->curheadval->used<HTTP_HEADVAL_MAX-1 ) { \
					     r->curheadval->data[r->curheadval->used] = p; \
						 r->curheadval->used++; }


#define del_first_char_and_update_pos( chunk, buf, pos ) \
    do{ \
        (buf)->data++; \
        (buf)->used--; \
        (pos)--; \
        (chunk)->total_size--; \
    }while(0)

#define del_last_char_and_update_pos( chunk, buf, pos ) \
    do{ \
        (buf)->used--; \
        (pos)--; \
        (chunk)->total_size--; \
    }while(0)


#define schema_is_ftp( buf ) \
    ( 3==(buf)->used && 'f'==(buf)->data[0] && 't'==(buf)->data[1] && 'p'==(buf)->data[2] )

enum at_stat { AT_YES, AT_NO, AT_AGAIN, AT_ERROR };
// 向前检查缓冲区中有无@符号
static enum at_stat check_at_forward( chunk_t *ck, buffer_t *buf, unsigned int pos )
{
    if( NULL==ck )
        return AT_ERROR;

    while( NULL != buf )
    {
        if( pos > buf->used )
        {
            buf = buffer_getnext( ck, buf );
            pos = 0;
            continue;
        }

        switch( buf->data[pos] )
        {
            case '@':
                return AT_YES;
            case '/': case ' ': case '\r': case '\n':
                return AT_NO;
            default:
                break;
        }
        pos++;
    }
    return AT_AGAIN;
}


int http_parse_command( connection_t *con )
{
	enum {
        sw_start = 0,
        sw_method,
        sw_spaces_before_uri,
        sw_schema,
        sw_schema_slash,
        sw_schema_slash_slash,
        sw_check_at,
        sw_username,
        sw_password,
        sw_spaces_before_host,//used connect method
        sw_host,
        sw_port,
        sw_after_slash_in_uri,
        sw_check_uri,
        sw_uri,
        sw_http_09,
        sw_http_H,
        sw_http_HT,
        sw_http_HTT,
        sw_http_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_spaces_after_digit,
        sw_almost_done
    } /*state*/;
	chunk_t *ck;
	http_req_t *r;
	buffer_t *buf;
	char p;
	unsigned char c;
	//int i;
	//int m;
	//int bpos;

	//is chunk null
	ck = (chunk_t *) con->readbuf;
	if( ck==NULL ) return HTTP_PARSE_NULL;


	r = (http_req_t *)con->context;
	//isbuffer exist 
	buf = buffer_getfirst( ck );
	if( buf==NULL ) return HTTP_PARSE_NULL;
	
	//find the buffer to parse
	if( r->parse_buf!=NULL )
	{
		while( buf )
		{
			if( buf==r->parse_buf ) break;
			else buf = buffer_getnext( ck, buf );
		}
		//if not find , parse at begin
		if( buf==NULL ) 
		{
			buf = buffer_getfirst(ck);
			r->parse_pos = 0;
		}
		else
		{
			if( r->parse_pos>r->parse_buf->used ) 
			{
				buf = buffer_getnext( ck, r->parse_buf );
				if( buf!=NULL ) r->parse_pos = 0;
			}
		}
	}

	if( buf==NULL ) return HTTP_PARSE_AGAIN;
	r->parse_buf = buf;
	//current pos
	//bpos = r->parse_pos;
	while( buf )
	{
		while( r->parse_pos<buf->used )//i=r->parse_pos; i<(buf->used-r->parse_pos); i++ )
		{
			p = buf->data[r->parse_pos];
			r->parse_pos++;
			switch (r->parse_stat){
			case sw_start:
				switch (p){
				case '\r':
				case '\n':
					//do nothing
					break;
				default:
					//method must be GET etc. CAP
					if( (p<'A'|| p>'Z') && p!='-' )
						return HTTP_PARSE_ERR_METHOD;
					//push first method char
					HTTP_PUSH_METHOD(p);
					r->parse_stat = sw_method;
				}
				break;
			case sw_method:
				if( p==' ' )
				{
					//method is over, fetch the method type
					r->method_type = http_get_method( (char *)(r->method->data), r->method->used );
					//if method is not right , return err
					if( r->method_type==HTTP_METHOD_UNSET ) return HTTP_PARSE_ERR_METHOD;
					if( r->method_type==HTTP_METHOD_CONNECT ) r->parse_stat = sw_spaces_before_host;
					else r->parse_stat = sw_spaces_before_uri;
					break;
				}
				if( (p<'A'|| p>'Z') && p!='-' )
					return HTTP_PARSE_ERR_METHOD;
				HTTP_PUSH_METHOD(p);
				break;
			case sw_spaces_before_uri:
				if( p==' ' || p=='\t' ) break;
				if( p=='/' ) 
				{
					//uri begin without http,ftp etc.
					r->url->used = 1;
					r->url->data[0] = p;
					r->parse_stat = sw_uri;
					break;
				}
				c = p|0x20;
				if( c>='a' && c<='z')
				{
					//uri begin with http ftp or other schema
					r->url->used = 1;
					r->url->data[0] = c;
					r->parse_stat = sw_schema;
					HTTP_PUSH_SCHEMA(c);

					//set to ' '
					if( !mode_frontend )					
                                        {
                                            buffer_t *tmp = chunk_split_buffer( ck, buf, r->parse_pos );
                                            if( NULL != tmp )
                                            {
                                                // split current buffer, next is url
                                                buf = tmp;
                                                del_last_char_and_update_pos( ck, buf, r->parse_pos );
                                            }
                                            else
                                            {
                                                log_error( "split method & uri buffer failed" );
                                                return HTTP_PARSE_NULL;
                                            }
                                        }

					break;
				}
				else return HTTP_PARSE_ERR_URL;//illigle char
				break;

			case sw_schema:
				c = p | 0x20;
				if( c>='a' && c<='z')
				{
					HTTP_PUSH_SCHEMA(c);
					HTTP_PUSH_URL(c);
					
					// del
					if( !mode_frontend )	
                                            del_first_char_and_update_pos( ck, buf, r->parse_pos );

					break;
				}

				switch (p) {
				case ':':
					HTTP_PUSH_URL(p);
					r->parse_stat = sw_schema_slash;

					//del
					if( !mode_frontend )	
                                            del_first_char_and_update_pos( ck, buf, r->parse_pos );

					break;
				default:
					return HTTP_PARSE_ERR_URL;
				}
                                break;
     
			case sw_schema_slash:
				switch (p) {
				case '/':
					HTTP_PUSH_URL(p);
					r->parse_stat = sw_schema_slash_slash;
					if( !mode_frontend )	
                                            del_first_char_and_update_pos( ck, buf, r->parse_pos );

					break;
				default:
					return HTTP_PARSE_ERR_URL;
				}
				break;

			case sw_schema_slash_slash:
				switch (p) {
				case '/':
					HTTP_PUSH_URL(p);		
                                        r->parse_stat = schema_is_ftp(r->schema) ? sw_check_at : sw_host;

					if( ! mode_frontend )
                                            del_first_char_and_update_pos( ck, buf, r->parse_pos );
					break;
				default:
					return HTTP_PARSE_ERR_URL;
				}
				break;

                        case sw_check_at:
                                r->parse_pos--;
                                switch( check_at_forward( ck, buf, r->parse_pos ) )
                                {
                                    case AT_YES:
                                        r->parse_stat = sw_username;
                                        break;
                                    case AT_NO:
                                        r->parse_stat = sw_host;
                                        break;
                                    case AT_AGAIN:
                                        return HTTP_PARSE_AGAIN;
                                    case AT_ERROR:
                                        return HTTP_PARSE_NULL;
                                    default:
                                        return HTTP_PARSE_ERR_URL;
                                }
                                break;

                        case sw_username:
                                switch( p )
                                {
                                    case '@':
                                        r->parse_stat = sw_host;
                                        break;
                                    case ':':
                                        r->parse_stat = sw_password;
                                        break;
                                    default:
                                        HTTP_PUSH_USERNAME( p );
                                        break;
                                }
                                break;

                        case sw_password:
                                switch( p )
                                {
                                    case '@':
                                        r->parse_stat = sw_host;
                                        break;
                                    default:
                                        HTTP_PUSH_PASSWORD( p );
                                        break;
                                }
                                break;

			case sw_spaces_before_host:
				//connect method used
				if( p==' ' || p=='\t' ) break;
				//notice:no break
				r->parse_stat = sw_host;
                        case sw_host:
				c = (p | 0x20);
				if (c >= 'a' && c <= 'z') {
					HTTP_PUSH_HOST(c);
					HTTP_PUSH_URL(c);
					if( !mode_frontend )	
                                            del_first_char_and_update_pos( ck, buf, r->parse_pos );

					break;
				}

				if ((p >= '0' && p <= '9') || p == '.' || p == '-') {
					HTTP_PUSH_HOST(p);
					HTTP_PUSH_URL(p);
					if( !mode_frontend )	
                                            del_first_char_and_update_pos( ck, buf, r->parse_pos );

					break;
				}

				switch (p) {
				case ':':
					HTTP_PUSH_URL(p);
					r->parse_stat = sw_port;
					if( !mode_frontend )	
                                            del_first_char_and_update_pos( ck, buf, r->parse_pos );

					break;
				case '/':
					HTTP_PUSH_URL(p);
					r->parse_stat = sw_after_slash_in_uri;
					break;
				case ' ':
					//a blank after host
					r->parse_stat = sw_http_09;
					break;
				default:
					return HTTP_PARSE_ERR_URL;
				}
				break;

			case sw_port:
				if (p >= '0' && p <= '9') {
					HTTP_PUSH_PORT(p);
					HTTP_PUSH_URL(p);
					if( !mode_frontend )	
                                            del_first_char_and_update_pos( ck, buf, r->parse_pos );
	
					break;
				}

				switch (p) {
				case '/':
					HTTP_PUSH_URL(p);
					r->parse_stat = sw_after_slash_in_uri;
					break;
				case ' ':
					r->parse_stat = sw_http_09;
					break;
				default:
					return HTTP_PARSE_ERR_URL;
				}
				break;
				
			case sw_after_slash_in_uri:
			case sw_uri:
				//if (usual[ch >> 5] & (1 << (ch & 0x1f))) {
				//	HTTP_PUSH_URL(p);
				//	break;
				//}
				switch (p) {
				case ' ':
					r->parse_stat = sw_http_09;
					break;
				case '\r':
					r->parse_stat = sw_almost_done;
					break;
				case '\n':
					goto done;
				default:
					if( p>='A' && p<='Z' )
					{
						HTTP_PUSH_URL((p|0x20));
					}
					else 
					{
						HTTP_PUSH_URL(p);
					}
				}
				break;
			case sw_http_09:
				switch (p) {
				//case ' ':
				//	break;
				case '\r':
					r->parse_stat = sw_almost_done;
					break;
				case '\n':
					goto done;
				default:
					break;
				//case 'H':
				//	state = sw_http_H;
				//	break;
				//default:
				//	return HTTP_PARSE_ERR_PROTOCOL;
				}
				break;
			case sw_almost_done:
				switch (p) {
				case '\n':
					goto done;
				default:
					return HTTP_PARSE_ERR;
				}
												
			}//switch (r->parse_stat)

		}//for( i=bpos; i<(buf->used-bpos); i++ )
		
		//get next buf and reset parse_pos
		buf = buffer_getnext( ck, buf );
		
		if( buf!=NULL ) 
		{
			r->parse_buf = buf;
			r->parse_pos = 0;
		}
		
	}//while( buf )
	return HTTP_PARSE_AGAIN;
done:
	r->schema_type = http_get_schema( (char *)(r->schema->data), r->schema->used );
	if( r->schema_type==HTTP_SCHEMA_UNSET ) return HTTP_PARSE_ERR_SCHEMA;
	r->port_num = http_get_port( (char *)(r->port->data), r->port->used );
	if( r->port_num==-1 ) return HTTP_PARSE_ERR_PORT;
	return HTTP_PARSE_OK;
}

//是否为content-length标识符
int http_is_content_length( char *p, int len )
{
	if( len==14 )
		if( http_memcmp(p,"content-length",14)==0 ) return 1;
	return 0;
}
//对应的content-length的值
//@todo:too long number,strtoint64 need handle
int http_get_content_length( char *p, int len, uint64_t *clen )
{
	//MAX:18446744073709551615
	int i;
	if( len<=0 || len>20 ) return -1;
	for( i=0; i<len; i++ )
	{
		if( p[i]<'0' || p[i]>'9') return -1;
	}
	if( len==20 && p[0]!=0 )
	{
		p[20] = '\0';
		i = strcmp( p, "18446744073709551615" );
		if( i>0 ) return -1;
	}
	p[len] = '\0';

#ifndef strtoint64
#define strtoint64(x) strtoull(x,NULL,10)
#endif
	(*clen) = strtoint64( p );
	return 0;
}

#define http_save_req_head( req, name ) \
    if( (sizeof(#name)-1)==(req)->curheadname->used &&  \
            0==strncasecmp( (const char *)((req)->curheadname->data),   \
                            #name, sizeof(#name)-1 )    \
      ) { \
        if( 0==buffer_copy( (req)->curheadval, (req)->head_##name ) )   \
            (req)->head_##name->data[(req)->head_##name->used] = 0;     \
        else log_warn( "save http head: " #name " failed" );            \
    }

int http_parse_head( connection_t *con )
{
	static unsigned char lowcase[] =
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0-\0\0"
		"0123456789\0\0\0\0\0\0"
        "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
        "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	enum {
        sw_start = 0,
        sw_name,
        sw_space_before_value,
        sw_value,
        sw_space_after_value,
        sw_ignore_line,
        sw_almost_done,
        sw_header_almost_done
    } /*state*/;

	chunk_t *ck;
	http_req_t *r;
	buffer_t *buf;
	char p;
	unsigned char c;
	//int i;
	//int m;

	//is chunk null
	ck = (chunk_t *) con->readbuf;
	if( ck==NULL ) return HTTP_PARSE_NULL;


	r = (http_req_t *)(con->context);
	//isbuffer exist 
	buf = buffer_getfirst( ck );
	if( buf==NULL ) return HTTP_PARSE_NULL;
	
	//find the buffer to parse
	if( r->parse_buf!=NULL )
	{
		while( buf )
		{
			if( buf==r->parse_buf ) break;
			else buf = buffer_getnext( ck, buf );
		}
		//if not find , parse at begin
		if( buf==NULL ) 
		{
			buf = buffer_getfirst(ck);
			r->parse_pos = 0;
		}
		else
		{
			if( r->parse_pos>r->parse_buf->used ) 
			{
				buf = buffer_getnext( ck, r->parse_buf );
				if( buf!=NULL ) r->parse_pos = 0;
			}
		}
	}

	if( buf==NULL ) return HTTP_PARSE_AGAIN;
	r->parse_buf = buf;
	while( buf )
	{
		while( r->parse_pos<buf->used )//i=r->parse_pos; i<(buf->used-r->parse_pos); i++ )
		{
			p = buf->data[r->parse_pos];
			r->parse_pos++;
			switch (r->parse_stat)
			{
			case sw_start:
				switch( p ) {
				case '\r':
					r->parse_stat = sw_header_almost_done;
					break;
				case '\n':
					goto done;
				default:			
					c = lowcase[(unsigned char)p];
					if( !c ) return HTTP_PARSE_ERR_HEAD;
					HTTP_PUSH_HEADNAME(c);
					r->parse_stat = sw_name;
				}
				break;
			case sw_name:
				c = lowcase[(unsigned char)p];
				if (c) {
					HTTP_PUSH_HEADNAME(c);
					break;
				}
				if (p == ':') {
					r->parse_stat = sw_space_before_value;
					break;
				}
				if (p == '\r') {
					r->parse_stat = sw_almost_done;
					break;
				}

				if (p == '\n') {
					goto done;
				}
				return HTTP_PARSE_ERR_HEAD;
				break;
			case sw_space_before_value:
				switch (p){
				case ' ':
					break;
				case '\r':
					r->parse_stat = sw_almost_done;
					break;
				case '\n':
					goto done;
				default:
					HTTP_PUSH_HEADVAL(p);
					r->parse_stat = sw_value;
				}
                break;
			case sw_value:
				switch (p) {
				case ' ':
					r->parse_stat = sw_space_after_value;
					break;
				case '\r':
					r->parse_stat = sw_almost_done;
					break;
				case '\n':
					goto done;
				default:
					HTTP_PUSH_HEADVAL(p);
				}
				break;
			case sw_space_after_value:
				switch (p) {
				case ' ':
					break;
				case '\r':
					r->parse_stat = sw_almost_done;
					break;
				case '\n':
					goto done;
				default:
					HTTP_PUSH_HEADVAL(p);
					r->parse_stat = sw_value;
					break;
				}
				break;

			case sw_almost_done:
				switch (p) {
				case '\n':
					goto done;
				case '\r':
					break;
				default:
					return HTTP_PARSE_ERR_HEAD;
				}
				break;

			case sw_header_almost_done:
				switch (p) {
				case '\n':
					goto header_done;
				default:
					return HTTP_PARSE_ERR_HEAD;
				}
				break;
			}//switch (r->parse_stat)
		}//for( i=bpos; i<(buf->used-bpos); i++ )

		buf = buffer_getnext( ck, buf );
		if( buf!=NULL ) 
		{
			r->parse_buf = buf;
			r->parse_pos = 0;
		}
	}//while( buf )
	return HTTP_PARSE_AGAIN;
done:

	if( http_is_content_length((char *)(r->curheadname->data),r->curheadname->used)==1 )
	{
		r->content_length_exist = 1;
		if( http_get_content_length( (char *)(r->curheadval->data), 
			r->curheadval->used, &(r->content_sumlen) )!=0 )
			return HTTP_PARSE_ERR_HEAD;
	}
	
	r->curheadval->data[r->curheadval->used] = '\0';
	r->curheadname->data[r->curheadname->used] = '\0';
	log_debug( "%s:%s",r->curheadname->data,r->curheadval->data );

        http_save_req_head( r, host );

	r->parse_stat = sw_start;
	r->curheadval->used = 0;
	r->curheadname->used = 0;

	return HTTP_PARSE_OK;
header_done:
	
	if( r->method_type==HTTP_METHOD_POST && r->content_length_exist!=1 ) 
		return HTTP_PARSE_ERR;
	//r->parse_stat = sw_start;
	return HTTP_PARSE_HEADOVER;
}


#define get_default_port_by_contype( type ) \
( \
     CONTYPE_CLIENT==(type) ? 80 : \
     CONTYPE_SSL==(type) ? 443 :   \
     CONTYPE_FTP==(type) ? 21 : 0  \
)

int http_head_host_split( connection_t *c )
{
    http_req_t *r;
    char *host;
    char *port;

    r = c ? c->context : NULL;
    if( NULL==r )
        return -1;

    host = strchr( (const char *)(r->head_host->data), '@' );
    if( NULL==host )
        host = (char *)(r->head_host->data);
    else
        host += 1;

    port = strchr( host, ':' );
    if( NULL==port )
    {
        buffer_copy( r->head_host, r->host );
        buffer_reset( r->port );
        r->port_num = get_default_port_by_contype( c->type );
    }
    else
    {
        r->host->used = snprintf( (char *)(r->host->data), r->host->size, "%.*s", (int)(port-host), host );
        port += 1;
        r->port->used = snprintf( (char *)(r->port->data), r->port->size, "%s", port );
        sscanf( port, "%u", &(r->port_num) );
    }

    return 0;
}

char *get_host_addr( connection_t *c )
{
    static char host_addr[16];
    http_req_t *r;
    char *host;
    char *port;

    r = c ? c->context : NULL;
    if( NULL==r )
        return NULL;

    if( r->host->used > 0 ) return (char *)(r->host->data);

    host = strchr( (const char *)(r->head_host->data), '@' );
    if( NULL==host )
        host = (char *)(r->head_host->data);
    else
        host += 1;

    port = strchr( host, ':' );
    if( NULL==port )    snprintf( (char *)(&host_addr), 16, "%.*s", (int)(r->head_host->used), host );
    else snprintf( (char *)(&host_addr), 16, "%.*s", (int)(port-host), host );

    return (char *)(&host_addr);
}

