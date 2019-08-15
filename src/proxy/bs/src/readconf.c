/** 
 * @file readconf.c
 * @brief 配置文件和选项表处理函数
 * @author sunxp
 * @version 0200
 * @date 2010-02-23
 */
#include "readconf.h"
#include "options.h"
#include "log.h"
#include "msg_audit.h"
#include "policy_cache.h"
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

p_conf_t conf = NULL;

static void log_level_value_conv( p_conf_t opt )
{
    enum log_level l;
    switch( opt->value.str[0] )
    {
        case 't': case 'T':
            l = LOG_LEVEL_TRACE;
            break;
        case 'd': case 'D':
            l = LOG_LEVEL_DEBUG;
            break;
        case 'i': case 'I':
            l = LOG_LEVEL_INFO;
            break;
        case 'w': case 'W':
            l = LOG_LEVEL_WARN;
            break;
        case 'e': case 'E':
            l = LOG_LEVEL_ERROR;
            break;
        default:
            log_info( "unknown log_level %s", opt->value.str );
            l = LOG_LEVEL_INFO;
            break;
    }
    opt->value.num = l;
}

static void audit_log_level_conv( p_conf_t opt )
{
    enum audit_log_level l;
    switch( opt->value.str[0] )
    {
        case 'n': case 'N':
            l = AUDIT_LOG_NONE;
            break;
        case 'd': case 'D':
            l = AUDIT_LOG_DENY;
            break;
        case 'a': case 'A':
            l = AUDIT_LOG_ALL;
            break;
        default:
            log_info( "unknown audit_log_level %s", opt->value.str );
            l = AUDIT_LOG_DENY;
            break;
    }
    opt->value.num = l;
}

/** 
 * @brief conf_init 初始化配置选项表
 * 
 * 分配内存，设置配置选项为默认值
 *
 * @return 结果状态
 * @retval -1 失败
 * @retval 0  成功
 */
