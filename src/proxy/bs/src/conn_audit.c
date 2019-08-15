/**
 * @brief audit.c 审计日志处理函数实现
 * 
 * @author	zzl
 *
 * @date	2011-4-28
 *
 * @version	0.1
 */
#include "conn_audit.h"
#include "connection.h"
#include "log.h"
#include "conn_common.h"
#include "chunk.h"
#include "readconf.h"
#include "job.h"
#include "fdevents.h"
#include "timehandle.h"
#include "conn_io.h"
#include "head.h"
#include "message.h"
#include "hexstr.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

connection_t *audit_conn;

static int audit_handle_in( connection_t *c )
{
    int ret;
    uint8_t buf[MSG_HEAD_SIZE];
    char str[MSG_HEAD_SIZE*2+1];

    ret = read( c->fd, buf, MSG_HEAD_SIZE );
    log_debug( "audit connection %d read %d data, %s", c->fd, ret, strerror(errno) );
    if( ret > 0 )
    {
        hex2str( buf, ret, str, MSG_HEAD_SIZE*2 );
        log_trace( "data: %.*s", MSG_HEAD_SIZE*2, str );
    }
    return CONN_ERR;
}

int audit_handle( int events, connection_t *conn )
{
	int ret = 0;
	if( NULL==conn || conn != audit_conn )
	{
		log_warn( "wrong audit log connection" );
		return CONN_ERR;
	}

	if( FDEVENT_CLOSE & events )
	{
		log_debug( "audit event close, fd:%d", conn->fd );
		audit_conn_close();
		return CONN_OK;
	}

	if( FDEVENT_ERR & events )
	{
		log_debug( "audit event err, fd:%d", conn->fd );
		goto errclose;
	}

	if( FDEVENT_HUP & events )
	{
		log_debug( "audit event hup, fd:%d", conn->fd );
		goto errclose;
	}

	if( FDEVENT_IN & events )
	{
		log_debug( "audit event in, fd:%d", conn->fd );
		conn->timeinfo.time_lastrec = curtime;

                // audit connection do not recv data
		ret = audit_handle_in( conn );
                goto err;
	}

	if( FDEVENT_OUT & events )
	{
		log_debug( "audit event out, fd:%d", conn->fd );
		conn->timeinfo.time_lastsend = curtime;

		if( conn->status == CONSTAT_CONNECTING )
		{
			conn->status = CONSTAT_WORKING;
			ret = ev_modfd( conn->fd, FDEVENT_IN, conn );
                        log_trace( "audit connection %d connected", conn->fd );
		}
		if( conn->status == CONSTAT_WORKING )
		{
			ret = conn_handle_out( conn );
			if( CONN_OK != ret && CONN_AGN != ret )
				return CONN_ERR;
		}
		else goto err;
	}

	return CONN_OK;

err:
errclose:
	ret = conn_signal_close( conn );
	return CONN_ERR;
}

/** 
 * @brief 创建与日志服务器的连接
 * 
 * @retval CONN_ERR 发生错误
 * @retval CONN_OK  成功
 */
int audit_conn_create()
{
	audit_conn = NULL;

	int fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( fd < 0 )
	{
		log_error( "create audit socket failed, %s", strerror(errno) );
		return CONN_ERR;
	}

	int ret = setnonblocking( fd );
	if( ret != 0 )
	{
		log_error( "set audit socket %d nonblocking failed", fd );
		goto errfd;
	}
	audit_conn = connection_new( fd, CONTYPE_AUDIT);
	if( NULL == audit_conn )
	{
		log_error( "create new audit connection failed, fd:%d", fd );
		goto errfd;
	}

	audit_conn->event_handle = audit_handle;

	struct sockaddr_in *addr = malloc( sizeof(struct sockaddr_in) );
	if( NULL == addr )
	{
		log_error( "malloc audit addr failed" );
		goto errconn;
	}

	memset( addr, 0, sizeof(struct sockaddr_in) );
	addr->sin_family = AF_INET;
	inet_aton( conf[POLICY_SERVER_ADDR].value.str, &(addr->sin_addr) );
	addr->sin_port = htons( conf[AUDIT_SERVER_PORT].value.num );
	audit_conn->addr = (struct sockaddr *)addr;

	ret = conn_connect( audit_conn );
	if( ret != 0 )
	{
		log_error( "audit %d connect failed", fd );
		goto errconn;
	}

        log_debug( "audit connection %d created", fd );
	return CONN_OK;

errconn:
	connection_del( audit_conn );
	audit_conn = NULL;
errfd:
	close( fd );
	return CONN_ERR;
}


/** 
 * @brief 关闭与策略服务器的连接
 */
void audit_conn_close()
{
	if( NULL == audit_conn )
		return;
	log_debug( "audit connection %d closed", audit_conn->fd );
	connection_del( audit_conn );
	audit_conn = NULL;
}


void audit_check_alive()
{
    int ret;
    if( NULL == audit_conn )
    {
        ret = audit_conn_create();
        log_debug( "recreate audit connection %s", CONN_OK==ret ? "success" : "failed" );
    }
}


/**
 * @brief 发送处理函数
 * @
 * @param[in]	buf	发送缓冲区
 *
 * @retval	-1	发送错误
 * @retval	0	发送成功
 */
int audit_send_buffer( buffer_t *buf )
{
    if( NULL==audit_conn || NULL==audit_conn->writebuf )
        return -1;

    int ret = buffer_put( audit_conn->writebuf, buf );
    if( ret != 0 )
        return -1;

    job_addconn( FDEVENT_OUT, audit_conn );
    return 0;
}
