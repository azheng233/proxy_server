#include "get_port_from_url.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PORT_LEN 8
#define PORT_MAX 65536

#define HTTP_DEAFULT_PORT 80
#define HTTPS_DEAFULT_PORT 443
#define FTP_DEAFULT_PORT 21

#define URL_UNUSUAL 0 //异常

#define schema_is_ftp(url,proto_len) ( 3==proto_len && ('f' == (*(url) | 0x20)) && \
        ('t' == (*((url)+1) | 0x20)) && ('p' == (*((url)+2) | 0x20)) )

#define schema_is_http(url,proto_len) ( 4==proto_len && ('h' == (*(url) | 0x20)) && \
        ('t' == (*((url)+1) | 0x20)) && ('t' == (*((url)+2) | 0x20)) && ('p' == (*((url)+3) | 0x20)) )

#define schema_is_https(url,proto_len) ( 5==proto_len && ('h' == (*(url) | 0x20)) && ('t' == (*((url)+1) | 0x20)) && \
        ('t' == (*((url)+2) | 0x20)) && ('p' == (*((url)+3) | 0x20)) && ('s' == (*((url)+4)| 0x20)) )

enum at_stat { AT_YES, AT_NO};

static enum at_stat check_at_forward(char *url, unsigned int i,size_t len)
{
    while(i<len)
    {

        switch( *((url)+i) )
        {
            case '@':
                return AT_YES;
            case '/':
                return AT_NO;
            default:
                break;
        }
        i++;
    }
    return AT_NO;
}

/**
 * @brief 从策路中得到端口号
 *
 * @param[in] url 联接
 * @param[in] len 链接长度
 *
 * 
 * @retval 返回端口
 * @retval 0
 */
uint16_t get_port_from_url( char *url, size_t len )
{
    enum {
        st_schema,
        st_schema_slash,
        st_schema_slash_slash,
        st_check_at,
        st_username,
        st_password,
        st_host,
        st_port,
    } stat = st_schema;
    char p;
    char c;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int proto_len=0;
    char url_port[PORT_LEN];
    uint16_t port;

    if(url == NULL || len<1)
        return URL_UNUSUAL;

    url_port[0]='\0';

    while(i<len)
    {
        p = *((url)+i);
        i++;
        switch(stat)
        {
            case st_schema:
                c = p | 0x20;
                if(c >='a' && c <='z')
                {
                    proto_len++;
                    break;
                }
                switch(p)
                {
                    case ':':
                        stat = st_schema_slash;
                        break;
                    default:
                        return URL_UNUSUAL;

                }
                break;
            case st_schema_slash:
                switch(p)
                {
                    case '/':
                        stat = st_schema_slash_slash;
                        break;
                    default:
                        return URL_UNUSUAL;

                }
                break;
            case st_schema_slash_slash:
                switch(p)
                {
                    case '/':
                        stat = schema_is_ftp(url,proto_len) ? st_check_at : st_host;
                        break;
                    default:
                        return URL_UNUSUAL;
                }
                break;
            case st_check_at:
                i--;
                switch(check_at_forward(url,i,len-i))
                {
                    case AT_YES:
                        stat = st_username;
                        break;
                    case AT_NO:
                        stat = st_host;
                        break;
                }
                break;
            case st_username:
                switch(p)
                {
                    case '@':
                        stat = st_host;
                        break;
                    case ':':
                        stat = st_password;
                        break;
                    default:
                        break;
                }
                break;
            case st_password:
                switch(p)
                {
                    case '@':
                        stat = st_host;
                        break;
                    default:
                        break;
                }
                break;
            case st_host:
                c=p | 0x20;
                if(c >= 'a' && c <= 'z')
                    break;
                if ((p >= '0' && p <= '9') || p == '.' || p == '-')
                    break;
                switch(p)
                {
                    case ':':
                        stat = st_port;
                        break;
                    case '/':
                        goto port_conv;
                    default:
                        return URL_UNUSUAL;
                }
                break;
            case st_port:
                if (p >= '0' && p <= '9')
                {
                    url_port[j]=p;
                    j++;
                    break;
                }
                switch(p)
                {
                    case '/':
                        url_port[j]='\0';
                        goto port_conv;
                    default:
                        return URL_UNUSUAL;
                }
                break;
            default:
                return URL_UNUSUAL;
        }
    }

port_conv:
    if( url_port[0] == '\0' )
    {
        if( schema_is_http( url, proto_len ) )
            return HTTP_DEAFULT_PORT;
        else if( schema_is_ftp( url, proto_len ) )
            return FTP_DEAFULT_PORT;
        else if( schema_is_https( url, proto_len ) )
            return HTTPS_DEAFULT_PORT;
    }
    else
    {
        port = atoi( url_port );
        if( port>0 && port<PORT_MAX )
            return port;
    }

    return URL_UNUSUAL;
}

#if 0
/**
 * 以上函数没有对username和password的字符进行判断
 */
int main()
{ 
    char *http_url="http://";
    char *https_url="https://wap.bank.ecitic.com:65534/mobilebank/index.html";
    char *ftp_url="ftp://ftp.neu.edu.cn:6666/bt.neu6.edu.cn.txt";
    char *ftp_url1="ftp://SMY_DL.04:lovesenmee04@smy04.senmee.com";
    uint16_t port=get_port_from_url(http_url,strlen(http_url));
    printf("port=%d\n",port);
    port=get_port_from_url(https_url,strlen(https_url));
    printf("port=%d\n",port);
    port=get_port_from_url(ftp_url,strlen(ftp_url));
    printf("port=%d\n",port);
    port=get_port_from_url(ftp_url1,strlen(ftp_url1));
    printf("port=%d\n",port);
}
#endif