int conf_init()
{
    conf = calloc( OPT_ENUM_MAX+1, sizeof(struct opt_st) );
    if( NULL == conf )
    {
        perror( "calloc conf struct" );
        return -1;
    }

    // 调试模式
    conf[DEBUG_MODE].type = BOOL;
    strncpy( conf[DEBUG_MODE].name, "debug_mode", OPT_NAME_N );
    conf[DEBUG_MODE].value.num = 0;

    // 日志级别
    conf[LOG_LEVEL].type = STRING;
    strncpy( conf[LOG_LEVEL].name, "log_level", OPT_NAME_N );
    conf[LOG_LEVEL].value.num = LOG_LEVEL_INFO;
    conf[LOG_LEVEL].conv = log_level_value_conv;

    // 运行模式，BOOL，默认frontend
    conf[MODE].type = BOOL;
    strncpy( conf[MODE].name, "mode", OPT_NAME_N );
    conf[MODE].value.num = 1;

    // 监听地址
    conf[CLIENT_LISTEN_ADDR].type = STRING;
    strncpy( conf[CLIENT_LISTEN_ADDR].name, "client_listen_addr", OPT_NAME_N );
    strncpy( conf[CLIENT_LISTEN_ADDR].value.str, "0.0.0.0", OPT_VAL_N );

    // 监听端口
    conf[CLIENT_LISTEN_PORT].type = NUMBER;
    strncpy( conf[CLIENT_LISTEN_PORT].name, "client_listen_port", OPT_NAME_N );
    conf[CLIENT_LISTEN_PORT].value.num = 8000;

    // 客户端和后端连接超时值
    conf[CONNECTION_TIMEOUT].type = NUMBER;
    strncpy( conf[CONNECTION_TIMEOUT].name, "connection_timeout", OPT_NAME_N );
    conf[CONNECTION_TIMEOUT].value.num = 60;

    // 最大连接数
    conf[CONNECTIONS_MAX].type = NUMBER;
    strncpy( conf[CONNECTIONS_MAX].name, "connections_max", OPT_NAME_N );
    conf[CONNECTIONS_MAX].value.num = 10240;

    //============================================================
    // rpc监听地址
    conf[RPC_LISTEN_ADDR].type = STRING;
    strncpy( conf[RPC_LISTEN_ADDR].name, "rpc_listen_addr", OPT_NAME_N );
    strncpy( conf[RPC_LISTEN_ADDR].value.str, "0.0.0.0", OPT_VAL_N );

    // rpc监听端口
    conf[RPC_LISTEN_PORT].type = NUMBER;
    strncpy( conf[RPC_LISTEN_PORT].name, "rpc_listen_port", OPT_NAME_N );
    conf[RPC_LISTEN_PORT].value.num = 9002;

    // rpc连接超时
    conf[RPC_CONN_TIMEOUT].type = NUMBER;
    strncpy( conf[RPC_CONN_TIMEOUT].name, "rpc_conn_timeout", OPT_NAME_N );
    conf[RPC_CONN_TIMEOUT].value.num = 300;
    //============================================================


    // 网关监听地址
    conf[GATEWAY_LISTEN_ADDR].type = STRING;
    strncpy( conf[GATEWAY_LISTEN_ADDR].name, "gateway_listen_addr", OPT_NAME_N );
    strncpy( conf[GATEWAY_LISTEN_ADDR].value.str, "0.0.0.0", OPT_VAL_N );

    // 网关监听端口
    conf[GATEWAY_LISTEN_PORT].type = NUMBER;
    strncpy( conf[GATEWAY_LISTEN_PORT].name, "gateway_listen_port", OPT_NAME_N );
    conf[GATEWAY_LISTEN_PORT].value.num = 8001;

    // 网关连接超时
    conf[GATEWAY_CONN_TIMEOUT].type = NUMBER;
    strncpy( conf[GATEWAY_CONN_TIMEOUT].name, "gateway_conn_timeout", OPT_NAME_N );
    conf[GATEWAY_CONN_TIMEOUT].value.num = 86400;

    // 策略服务器地址
    conf[POLICY_SERVER_ADDR].type = STRING;
    strncpy( conf[POLICY_SERVER_ADDR].name, "policy_server_addr", OPT_NAME_N );
    strncpy( conf[POLICY_SERVER_ADDR].value.str, "127.0.0.1", OPT_VAL_N );

    // 策略服务器端口
    conf[POLICY_SERVER_PORT].type = NUMBER;
    strncpy( conf[POLICY_SERVER_PORT].name, "policy_server_port", OPT_NAME_N );
    conf[POLICY_SERVER_PORT].value.num = 1950;

    // 策略版本
    conf[POLICY_VERSION].type = NUMBER;
    strncpy(conf[POLICY_VERSION].name, "policy_version", OPT_NAME_N);
    conf[POLICY_VERSION].value.num = 13;

    // ssl使用客户端证书
    conf[SSL_USE_CLIENT_CERT].type = BOOL;
    strncpy(conf[SSL_USE_CLIENT_CERT].name, "ssl_use_client_cert", OPT_NAME_N);
    conf[SSL_USE_CLIENT_CERT].value.num = 0;

    // ssl监听地址
    conf[SSL_LISTEN_ADDR].type = STRING;
    strncpy( conf[SSL_LISTEN_ADDR].name, "ssl_listen_addr", OPT_NAME_N );
    strncpy( conf[SSL_LISTEN_ADDR].value.str, "0.0.0.0", OPT_VAL_N );

    // ssl监听端口
    conf[SSL_LISTEN_PORT].type = NUMBER;
    strncpy( conf[SSL_LISTEN_PORT].name, "ssl_listen_port", OPT_NAME_N );
    conf[SSL_LISTEN_PORT].value.num = 443;

    // https后置机端口
    conf[SSL_BACKEND_PORT].type = NUMBER;
    strncpy( conf[SSL_BACKEND_PORT].name, "ssl_backend_port", OPT_NAME_N );
    conf[SSL_BACKEND_PORT].value.num = 8010;

    // ftp监听地址
    conf[FTP_LISTEN_ADDR].type = STRING;
    strncpy( conf[FTP_LISTEN_ADDR].name, "ftp_listen_addr", OPT_NAME_N );
    strncpy( conf[FTP_LISTEN_ADDR].value.str, "0.0.0.0", OPT_VAL_N );

    // ftp监听端口
    conf[FTP_LISTEN_PORT].type = NUMBER;
    strncpy( conf[FTP_LISTEN_PORT].name, "ftp_listen_port", OPT_NAME_N );
    conf[FTP_LISTEN_PORT].value.num = 8021;

    // ftp后端机端口
    conf[FTP_BACKEND_PORT].type = NUMBER;
    strncpy( conf[FTP_BACKEND_PORT].name, "ftp_backend_port", OPT_NAME_N );
    conf[FTP_BACKEND_PORT].value.num = 8020;

    // 后端服务器地址
    conf[BACKEND_SERVER_ADDR].type = STRING;
    strncpy( conf[BACKEND_SERVER_ADDR].name, "backend_server_addr", OPT_NAME_N );
    strncpy( conf[BACKEND_SERVER_ADDR].value.str, "127.0.0.1", OPT_VAL_N );

    // 后端服务器端口
    conf[BACKEND_SERVER_PORT].type = NUMBER;
    strncpy( conf[BACKEND_SERVER_PORT].name, "backend_server_port", OPT_NAME_N );
    conf[BACKEND_SERVER_PORT].value.num = 8010;

    // CA证书文件
    conf[SSL_CA_CERTIFICATE].type = STRING;
    strncpy( conf[SSL_CA_CERTIFICATE].name, "ssl_ca_certificate", OPT_NAME_N );
    strncpy( conf[SSL_CA_CERTIFICATE].value.str, "./ca.cer", OPT_VAL_N );

    // 服务器证书文件
    conf[SSL_SERVER_CERTIFICATE].type = STRING;
    strncpy( conf[SSL_SERVER_CERTIFICATE].name, "ssl_server_certificate", OPT_NAME_N );
    strncpy( conf[SSL_SERVER_CERTIFICATE].value.str, "./server.cer", OPT_VAL_N );

    // 服务器私钥文件
    conf[SSL_SERVER_KEY].type = STRING;
    strncpy( conf[SSL_SERVER_KEY].name, "ssl_server_key", OPT_NAME_N );
    strncpy( conf[SSL_SERVER_KEY].value.str, "./server.key", OPT_VAL_N );

    // dns缓存超时时间
    conf[DNS_CACHE_TIMEOUT].type = NUMBER;
    strncpy( conf[DNS_CACHE_TIMEOUT].name, "dns_cache_timeout", OPT_NAME_N );
    conf[DNS_CACHE_TIMEOUT].value.num = 3600;

    // ip过滤程序端口
    conf[IPFILTER_PORT].type = NUMBER;
    strncpy( conf[IPFILTER_PORT].name, "ipfilter_port", OPT_NAME_N );
    conf[IPFILTER_PORT].value.num = 8009;

    // license文件路径
    conf[LICENSE_FILE].type = STRING;
    strncpy( conf[LICENSE_FILE].name, "license_file", OPT_NAME_N );
    strncpy( conf[LICENSE_FILE].value.str, "./device.lic", OPT_VAL_N );


    // 日志服务器端口
    conf[AUDIT_SERVER_PORT].type = NUMBER;
    strncpy( conf[AUDIT_SERVER_PORT].name, "audit_server_port", OPT_NAME_N );
    conf[AUDIT_SERVER_PORT].value.num = 1960;

    // 审计日志上传周期，（流量日志）
    conf[AUDIT_LOG_CYCLE].type = NUMBER;
    strncpy( conf[AUDIT_LOG_CYCLE].name, "audit_log_cycle", OPT_NAME_N );
    conf[AUDIT_LOG_CYCLE].value.num = 120;

    // 审计日志级别，（url日志）
    conf[AUDIT_LOG_LEVEL].type = STRING;
    strncpy( conf[AUDIT_LOG_LEVEL].name, "audit_log_level", OPT_NAME_N );
    conf[AUDIT_LOG_LEVEL].value.num = AUDIT_LOG_DENY;
    conf[AUDIT_LOG_LEVEL].conv = audit_log_level_conv;


    // url匹配算法
    conf[URL_CMP_METHOD].type = STRING;
    strncpy( conf[URL_CMP_METHOD].name, "url_cmp_method", OPT_NAME_N );
    strncpy( conf[URL_CMP_METHOD].value.str, "slash", OPT_VAL_N );


    // 策略缓存默认大小
    conf[POLICY_CACHE_SIZE].type = NUMBER;
    strncpy( conf[POLICY_CACHE_SIZE].name, "policy_cache_size", OPT_NAME_N );
    conf[POLICY_CACHE_SIZE].value.num = POLICY_CACHE_DEFAULT_SIZE;

    // 心跳文件路径
    conf[HEARTBEAT_PATH].type = STRING;
    strncpy( conf[HEARTBEAT_PATH].name, "heartbeat_path", OPT_NAME_N );
    strncpy( conf[HEARTBEAT_PATH].value.str, "/tmp", OPT_VAL_N );

    // 心跳间隔
    conf[HEARTBEAT_INTERVAL].type = NUMBER;
    strncpy( conf[HEARTBEAT_INTERVAL].name, "heartbeat_interval", OPT_NAME_N );
    conf[HEARTBEAT_INTERVAL].value.num = 10;


    // 启用透明代理模式
    conf[TRANSPARENT_ENABLE].type = BOOL;
    strncpy( conf[TRANSPARENT_ENABLE].name, "transparent_enable", OPT_NAME_N );
    conf[TRANSPARENT_ENABLE].value.num = 0;

    // 透明代理抓包网卡
    conf[TRANSPARENT_INTERFACE].type = STRING;
    strncpy( conf[TRANSPARENT_INTERFACE].name, "transparent_interface", OPT_NAME_N );
    strncpy( conf[TRANSPARENT_INTERFACE].value.str, "eth1", OPT_VAL_N );


    // 自定义用户标识字段名称
    conf[UID_NAME].type = STRING;
    strncpy( conf[UID_NAME].name, "uid_name", OPT_NAME_N );
    strncpy( conf[UID_NAME].value.str, "PMobile-ID", OPT_VAL_N );

    //
    //conf[].type = STRING;
    //strncpy( conf[].name, "", OPT_NAME_N );
    //strncpy( conf[].value.str, "", OPT_VAL_N );

    //conf[].type = NUMBER;
    //strncpy( conf[].name, "", OPT_NAME_N );
    //conf[].value.num = ;

    //conf[SESSION_KEY].type = ARRAY;
    //strncpy( conf[SESSION_KEY].name, "session_key", OPT_NAME_N );
    //strncpy( conf[SESSION_KEY].value.str, "010101010101010101010101010101010101010101010101", OPT_VAL_N );

    return 0;
}

