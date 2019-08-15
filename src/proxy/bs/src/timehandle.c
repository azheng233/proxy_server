/**
 * @file timehandle.c
 * @brief 定时处理
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#include "timehandle.h"
#include "connection.h"
#include "conn_peer.h"
#include "conn_gateway.h"
#include "fd_rpc.h"
#include "conn_policy.h"
#include "conn_ipfilter.h"
#include "conn_dns.h"
#include "log.h"
#include "readconf.h"
#include "dnscache.h"
#include "license.h"
#include "monitor_log.h"
#include "policy_cache.h"
#include "conn_audit.h"
#include "heartbeat.h"
#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

time_t curtime;
extern connhead_t gw_connhead;  ///< 网关连接队列头
extern connhead_t rpc_connhead;  ///< rpc连接队列头
extern connection_t *dns_conn;  ///< 与DNS辅助程序连接
#ifdef ENABLE_IPFILTE_MANAGER
extern connection_t *ipf_conn;
#endif

/**
 * @brief 初始化时间和定时器
 *
 * @return 0
 */
int time_init()
{
    curtime = time(NULL);

    struct itimerval interval;
    interval.it_interval.tv_sec = TIMER_INTERVAL;
    interval.it_interval.tv_usec = 0;
    interval.it_value.tv_sec = TIMER_INTERVAL;
    interval.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &interval, NULL);
    return 0;
}


/**
 * @brief 定时处理函数
 *
 * 关闭超时的客户端和后端连接
 * 关闭超时的网关连接
 * 重连断开的策略服务器连接
 *
 * @return 0
 */
int time_handle()
{
    int ret = 0;
    static time_t flow_timestramp = 0;
    static time_t heartbeat_timestramp = 0;
    time_t latest;
    connection_t *conn;
    uint32_t flow_loged;

    if (license_verify_date(curtime) != 0)
    {
        log_debug("out of valid period");
        mlog_alert("out of valid period");
        kill(getpid(), SIGTERM);
        return 0;
    }

    // heartbeat
    if (curtime - heartbeat_timestramp >= conf[HEARTBEAT_INTERVAL].value.num)
    {
        hb_heartbeat();
        heartbeat_timestramp = curtime;
    }

    //log_trace( "total %d connections", connhead.num );
    struct policy_node *pnode;
    buffer_t * buf = NULL;
    int timeout = 1;

    //if connhead.num not zero, audit_log_flow_begin
    if (mode_frontend && connhead.num != 0)
    {
        buf = audit_log_flow_begin(connhead.num);
        flow_loged = 0;
    }

    // client & backend
    queue_t *q = connhead.queue.next;
    while (q != &(connhead.queue))
    {
        conn = queue_data(q, connection_t, queue);
        q = q->next;

        if (timeout)
        {
            latest = conn->timeinfo.time_lastrec > conn->timeinfo.time_lastsend ? \
                conn->timeinfo.time_lastrec : conn->timeinfo.time_lastsend;
            if (curtime - latest >= conf[CONNECTION_TIMEOUT].value.num)
            {
                log_debug("connection timeout, fd:%d, type:%d", conn->fd, conn->type);
                conn_signal_thisclose(conn);
            } else
            {
                log_trace("latest is %u, connection timeout:%d", latest, conf[CONNECTION_TIMEOUT].value.num);
                timeout = 0;
            }
        }

        if (mode_frontend && curtime - flow_timestramp >= conf[AUDIT_LOG_CYCLE].value.num)
        {
            //if the log flow is not null, get user's sn, append
            if (CONTYPE_CLIENT == conn->type && NULL != buf)
            {
                if (conn->upsize != 0 || conn->downsize != 0)
                {
                    pnode = policy_node_find_from_rbtree_byconn(conn);
                    if (pnode && !pnode->isforbidden)
                    {
                        audit_log_flow_append(buf, pnode->sn, pnode->snlen, conn->upsize, conn->downsize);
                        flow_loged++;

                        conn->upsize = 0;
                        conn->downsize = 0;
                    }
                }
            }
        }
    }
    if (NULL != buf)
    {
        if (flow_loged)
        {
            audit_log_flow_send(buf);
            flow_timestramp = curtime;
            log_debug("%u of %u connection(s) send flow log", flow_loged, connhead.num);
        } else buffer_del(buf);
    }

    if (!mode_frontend)
    {
        if (NULL == dns_conn)
        {
            log_debug("re connect dns");
            ret = dns_conn_create();
            if (CONN_OK != ret)
            {
                log_debug("dns connection create failed");
            }
        }

        dnscache_timeout(conf[DNS_CACHE_TIMEOUT].value.num);

        return 0;
    }

    if (mode_frontend)
    {
        q = rpc_connhead.queue.next;
        while (q != &(rpc_connhead.queue))
        {
            conn = queue_data(q, connection_t, queue);
            q = q->next;

            latest = conn->timeinfo.time_lastrec;
            if (curtime - latest >= conf[RPC_CONN_TIMEOUT].value.num)
            {
                log_debug("rpc connection timeout, fd:%d, type:%d", conn->fd, conn->type);
                rpc_signal_close(conn);
            } else
            {
                //log_debug( "latest is %u", latest );
                break;
            }
        }
    }

    // gateway
    //log_trace( "total %d gws", gw_connhead.num );
    q = gw_connhead.queue.next;
    while (q != &(gw_connhead.queue))
    {
        conn = queue_data(q, connection_t, queue);
        q = q->next;

        latest = conn->timeinfo.time_lastrec;
        if (curtime - latest >= conf[GATEWAY_CONN_TIMEOUT].value.num)
        {
            log_debug("gw connection timeout, fd:%d, type:%d", conn->fd, conn->type);
            gateway_signal_close(conn);
        } else
        {
            //log_debug( "latest is %u", latest );
            break;
        }
    }

#ifndef DEBUG_ALLOW_ALL_URL
    // policy
    policy_check_alive();
    // audit
    audit_check_alive();
#endif

#ifdef ENABLE_IPFILTE_MANAGER
    // ipfilter
    if (NULL == ipf_conn)
    {
        log_debug("re-connect ipfilter");
        ret = ipfilter_conn_create();
        if (CONN_OK != ret)
            log_debug("ipfilter connection create failed");
    }
#endif

    return 0;
}


const char *time_gmt()
{
    static char tmptimebuf[256];
    struct tm tm;
    int tlen;

    gmtime_r(&curtime, &tm);

    tlen = strftime(tmptimebuf, 256, "%A, %d-%b-%Y %H:%M:%S %Z", &tm);
    if (tlen <= 0)
        return "";

    return tmptimebuf;
}
