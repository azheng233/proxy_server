/** 
 * @file readconf.h
 * @brief 全局配置选项表及处理函数声明
 * @author sunxp
 * @version 0300
 * @date 2010-07-27
 */
#ifndef _READFONF_H_
#define _READFONF_H_

#include <netinet/in.h>

/**
 * @defgroup conf 
 * @brief 配置选项表
 *
 * - 配置选项表保存从配置文件中解析出的选项
 * - 配置选项表(\a conf)为一个数组，每个单元(\a struct opt_st)保存一条选项，包括名称、类型和值
 * - 用 conf[CONFIG_OPTION].value.type 的形式取得配置项的值
 */
///@{

#define OPT_MAX_N  512          ///< 选项最多个数
#define OPT_NAME_N 32           ///< 选项名称长度
#define OPT_VAL_N  1024         ///< 选项值长度

#define ARRAY_DELIM  ","        ///< array类型分割符

/// 选项值的类型
typedef enum {
    NULL_VAL,   ///< 空类型
    ARRAY,      ///< 数组类型
    BOOL,       ///< 布尔类型
    FLOAT,      ///< 浮点值类型
    NUMBER,     ///< 数值类型
    STRING      ///< 字符串类型
} val_type_t;

/// 配置选项表结构
typedef struct opt_st {
    char name[OPT_NAME_N];      ///< 选项名称
    val_type_t type;            ///< 选项类型
    union {
        char str[OPT_VAL_N];
        short array[0];
        int num;                ///< 数值或布尔值
        struct in_addr ip;      ///< ip地址
        float flt;
    } value;                    ///< 选项值
    void (*conv)( struct opt_st * ); ///< 转换函数
} *p_conf_t;

/// 所有支持的选项，支持多配置文件需按顺序
enum opt_name {
    DEBUG_MODE,                 ///< 调试模式，布尔
    /* global */
    MODE,                       ///< 运行模式，BOOL，默认frontend
    LOG_LEVEL,                  ///< 日志级别
    CONNECTION_TIMEOUT,         ///< 客户端和后端连接超时值，NUMBER，默认 60 秒
    CONNECTIONS_MAX,            ///< 最大连接数
    /* client */
    CLIENT_LISTEN_ADDR,         ///< 监听地址，STRING，默认 0.0.0.0
    CLIENT_LISTEN_PORT,         ///< 监听端口，NUMBER，默认 8000
    /* backend */
    BACKEND_SERVER_ADDR,        ///< 后端服务器地址，网栅，STRING，默认127.0.0.1
    BACKEND_SERVER_PORT,        ///< 后端服务器端口，NUMBER，默认8003
    /* ftp */
    FTP_LISTEN_ADDR,			///< ftp
    FTP_LISTEN_PORT,			///< ftp
    FTP_BACKEND_PORT,			///<
    /* ssl */
    SSL_USE_CLIENT_CERT,        ///< ssl使用客户端证书，BOOL，默认不使用
    SSL_LISTEN_ADDR,            ///< ssl监听地址，STRING，默认 0.0.0.0
    SSL_LISTEN_PORT,            ///< ssl监听端口，NUMBER，默认 443
    SSL_BACKEND_PORT,           ///< https后置机端口
    SSL_CA_CERTIFICATE,         ///< CA证书文件，STRING，
    SSL_SERVER_CERTIFICATE,     ///< 服务器证书文件，STRING，
    SSL_SERVER_KEY,             ///< 服务器私钥文件，STRING，
    /* rpc listen*/
    RPC_LISTEN_ADDR,            ///< rpc监听地址，STRING, 默认 0.0.0.0
    RPC_LISTEN_PORT,            ///< rpc监听端口，NUMBER，默认 9002
    RPC_CONN_TIMEOUT,           ///< rpc连接超时，NUMBER，默认 300 秒
    /* wateway */
    GATEWAY_LISTEN_ADDR,        ///< 网关监听地址，STRING, 默认 0.0.0.0
    GATEWAY_LISTEN_PORT,        ///< 网关监听端口，NUMBER，默认 8001
    GATEWAY_CONN_TIMEOUT,       ///< 网关连接超时，NUMBER，默认 86400 秒(一天)
    /* policy */
    POLICY_SERVER_ADDR,         ///< 策略服务器地址, STRING, 默认 127.0.0.1
    POLICY_SERVER_PORT,         ///< 策略服务器端口，NUMBER，默认8002
    POLICY_VERSION,             ///< 策略版本，支持1.3(13)、1.4(14)， 1.4(14)版本增加黑名单
    /* dns cache */
    DNS_CACHE_TIMEOUT,          ///< dns缓存超时时间，NUMBER, 默认3600
    /* ipfilter port */
    IPFILTER_PORT,              ///< ip过滤程序端口, 默认8009
    /* license */
    LICENSE_FILE,               ///< license文件路径
    /* audit log */
    AUDIT_SERVER_PORT,          ///< 审计日志服务器端口
    AUDIT_LOG_CYCLE,            ///< 审计日志上传周期，（流量日志）
    AUDIT_LOG_LEVEL,            ///< 审计日志级别，（url日志）
    /* url compare method */
    URL_CMP_METHOD,             ///< url匹配算法
    /* policy cahce size */
    POLICY_CACHE_SIZE,          ///< 策略缓存默认大小
    /* heartbeat */
    HEARTBEAT_PATH,             ///< 心跳文件路径
    HEARTBEAT_INTERVAL,         ///< 心跳间隔
    /* transparent */
    TRANSPARENT_ENABLE,         ///< 启用透明代理模式
    TRANSPARENT_INTERFACE,      ///< 透明代理抓包网卡
    /* uid name */
    UID_NAME,                   ///< 自定义用户标识字段名称
    /* end */
    OPT_ENUM_MAX                ///< 选项数目，数值
};

/// 配置项数组
extern p_conf_t conf;

int conf_init();
void conf_destroy();
int readallconfig();

#define mode_frontend \
    ( (const int)(conf[MODE].value.num) )

#define ssl_use_client_cert \
    ( (const int)(conf[SSL_USE_CLIENT_CERT].value.num) )

///@}
#endif