/** 
 * @brief conf_destroy 释放配置选项表
 */
void conf_destroy()
{
    if( conf )
        free( conf );
    conf = NULL;
}


/** 
 * @brief deconfline 解析一行
 *
 * 解析一行，匹配则填充相应配置表项
 * 
 * @param[in] option 配置选项表
 * @param[in] line[] 一行文本
 * @param[in] lino   行号，输出日志用
 * 
 * @return 结果状态
 * @retval -1 失败
 * @retval 0  成功
 */
static int deconfline( p_conf_t option, char line[], const unsigned int lino )
{
    char *opt = option->name;
    char *pos = line;
    int match = 0;
    int j = 0;
    int quotes = 0;
    const char delim[] = ARRAY_DELIM;

    for( pos=line; *pos; pos++ )
    {
        if( match < 1 )
        {
            if( *opt )
            {
                if( *opt == *pos)
                {
                    opt++;
                    continue;
                }
                else break;
            }
            match = 1;
        }
        if( match < 3 && isblank( *pos ) )
            continue;
        if( match < 2 )
        {
            if( *pos == '=' )
            {
                match = 2;
                continue;
            }
            else
            {
                fprintf( stderr, "syntax error: invalid character '%c' at %d of line %d\n", *pos, (int)(pos-line), lino );
                break;
            }
        }
        match++;

        char *s = NULL;
        char *p = pos;
        switch( option->type )
        {
            case ARRAY:
                s = strtok( p, delim );
                for( j=0; s && j<OPT_VAL_N/sizeof(short); j++ )
                {
                    sscanf( s, "%hx", &(option->value.array[j]) );
                    s = strtok( NULL, delim );
                }
                break;
            case BOOL:
                if( 0 == strncasecmp( p, "true", 4 ) || \
                        0 == strncasecmp( p, "yes", 3 ) || \
                        0 == strncasecmp( p, "on", 2 ) || \
                        0 == strncasecmp( p, "frontend", 8 ) || \
                        *p == '1' )
                    option->value.num = 1;
                else if( 0 == strncasecmp( p, "false", 5 ) || \
                        0 == strncasecmp( p, "no", 2 ) || \
                        0 == strncasecmp( p, "off", 3 ) || \
                        0 == strncasecmp( p, "backend", 7 ) || \
                        *p == '0' )
                    option->value.num = 0;
                else
                {
                    match--;
                    fprintf( stderr, "value %c is not bool, %d of line %d\n", *p, (int)(p-line-1), lino );
                }
                break;
            case NUMBER:
                sscanf( p, "%i", &(option->value.num) );
                break;
            case FLOAT:
                sscanf( p, "%f", &(option->value.flt) );
                break;
            case STRING:
                quotes = 0;
                for( j=0; *pos && (j < OPT_VAL_N-1); pos++ )
                {
                    if( *pos == '\n' || *pos == '\r' )
                        break;
                    if( *pos == '"' )
                    {
                        if( *(pos-1) != '\\' )
                        {
                            quotes++;
                            continue;
                        }
                        else j--;
                    }
                    if( quotes > 1 )
                        break;
                    option->value.str[j] = *pos;
                    j++;
                }
                option->value.str[j] = 0;
                if( NULL != option->conv )
                    option->conv( option );
                break;
            default:
                match--;
                fprintf( stderr, "unknown option type %d\n", option->type );
                break;
        }
        match++;
        if( match >= 3 )
            break;
    }
    return match<1 ? -1 : ( match<3 ? 1 : 0 );
}

