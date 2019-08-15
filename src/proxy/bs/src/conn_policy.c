#include "conn_policy.h"
#include "message.h"
#include "connection.h"
#include "log.h"
#include "conn_common.h"
#include "readconf.h"
#include "job.h"
#include "fdevents.h"
#include "timehandle.h"
#include "conn_io.h"
#include "head.h"
#include "chunk.h"
#include "msg_policy.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


connection_t *plcy_conn;


int policy_handle( int events, connection_t *conn )
{
	int ret = 0;
	if( NULL==conn || conn != plcy_conn )
	{
		log_warn( "wrong policy connection" );
		return CONN_ERR;
	}

	if( FDEVENT_CLOSE & events )
	{
		log_debug( "policy event close, fd:%d", conn->fd );
		policy_conn_close();
		return CONN_OK;
	}

	if( FDEVENT_ERR & events )
	{
		log_debug( "policy event err, fd:%d", conn->fd );
		goto errclose;
	}

	if( FDEVENT_HUP & events )
	{
		log_debug( "policy event hup, fd:%d", conn->fd );
		goto errclose;
	}

	if( FDEVENT_IN & events )
	{
		log_debug( "policy event in, fd:%d", conn->fd );
		conn->timeinfo.time_lastrec = curtime;

		ret = conn_handle_in( conn, policy_parse_data );
		if( CONN_OK != ret )
			goto err;
	}

	if( FDEVENT_OUT & events )
	{
		log_debug( "policy event out, fd:%d", conn->fd );
		conn->timeinfo.time_lastsend = curtime;

		if( conn->status == CONSTAT_CONNECTING )
		{
			conn->status = CONSTAT_WORKING;
			ret = ev_modfd( conn->fd, FDEVENT_IN, conn );
                        log_trace( "policy connection %d connected", conn->fd );
#ifdef REQUEST_ALL_POLICY
                        ret = plcy_request_all();
                        if( ret != 0 )
                            log_warn( "request all policy failed" );
                        else
                            log_info( "request all policy" );
#endif
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
 * @brief 创建与策略服务器的连接
 * 
 * @retval CONN_ERR 发生错误
 * @retval CONN_OK  成功
 */
int policy_conn_create()
{
	plcy_conn = NULL;

	int fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( fd < 0 )
	{
		log_error( "create policy socket failed %d, %s", errno, strerror(errno) );
		return CONN_ERR;
	}

	int ret = setnonblocking( fd );
	if( ret != 0 )
	{
		log_error( "set policy socket %d nonblocking failed", fd );
		goto errfd;
	}

	plcy_conn = connection_new( fd, CONTYPE_POLICY );
	if( NULL == plcy_conn )
	{
		log_error( "create new policy connection failed, fd:%d", fd );
		goto errfd;
	}

	plcy_conn->event_handle = policy_handle;

	struct sockaddr_in *addr = malloc( sizeof(struct sockaddr_in) );
	if( NULL == addr )
	{
		log_error( "malloc policy addr failed" );
		goto errconn;
	}

	memset( addr, 0, sizeof(struct sockaddr_in) );
	addr->sin_family = AF_INET;
	inet_aton( conf[POLICY_SERVER_ADDR].value.str, &(addr->sin_addr) );
	addr->sin_port = htons( conf[POLICY_SERVER_PORT].value.num );
	plcy_conn->addr = (struct sockaddr *)addr;

	ret = conn_connect( plcy_conn );
	if( ret != 0 )
	{
		log_error( "policy %d connect failed", fd );
		goto errconn;
	}

        log_debug( "policy connection %d created", fd );
	return CONN_OK;

errconn:
	connection_del( plcy_conn );
	plcy_conn = NULL;
errfd:
	close( fd );
	return CONN_ERR;
}


/** 
 * @brief 关闭与策略服务器的连接
 */
void policy_conn_close()
{
	if( NULL == plcy_conn )
		return;
	log_debug( "policy connection %d closed", plcy_conn->fd );
	connection_del( plcy_conn );
	plcy_conn = NULL;
}

void policy_check_alive()
{
    int ret;
    if( NULL == plcy_conn )
    {
        ret = policy_conn_create();
#ifndef DEBUG_ALLOW_ALL_URL
        log_debug( "recreate policy connection %s", CONN_OK==ret ? "success" : "failed" );
#endif
    }
}


int policy_send_buffer( buffer_t *buf )
{
    if( NULL==plcy_conn || NULL==plcy_conn->writebuf )
        return -1;

    int ret = buffer_put( plcy_conn->writebuf, buf );
    if( ret != 0 )
        return -1;

    job_addconn( FDEVENT_OUT, plcy_conn );
    return 0;
}

