#include "msg_audit.h"
#include "timehandle.h"
#include "message.h"
#include "conn_audit.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

///批量日志缓冲头长度
#define LOG_FLOW_BUF_HEAD   13

///批量日志缓冲数据长度
#define LOG_FLOW_BUF_MULTI  264

///单条日志缓冲总长度
#define LOG_FLOW_BUF_SINGLE 276

///时间
time_t curtime;

buffer_t * audit_log_flow_begin(unsigned int connection_count)
{
    unsigned char * log_buf_data = NULL;
    buffer_t * log_flow_buf = NULL;

    log_flow_buf = buffer_new(LOG_FLOW_BUF_HEAD + connection_count*LOG_FLOW_BUF_MULTI);
    if(log_flow_buf == NULL)
        return NULL;

    log_buf_data = log_flow_buf->data;
    log_buf_data += MSG_HEAD_SIZE; //jump the head
    *log_buf_data = 0; //snlen is 0, sn is null
    log_buf_data++;
    *log_buf_data = AUDIT_LOG_FLOW_ALL;
    log_buf_data++;
    *((uint32_t*)log_buf_data) = htonl(curtime);
    log_buf_data = NULL;
    log_flow_buf->used = LOG_FLOW_BUF_HEAD;	

    return log_flow_buf;
}

int audit_log_flow_append(buffer_t * buf, uint8_t *sn, uint8_t snlen, uint32_t upload, uint32_t download)
{
    unsigned char * log_buf_data = buf->data;
    log_buf_data += buf->used;
    *log_buf_data = snlen;
    log_buf_data++;
    memcpy(log_buf_data,sn,snlen);
    log_buf_data += snlen;
    *((uint32_t*)log_buf_data) = htonl(upload);
    log_buf_data += 4;
    *((uint32_t*)log_buf_data) = htonl(download);
    log_buf_data = NULL;
    buf->used += (1 + snlen + 8);

    return 0;
}

int audit_log_flow_send(buffer_t * buf)
{
    msg_add_head(buf->data,MSG_AUDITLOG,buf->used - MSG_HEAD_SIZE );
    int ret =  audit_send_buffer( buf );
    if( ret != 0 )
    {
        buffer_del( buf );
        return -1;
    }
    return 0;
}

int audit_log_flow(uint8_t *sn, uint8_t snlen, uint32_t upload, uint32_t download)
{
    unsigned char * log_buf_data = NULL;
    buffer_t * log_flow_buf = NULL;

    log_flow_buf = buffer_new(LOG_FLOW_BUF_SINGLE);
    if(log_flow_buf == NULL)
        return -1;

    log_buf_data = log_flow_buf->data;
    log_buf_data += MSG_HEAD_SIZE;   //jump the head
    *log_buf_data = snlen;
    log_buf_data++;
    memcpy(log_buf_data,sn,snlen);
    log_buf_data += snlen;
    *log_buf_data = AUDIT_LOG_FLOW_ONE;
    log_buf_data++;
    *((uint32_t*)log_buf_data) = htonl(curtime);
    log_buf_data += 4;
    *((uint32_t*)log_buf_data) = htonl(upload);
    log_buf_data += 4;
    *((uint32_t*)log_buf_data) = htonl(download);
    log_buf_data = NULL;
    log_flow_buf->used += (21 + snlen);

    msg_add_head(log_flow_buf->data,MSG_AUDITLOG,log_flow_buf->used - MSG_HEAD_SIZE );

    int ret = audit_send_buffer( log_flow_buf );
    if( ret != 0 )
    {
        buffer_del( log_flow_buf );
        return -1;
    }
    return 0;
}


int audit_log_url( uint8_t *sn, uint8_t snlen, char *url, uint16_t urlen, enum url_action action )
{
    buffer_t *buf = buffer_new( MSG_HEAD_SIZE+1+snlen+1+sizeof(uint32_t)+1+sizeof(uint16_t)+urlen );
    if( NULL == buf )
        return -1;

    uint8_t *p = buf->data + MSG_HEAD_SIZE;
    *p++ = snlen;
    memcpy( p, sn, snlen );
    p += snlen;
    *p++ = AUDIT_LOG_URL;
    *(uint32_t*)p = htonl( curtime );
    p += sizeof(uint32_t);
    *p++ = action;
    *(uint16_t*)p = htons( urlen );
    p += sizeof(uint16_t);
    memcpy( p, url, urlen );
    p += urlen;

    buf->used = p - buf->data;
    msg_add_head( buf->data, MSG_AUDITLOG, buf->used-MSG_HEAD_SIZE );
    
    int ret = audit_send_buffer( buf );
    if( ret != 0 )
    {
        buffer_del( buf );
        return -1;
    }
    return 0;
}