/** 
 * @brief conf_read 解析配置文件，填充配置选项表
 * 
 * @return 结果状态
 * @retval -1 失败
 * @retval 0  成功
 */
static int conf_read( const char *filename, p_conf_t conf, unsigned int num )
{
    //char *filename = conf[CONF_FILE].value.str;
    FILE *cfile = fopen( filename, "r" );
    if( NULL == cfile )
    {
        fprintf( stderr, "connot open %s, %s\n", filename, strerror(errno) );
        return -1;
    }

    rewind( cfile );
    unsigned int lino = 0;
    char line[1200] = {0};
    int nopts = num;
    int i = 0;
    int n = -1;

    while( nopts>0 && fgets( line, 1200, cfile ) )
    {
        lino++;
        switch( line[0] )
        {
            case '#': case '\r': case '\n': case '\0':
                continue;
            case '[': //ini file
                continue;
            default:
                break;
        }
        if( ! isalpha( line[0] ) )
        {
            fprintf( stderr, "syntax error: invalid character '%c' at beginning of line %d in %s\n", line[0], lino, filename );
            continue;
        }

        for( i=0; i < num; i++ )
        {
            n = n<num-1 ? n+1 : 0;
            if( deconfline( conf+n, line, lino ) == 0 )
            {
                nopts--;
                break;
            }
        }
    }

    if( nopts )
        printf( "%d option(s) not list in %s, use default\n", nopts, filename );

    fclose( cfile );
    return 0;
}


/** 
 * @brief 读取所有配置
 * 
 * @retval -1 失败
 * @retval 0 成功
 */
int readallconfig()
{
    if( conf_read( config_file, conf+MODE, OPT_ENUM_MAX-MODE ) == -1 )
        return -1;

    return 0;
}
