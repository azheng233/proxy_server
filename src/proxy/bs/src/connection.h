/** 
 * @file connection.h
 * @brief 连接
 * @author hps,sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "queue.h"
#include "bufqu.h"
#include "rbtree.h"
#include "log.h"
#include <netinet/in.h>
#include <stdint.h>

/**
 * @defgroup connection
 * @brief 连接
 *
 * 封装socket连接
 * @{
 */

#define CONN_OK   0
#define CONN_ERR  -1
#define CONN_AGN  1

typedef struct connection connection_t;
#include "waitconn.h"

/// 缓冲区大小限制
#define CHUNKSIZE_LIMIT  102400

/// 连接类型
enum conn_type {
    CONTYPE_CLIENT_LSN,         ///< 客户端监听
    CONTYPE_CLIENT,             ///< 客户端连接
    CONTYPE_SSL_LSN,
    CONTYPE_SSL,
    CONTYPE_SSL_BACKEND,
    CONTYPE_BACKEND,            ///< 与后端连接
    CONTYPE_RPC_LSN,            ///< 网关监听
    CONTYPE_RPC,                ///< 网关连接
    CONTYPE_GATEWAY_LSN,        ///< 网关监听
    CONTYPE_GATEWAY,            ///< 网关连接
    CONTYPE_POLICY,             ///< 与策略服务器连接
    CONTYPE_IPFILTER,
    CONTYPE_DNS,                ///< 后端机连接DNS程序
    CONTYPE_FTP_LSN,
    CONTYPE_FTP,
	CONTYPE_AUDIT				///<审计日志链接
};

/// 连接状态
enum conn_status {
    CONSTAT_INIT,               ///< 初始化
    CONSTAT_CONNECTING,         ///< 正在连接
    CONSTAT_HTTPCONNECT,
    CONSTAT_HANDSHAKE,
    CONSTAT_WORKING,            ///< 正常工作
    CONSTAT_CLOSING_WRITE,      ///< 关闭前返回错误页面
    CONSTAT_PEERCLOSED,         ///< 对等端连接关闭
    CONSTAT_CLOSED,             ///< 本连接关闭
    CONSTAT_ERROR               ///< 错误
};

/// 连接的时间信息
typedef struct timeinfo_s {
    time_t time_create;         ///< 创建时间
    time_t time_lastrec;        ///< 最后接收数据时间
    time_t time_lastsend;       ///< 最后发送数据时间
    time_t time_waitclose;      ///< @todo 未使用
}timeinfo_t;


/// 连接事件处理回调函数
typedef int (*fun_event_handle)( int, connection_t* );

/// 得到缓冲区大小，未使用
typedef int (*fun_get_leftsize)( void );

struct msg_head_format {
    unsigned max_size;          //消息体最大长度
    unsigned head_size;         //消息头长度
    unsigned len_pos;           //消息头长度位置
    unsigned len_size;          //消息头长度指示的长度
};

typedef struct readbuf_s
{
    unsigned int pos;		//当前位置
    unsigned int used;		//有效内容大小
    unsigned int size;		//缓存总长度

    struct msg_head_format head_fmt;
#define msgmax          head_fmt.max_size
#define msghead_len     head_fmt.head_size
#define msghead_lenpos  head_fmt.len_pos
#define msghead_lensize head_fmt.len_size

    int msglen;		//消息长度
    unsigned char *msgbuf;	//消息开始位置

    unsigned char *buf;		//读缓冲
} readbuf_t;

typedef struct writebuf_s
{
    buffer_queue_t *bufqu;//写队列
} writebuf_t;

/// 连接结构
struct connection {
    int fd;                     ///< socket描述符
    enum conn_type type;        ///< 类型
    enum conn_status status;    ///< 状态

    uint8_t version[2];

    struct sockaddr *addr;      ///< 地址和端口信息
    struct sockaddr *local_addr;///< 本地地址和端口信息

    timeinfo_t timeinfo;        ///< 时间信息

    int curevents;	        ///< 当前发生的事件, 未使用
    int lsnevents;	        ///< 当前监听的事件

    struct readbuf_s    rbuf;
    struct writebuf_s   wbuf;

    void *readbuf;	        ///< 读缓冲chunk_head
    void *writebuf;	        ///< 写缓冲chunk_head
    uint32_t upsize;            ///< 接收数据量
    uint32_t downsize;          ///< 发送数据量
    size_t mlog_up_flow;
    size_t mlog_down_flow;

    waitqueue_t waitq;	        ///< 等待队列
    waitnode_t wqnode;          ///< 等待节点

    void *peer;                 ///< 对端连接
    void *context;	        ///< 上下文
    void *ssl;          //ssl连接信息

    fun_event_handle event_handle;      ///< 事件处理
    fun_get_leftsize get_leftsize;      ///< 获取写缓冲剩余大小

    int action_prev;

    queue_t queue;
    rbtree_node_t *rbnode;
};

/// 连接队列头
typedef struct connhead_s {
    unsigned int num;           ///< 队列中连接数目
    queue_t queue;
} connhead_t;


int conn_queue_init( connhead_t *connhead );
void conn_queue_destroy( connhead_t *connhead );

connection_t *connection_new( int fd, enum conn_type type );
void connection_del( connection_t *conn );

int conn_add_queue( connhead_t *connhead, connection_t *conn );
int conn_del_queue( connhead_t *connhead, connection_t *conn );

/** 
 * @brief 设置连接状态
 * @param[in] conn 连接
 * @param[in] statu 连接状态
 */
#define set_conn_status( conn, statu ) \
    ( (conn)->status = (statu) )

int conn_timeupdate( connhead_t *connhead, connection_t *conn );

///@}
#endif
