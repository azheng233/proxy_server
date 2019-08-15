#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "readconf.h"
#include "http_parse.h"
#include "buffer.h"
#include "chunk.h"
#include "forward.h"
#include "log.h"
#include "conn_backend.h"
#include "job.h"
#include "fdevents.h"
#include "conn_ssl_backend.h"
#include "conn_peer.h"
#include "dns_parse.h"
#include "dnscache.h"
#include "monitor_log.h"
#include "conn_gateway.h"
#include "policy_cache.h"
#include "url_policy.h"
#include "hexstr.h"
#include "errpage.h"
#include <stdint.h>

//int http_handle_findpos( connection_t *con, buffer_t *endbuf, int endpos)

void buffer_print(char *name,buffer_t *buf)
{
	
	if( buf==NULL )
	{
		log_debug("%s:buf is null",name);
		return;
	}
	if( buf->used==0 )
	{
		log_debug("%s:buf used=0",name);
		return;
	}
	buf->data[buf->used]='\0';
	log_debug("%s:%s",name,buf->data);
	return;

}

void connection_req_print(connection_t *con)
{
	http_req_t *r;
	if( con==NULL ) return;
	r =(http_req_t *)( con->context);
	log_debug("connection:%d",con->fd);
	if( r==NULL )
	{
		log_warn("request is null");
		return;
	}
	
	buffer_print("url",r->url);
	buffer_print("method",r->method);
	buffer_print("host",r->host);
	buffer_print("port",r->port);
	buffer_print("schema",r->schema);
        if( r->username->used > 0 )
            buffer_print( "username", r->username );

	log_debug("cotent_sumlen:%d",r->content_sumlen);
}

#define http_req_buf_new( req, name, size ) \
    if( NULL==( (req)->name = buffer_new( size ) ) )  \
        return -1;

int connection_req_init( connection_t *con )
{
	//int size;
	http_req_t *r;
	//size = sizeof(http_req_t);
	//if( size%64!=0 ) size += 64-size%64;//the size should be n*64
	if( con==NULL ) return -1;
	con->context = malloc(sizeof(http_req_t));
	r = con->context;
	if( r==NULL ) return -1;

	r->stat = HTTP_STAT_COMMONDREQ;
	r->parse_buf = NULL;
	r->parse_pos = 0;
	r->parse_stat = 0;
	r->method_type = -1;
	r->schema_type = -1;
	r->port_num = -1;
	
	r->content_sumlen = 0;
	r->content_curlen = 0;
	r->content_length_exist = 0;

	r->url = buffer_new( HTTP_URL_MAX ); if(!r->url) return -1;
	r->method = buffer_new( HTTP_METHOD_MAX ); if(!r->method) return -1;
	r->host = buffer_new( HTTP_HOST_MAX ); if(!r->host) return -1;
	r->port = buffer_new( HTTP_PORT_MAX ); if(!r->port) return -1;
	r->schema = buffer_new( HTTP_SCHEMA_MAX ); if(!r->schema) return -1;
	r->curheadname = buffer_new( HTTP_HEADNAME_MAX ); if(!r->curheadname) return -1;
	r->curheadval = buffer_new( HTTP_HEADVAL_MAX ); if(!r->curheadval) return -1;
        r->username = buffer_new( HTTP_USERNAME_MAX ); if( NULL==r->username ) return -1;
        r->password = buffer_new( HTTP_PASSWORD_MAX ); if( NULL==r->password ) return -1;


	r->host_prev = buffer_new( HTTP_HOST_MAX ); if(!r->host_prev) return -1;
	r->port_prev = buffer_new( HTTP_PORT_MAX ); if(!r->port_prev) return -1;

	r->host_ssl = buffer_new( HTTP_HOST_MAX ); if(!r->host_ssl) return -1;
	r->port_ssl = buffer_new( HTTP_PORT_MAX ); if(!r->port_ssl) return -1;

        http_req_buf_new( r, head_host, HTTP_HOST_MAX );
        r->url_prev = buffer_new( HTTP_URL_MAX ); if(!r->url) return -1;

	return 0;
}

void connection_req_reset( connection_t *con )
{
	http_req_t *r;
	if( con==NULL ) return;
	r = con->context;
	if( r==NULL ) return;

	r->method_type = -1;
	r->schema_type = -1;
	r->port_num = -1;
	
	r->content_sumlen = 0;
	r->content_curlen = 0;
	r->content_length_exist = 0;

	r->url->used = 0;
	r->method->used = 0;
	r->host->used = 0;
	r->port->used = 0;
	r->schema->used = 0;
	r->curheadname->used = 0;
	r->curheadval->used = 0;
        buffer_reset( r->username );
        buffer_reset( r->password );
}

void connection_req_free( connection_t *con )
{
	http_req_t *r;
	if( con==NULL ) return;
	r = con->context;
	log_debug( "connection %d req free", con->fd );
	if( r==NULL ) return;
	buffer_del(r->url);
	buffer_del(r->method);
	buffer_del(r->host);
	buffer_del(r->port);
	buffer_del(r->schema);
	buffer_del(r->curheadname);
	buffer_del(r->curheadval);
	buffer_del(r->host_prev);
	buffer_del(r->port_prev);
	buffer_del(r->host_ssl);
	buffer_del(r->port_ssl);
        buffer_del( r->username );
        buffer_del( r->password );
        buffer_del( r->head_host );
        buffer_del( r->url_prev );
	free( r );
}


int checkip( char *url, int len )
{
    if( len<=0 || len >15 )
        return 0;
    if( url[0]<'0' || url[0]>'9' )
        return 0;

    struct in_addr ip;
    if( inet_aton( url, &ip ) <= 0 )
        return 0;
    //todo
    return 1;
}

int peer_create( connection_t *conn, buffer_t *host, int port, int isssl )
{
	struct sockaddr_in addr;
    uint32_t in_addr;
	connection_t *peer = NULL;
        dnsname_node_t *dnsname = NULL;
        int isip = checkip( (char *)(host->data), host->used );
        int need_dns = 0;

        memset( &addr, 0, sizeof(struct sockaddr_in) );
        addr.sin_family = AF_INET;
        addr.sin_port = htons( port );

        if( isip )
            inet_aton( (char *)(host->data), &(addr.sin_addr) );
        else
        {
            dnsname = dnscache_find((char *)host->data, host->used);
            if (NULL == dnsname) {
                need_dns = 1;
            } else {
                in_addr = *(uint32_t *)dnsname->ip;
                addr.sin_addr.s_addr = (in_addr_t)in_addr;
            }
        }

        if( need_dns )
        {
            if( isssl ) peer = ssl_backend_conn_create_noip( conn, &addr );
            else  peer = backend_conn_create_noip( conn, &addr );
        }
        else
        {
            if( isssl ) peer = ssl_backend_conn_create( conn, &addr );
            else  peer = backend_conn_create( conn, &addr );
        }

	if( NULL == peer )
	{
		log_warn( "connect to backend failed" );
		return -1;
	}
	conn->peer = peer;
	log_debug( "client %d connect to backend", conn->fd );

        if( need_dns )
        {
            if( dns_query( peer, (char *)host->data, host->used ) != DNS_OK )
            {
                log_warn( "make dns query failed" );
                return -1;
            }
            log_debug( "connection %d query dns, host:%s, port:%d", conn->fd, host->data, port );
        }

	return 0;
}

//copyflag:0,no,  1,yes
int http_handle_forwarddata(connection_t *con,int copyflag)
{
    //log_debug( "http_handle_parseandforwarddata, fd:%d", con->fd );
	//split buffer
	chunk_t *ck;
	buffer_t *buf;
	buffer_t *tmpbuf;
	http_req_t *r;
	chunk_t *ock;
	connection_t *peer;
	int left;
	if( con==NULL ) return -1;
	if( con->readbuf==NULL ) return -1;
	
    //log_debug( "http_handle_parseandforwarddata, fd:%d", con->fd );
	//@todo just the other socket is existed?
	peer = (connection_t *)(con->peer);
	if( peer==NULL ) return -1;
	ock = (chunk_t *)(peer->writebuf);
	if( ock==NULL ) return -1;

    //log_debug( "http_handle_parseandforwarddata, fd:%d", con->fd );
	ck = con->readbuf;
	buf = buffer_getfirst( ck );
	r = con->context;


	buffer_t *ebuf;
	int epos;
	int bodyleft;
	buffer_t *tmpbuf2;
	
	//cal the end buf and pos

	if( r->parse_buf==NULL ) 
	{
		//no head
		ebuf=buf;
		epos = 0;
		tmpbuf = ebuf;
		bodyleft = r->content_sumlen - r->content_curlen;
	}
	else
	{
		//with head
		ebuf = r->parse_buf;
		epos = r->parse_pos;
		//一开始剩余的体为总长度
		bodyleft = r->content_sumlen;
		//如果 bodyleft>全部长度-头长度,说明没有发送完毕,否则说明该分析有足够的body长度
		if( bodyleft>(ebuf->used-epos) )
		{
			//先计算可发送的长度ebuf->used-epos
			r->content_curlen = ebuf->used-epos;
			//改变最终位置为分组结尾
			epos = ebuf->used;
			//如果下一个缓冲区存在,则可以继续处理下一个
			tmpbuf = buffer_getnext( ck, ebuf );
			if( !tmpbuf )
			{
				ebuf = tmpbuf;
				epos = 0;
			}
		}
		else
		{
			//分组长度包括了所有要发送的内容
			//当前长度即为体长度
			r->content_curlen = bodyleft;
			//当前位置为 头结束位置+bodyleft
			epos = epos + bodyleft;
			//不需要再处理下面的分组
			tmpbuf = NULL;
		}
	}

	//下面处理每个分组,每个分组均从头开始(epos=0)
	while( tmpbuf!=NULL )
	{
		ebuf = tmpbuf;
		//如果剩余长度比当前分组大,则当前分组全部需要发送
		if( bodyleft>ebuf->used )
		{
			epos = ebuf->used;
			bodyleft -= ebuf->used;
			r->content_curlen += ebuf->used;
		}
		else
		{
			//如果剩余长度比当前分组小或等于,则只能部分发送,而且不需要再处理下面的分组
			epos = bodyleft;
			r->content_curlen += bodyleft;
			break;
		}
		tmpbuf = buffer_getnext( ck, ebuf );

	}

	//begin to send data
	while( buf )
	{
		tmpbuf2 = buffer_getnext( ck, buf );
		if( buf==ebuf )
		{
			if( ebuf->used==epos )
			{
				chunk_delbuffer( ck, buf );
				//copy data from one to another
				if( copyflag ) buffer_put( ock, buf );
                else buffer_del(buf);
			}
			else
			{
				//copy data from one to another
				if( copyflag )
				{
					tmpbuf = buffer_new( epos );
					memcpy( tmpbuf->data, buf->data, epos );					
					tmpbuf->used = epos;
					buffer_put( ock, tmpbuf );
				}
				
				//left buf 
				left = buf->used-epos;
				buf->used = left;
				//memcpy( buf->data, buf->data+epos, left );
                                buf->data += epos;
			}
			break;
		}
		else
		{
			tmpbuf = buffer_getnext( ck, buf );
			chunk_delbuffer( ck, buf );
			//copy data from one to another
			if( copyflag ) buffer_put( ock, buf );
            else buffer_del(buf);
		}
		buf = tmpbuf2;
	}
	if( peer->status == CONSTAT_WORKING )
		job_addconn( FDEVENT_OUT, peer );
	r->parse_buf = NULL;
	r->parse_pos = 0;
	return 0;
}


int insert_id_head( connection_t *con, char *userinfo, size_t len )
{
    int hlen;
    int leftlen;
    uint8_t *p;
    http_req_t *r = con->context;
    buffer_t *buf = r->parse_buf;//当前分组
    int pos = r->parse_pos;//当前结点

    if( pos<=0 )
        return -1;

    size_t morelen = len + HTTP_HEADNAME_MAX + 4;
    //申请一个新缓冲
    buffer_t *newbuf = buffer_new( buf->used+morelen );
    if( NULL == newbuf )
    {
        log_error( "alloc new head buffer failed" );
        return -1;
    }

    //复制前pos个数据
    p = newbuf->data;
    memcpy( p, buf->data, pos );
    p += pos;

    //添加自定义头域
    hlen = snprintf( (char*)p, morelen, "%s: %.*s\r\n", conf[UID_NAME].value.str, (int)len, userinfo );
    log_debug( "headstr: %.*s", hlen, p );
    p += hlen;

    //复制完head后，复制buf里后面的数据，大不为used-pos
    leftlen = buf->used - pos;
    memcpy( p, buf->data+pos, leftlen );
    p += leftlen;

    newbuf->used = p-newbuf->data;
    //插入新结点到头中
    buffer_put_head( (chunk_t *) con->readbuf, newbuf );
    //将原结点删除
    chunk_delbuffer( (chunk_t *)con->readbuf, buf );
    buffer_del( buf );

    //设置当前缓冲为newbuf
    //设置当前结点为r->parse_pos+headlen;
    r->parse_buf = newbuf;
    r->parse_pos = r->parse_pos+hlen;

    //unsigned char* find_cert_id_in_tree( unsigned char *ip, unsigned int *idlen );
    //int buffer_put_head( chunk_t *ck, buffer_t *buf );
    //int buffer_copy( buffer_t *from, buffer_t *to );

    return 0;
}

char * get_prev_url( connection_t *c )
{
    http_req_t *r;

    r = c ? c->context : NULL;
    if( !r )
        return "";

    return (char*)(r->url_prev->data);
}

char * get_req_url( connection_t *c, size_t *len )
{
    static char tmp_url[HTTP_URL_MAX];
    char port[16];
    http_req_t *r;
    char *url;
    size_t urlen;

    r = c ? c->context : NULL;
    if( !r )
    {
        tmp_url[0] = 0;
        url = tmp_url;
        urlen = 0;
    }
    else
    {
        r->url->data[r->url->used] = 0;

        if( c->type==CONTYPE_SSL )
        {
            //TODO https透明模式暂不支持
            if( r->port_ssl->used==3 \
                    && r->port_ssl->data[0]=='4' \
                    && r->port_ssl->data[1]=='4' \
                    && r->port_ssl->data[2]=='3'
              )
                port[0] = 0;
            else
                snprintf( port, 15, ":%s", r->port_ssl->data );

            if (ssl_use_client_cert) {
                urlen = snprintf( tmp_url, HTTP_URL_MAX, "https://%s%s", r->host_ssl->data, port );
            } else {
                urlen = snprintf( tmp_url, HTTP_URL_MAX, "https://%s%s%s", r->host_ssl->data, port, r->url->data );
            }
            url = tmp_url;
        }
        else
        {
            if( conf[TRANSPARENT_ENABLE].value.num )
            {
                urlen = snprintf( tmp_url, HTTP_URL_MAX, "http://%s%s", r->head_host->data, r->url->data );
                url = tmp_url;
            }
            else
            {
                url = (char *)r->url->data;
                urlen = r->url->used;
            }
        }
    }
    log_debug("url to judge:%s", url);

    if( len )
        *len = urlen;
    return url;
}

#if 0
static int echo_established(connection_t *c)
{
    int ret;

    ret = putmsg_connect_established(c->writebuf);
    if (ret) {
        log_warn( "genrate established page failed" );
        return -1;
    }
    log_debug( "send back established page" );

    job_addconn(FDEVENT_OUT, c);
    return 0;
}
#endif

static int echo_err_page(connection_t *c, struct policy_node *pnode, int err, char *url, int urlen, char *local_host)
{
#define ERR_MSG_MAX_LEN 1024*19
    static char msg[ERR_MSG_MAX_LEN];
    static char allowlink[ERR_MSG_MAX_LEN];
    char *s;
    int ret;

    if (!pnode) {
        log_error("gen error page failed, pnode null");
        return -1;
    }

    if (urlen > ERR_MSG_MAX_LEN / 2) {
        urlen = ERR_MSG_MAX_LEN / 2;
    }

    s = msg;
    s += snprintf(s, ERR_MSG_MAX_LEN, "当前访问的地址：%.*s 被阻止。<br/>原因是：", urlen, url);
    snprintf(allowlink, ERR_MSG_MAX_LEN, "http://%s:%d/", local_host, conf[CLIENT_LISTEN_PORT].value.num);

    switch (err) {
    case POLICY_JUDGE_FORBIDDEN:
        snprintf(s, ERR_MSG_MAX_LEN + msg - s, "该用户已被管理员禁止访问");
        break;
    case POLICY_JUDGE_IN_BLACKLIST:
        snprintf(s, ERR_MSG_MAX_LEN + msg - s, "访问地址在黑名单禁止访问策略内");
        break;
    case POLICY_JUDGE_NOT_IN_WHITELIST:
        snprintf(s, ERR_MSG_MAX_LEN + msg - s, "访问地址不在白名单策略允许范围内");
        break;
    case POLICY_JUDGE_TIME_INVALID:
        snprintf(s, ERR_MSG_MAX_LEN + msg - s, "访问时间不在本策略允许的时间段内");
        break;
    default:
        snprintf(s, ERR_MSG_MAX_LEN + msg - s, "其他错误 %d", err);
        break;
    }

    ret = putmsg_err(c->writebuf, HTTP_403_FORBIDDEN, msg, allowlink);
    if (ret != 0) {
        log_error("gen error page for user, user info: %s, sn: %s", pnode->userinfo, pnode->snstr);
        return -1;
    }
    log_info("send error page for user, user info: %s, sn: %s", pnode->userinfo, pnode->snstr);

    set_conn_status(c, CONSTAT_CLOSING_WRITE);
    return 0;
}

static int echo_allowlist_page(connection_t *c, struct policy_node *pnode)
{
#define ALLOWLIST_MAX_LEN ERR_MSG_MAX_LEN
    static char msg[ALLOWLIST_MAX_LEN];
    char *s;
    int ret;
    struct url_policy *purl;
    char *p;
    int i = 0;

    if (!pnode) {
        log_error("echo allow list page failed, pnode null");
        return -1;
    }

    s = msg;
    if (!pnode->whitelist) {
        s += snprintf(s, ERR_MSG_MAX_LEN + msg - s, "<tr><td align=\"left\" width=\"75%%\">当前用户没有可访问地址！</td></tr>");
        log_info("no whitelist url policy for user, user info: %s, sn: %s", pnode->userinfo, pnode->snstr);
        goto send;
    }

    purl = pnode->whitelist->policy;
    while (purl) {
        if (is_whitelist_policy_allow(pnode, purl)) {
            if (purl->url_len) {
                log_debug("pack url %d.%s, user info: %s, sn: %s", i, purl->url, pnode->userinfo, pnode->snstr);
                if (conf[TRANSPARENT_ENABLE].value.num) {
                    p = strstr(purl->url, "https");
                    if (p != purl->url) {
                        s += snprintf(s, ERR_MSG_MAX_LEN + msg - s, "<tr><td align=\"left\" width=\"75%%\"><a href=\"%s\">%d.%s</a></td></tr>", purl->url, i, purl->url);
                        i++;
                    }
                } else {
                    s += snprintf(s, ERR_MSG_MAX_LEN + msg - s, "<tr><td align=\"left\" width=\"75%%\"><a href=\"%s\">%d.%s</a></td></tr>", purl->url, i, purl->url);
                    i++;
                }
            }
        }
        purl = purl->next;
    }

send:
    ret = putmsg(c->writebuf, "当前可访问地址列表", msg);
    if (ret) {
        log_error("send whitelist url policy[%d] for user failed, user info: %s, sn: %s", i, pnode->userinfo, pnode->snstr);
        return -1;
    }
    log_info("send whitelist url policy[%d] for user, user info: %s, sn: %s", i, pnode->userinfo, pnode->snstr);

    set_conn_status(c, CONSTAT_CLOSING_WRITE);
    return 0;
}

int http_handle_parseandforwarddata(connection_t *con)
{
    int ret;
    http_req_t *r;
    int tocontinue;
    struct policy_node *pnode;
    char *url;
    size_t urlen;
    enum url_action action;
    char *host;
    char *local_host;

    pnode = NULL;
    if (mode_frontend) {
        pnode = policy_node_find_from_rbtree_byconn(con);
    }

    r = con->context;
    tocontinue = 1;
    while (tocontinue)
    {
        switch (r->stat)
        {
        case HTTP_STAT_COMMONDREQ:
            ret = http_parse_command(con);
            connection_req_print(con);
            switch (ret)
            {
            case HTTP_PARSE_AGAIN:
            case HTTP_PARSE_NULL:
                tocontinue = 0;
                log_debug("http parse agin or null");
                //@todo: verify max req
                break;
            case HTTP_PARSE_OK:
                log_debug("http parse ok");
                r->stat = HTTP_STAT_HEADREQ;
                r->parse_stat = 0;
                // 添加自定义头域
                if (pnode && strlen(pnode->userinfo)) {
                    ret = insert_id_head(con, pnode->userinfo, strlen(pnode->userinfo));
                    if (ret != 0) {
                        log_warn("insert id head failed");
                    }
                }
                break;
                //case HTTP_PARSE_ERR:
            default:
                log_error("http parse error");
                goto errparse;
                break;
            }
            break;
        case HTTP_STAT_HEADREQ:
            log_trace("===head one===");
            ret = http_parse_head(con);
            //connection_req_print(con);
            switch (ret)
            {
            case HTTP_PARSE_AGAIN:
            case HTTP_PARSE_NULL:
                log_debug("http parse again or null");
                tocontinue = 0;
                //@todo: verify max req
                break;
            case HTTP_PARSE_OK:
                log_trace("one buffer has been parsed");
                //continue
                break;
            case HTTP_PARSE_HEADOVER:
                log_trace("====http parse head over===");
                //@todo: justify if the url is right
                //if right split and forward it
                //if deny this connection, to said deny to client, and to continue
                /*
                if( r->method_type==HTTP_METHOD_CONNECT )
                {
                    //@todo if stat is connected goto err
                    //connect
                    //goto toconnect;
                    r->stat = HTTP_STAT_COMMONDREQ;
                    break;
                }
                */

                if (r->method_type == HTTP_METHOD_POST) {
                    if (r->content_length_exist == 0) {
                        goto errparse;
                    }
                }

                if (r->method_type == HTTP_METHOD_GET || r->method_type == HTTP_METHOD_CONNECT) {
                    if (r->content_length_exist == 1) {
                        if (r->content_sumlen > 0) {
                            goto errparse;
                        }
                    }
                }

                r->parse_stat = 0;

                //if no connect then connect
                r->host->data[r->host->used] = '\0';
                r->port->data[r->port->used] = '\0';
                //backend:if prev host is not empty, cmp cur host and prev host
                if (con->peer == NULL && (r->host)->used == 0 && 0 == r->head_host->used) {
                    log_debug("errforward: peer %p, host_used %u, head_host_used %u", \
                        con->peer, r->host->used, r->head_host->used);
                    goto errforward;
                }

                if (conf[TRANSPARENT_ENABLE].value.num) {
                    http_head_host_split(con);
                }

                if (mode_frontend)
                {
                    if (r->method_type == HTTP_METHOD_CONNECT)
                    {
                        if (CONTYPE_FTP == con->type) {
                            log_debug("not support http connect proxy for ftp now");
                            goto errurl;
                        }

                        buffer_copy(r->host, r->host_ssl);
                        r->host_ssl->data[r->host_ssl->used] = 0;
                        buffer_copy(r->port, r->port_ssl);
                        r->port_ssl->data[r->port_ssl->used] = 0;

                        if (ssl_use_client_cert) {
                            goto ssl_connect_judge;
                        }
                    } else {
ssl_connect_judge:
                        host = get_host_addr(con);
                        local_host = inet_ntoa(((struct sockaddr_in *)con->local_addr)->sin_addr);
                        
                        if (host) {
                            if (con->type == CONTYPE_SSL) {
                                host = (char *)(r->host_ssl->data);
                            }

                            log_trace("-------->host:%s, local addr:%s", host, local_host);
                            if (0 == strcmp(host, local_host)) {
                                if (pnode) {
                                    echo_allowlist_page(con, pnode);
                                }
                                con->action_prev = URL_DENY;
                                goto errurl;
                            }
                        }

                        url = get_req_url(con, &urlen);

                        //save url
                        strncpy((char*)(r->url_prev->data), url, urlen);
                        r->url_prev->data[urlen] = 0;
                        r->url_prev->used = urlen;

                        // judge url
                        ret = policy_judge(pnode, url, urlen);
                        if (ret != POLICY_JUDGE_ALLOWED) {
                            log_debug("judge url failed, sn: %s, error %d", pnode ? pnode->snstr : "", ret);
                            echo_err_page(con, pnode, ret, url, urlen, local_host);
                            action = URL_DENY;
                        } else {
                            action = URL_PASS;
                        }

                        if (pnode) {
                            if (!pnode->isforbidden) {
                                if (AUDIT_LOG_ALL == conf[AUDIT_LOG_LEVEL].value.num || (AUDIT_LOG_DENY == conf[AUDIT_LOG_LEVEL].value.num && URL_DENY == action)) {
                                    audit_log_url(pnode->sn, pnode->snlen, url, urlen, action);
                                }
                            }
                        }

                        if (pnode) {
                            mlog_log_url(pnode->snstr, url, action);
                            log_info("%s url: %s, user info: %s, sn: %s", (URL_PASS == action) ? "pass" : "deny", url, pnode->userinfo, pnode->snstr);
                        }

#ifndef DEBUG_ALLOW_ALL_URL
                        if (URL_PASS != action) {
                            con->action_prev = URL_DENY;
                            if (pnode) {
                                mlog_user_access_deny(pnode->snstr, url, 0, 0);
                            }
                            goto errurl;
                        }
                        con->action_prev = URL_PASS;
#endif
                    }
                } else {
                    //save url
                    buffer_copy(r->url, r->url_prev);
                    r->url_prev->data[r->url_prev->used] = 0;

                    if (r->host->used > 0)
                    {
                        if (r->host_prev->used > 0)
                        {
                            if (buffer_cmp(r->host_prev, r->host) != 0 || buffer_cmp(r->port_prev, r->port) != 0)
                            {
                                //close the peer 
                                if (NULL != con->peer)
                                {
                                    conn_signal_peerclose(con->peer);
                                    con->peer = NULL;
                                }
                                //set prev flag to last flag
                                buffer_copy(r->host, r->host_prev);
                                buffer_copy(r->port, r->port_prev);
                            }
                        } else {
                            buffer_copy(r->host, r->host_prev);
                            buffer_copy(r->port, r->port_prev);
                        }
                    }
                }

                //r->host->data[r->host->used] = '\0';
                if (con->peer == NULL)
                {
                    //connect diffent ip and port
                    if (mode_frontend)
                    {
                        buffer_t backend_host = {
                            .data = (unsigned char *)conf[BACKEND_SERVER_ADDR].value.str,
                            .used = strlen(conf[BACKEND_SERVER_ADDR].value.str)
                        };

                        int backend_port;
                        switch (con->type)
                        {
                        case CONTYPE_FTP:
                            backend_port = conf[FTP_BACKEND_PORT].value.num;
                            break;
                        case CONTYPE_SSL:
                            backend_port = conf[SSL_BACKEND_PORT].value.num;
                            break;
                        default:
                            backend_port = conf[BACKEND_SERVER_PORT].value.num;
                            break;
                        }

                        if (peer_create(con, &backend_host, backend_port, 0) != 0)
                            goto errforward;
                    } else {
                        //ssl connect
                        if (r->method_type == HTTP_METHOD_CONNECT)
                        {
                            if (peer_create(con, r->host, r->port_num, 1) != 0) {
                                goto errforward;
                            }
                        } else {
                            //none ssl
                            if (peer_create(con, r->host, r->port_num, 0) != 0) {
                                log_debug("errforward: create peer failed, %.*s:%hu", r->host->used, r->host->data, r->port_num);
                                goto errforward;
                            }
                        }
                    }
                } else if (((connection_t *)(con->peer))->status != CONSTAT_WORKING && ((connection_t *)(con->peer))->status != CONSTAT_CONNECTING && ((connection_t *)(con->peer))->status != CONSTAT_HANDSHAKE)
                {
                    log_debug("errforward: peer %d status %d", ((connection_t *)(con->peer))->fd, ((connection_t *)(con->peer))->status);
                    goto errforward;
                }


                if (r->content_length_exist == 1 && r->content_sumlen > 0)
                {
                    r->stat = HTTP_STAT_BODYREQ;
                } else {
                    if (mode_frontend)
                    {
                        if (http_handle_forwarddata(con, 1) != 0) {
                            goto errforward;
                        }
                    } else {
                        //backend need not forward connect message
                        if (r->method_type == HTTP_METHOD_CONNECT)
                        {
                            if (http_handle_forwarddata(con, 0) != 0) {
                                goto errforward;
                            }
                            if (ssl_use_client_cert) {
                                //echo_established(con);
                            }
                        } else {
                            int r = http_handle_forwarddata(con, 1);
                            if (r != 0) {
                                log_debug("errforward: http_handle_forwarddata return %d", r);
                                goto errforward;
                            }
                        }
                    }
                    r->stat = HTTP_STAT_COMMONDREQ;
                    connection_req_reset(con);
                }

                break;

                //case HTTP_PARSE_ERR:
            default:
                log_warn("http parse error");
                //@todo: errhandler
                goto errparse;
                break;
            }
            break;

        case HTTP_STAT_BODYREQ:
            //cal the split pos and split the chunk
            //if all buffer is done tocontinue = 0;
            //if not all buffer is done and body is end, tocontinue = 1;
            //@todo:do len
            log_debug("HTTP_STAT_BODYREQ, fd:%d", con->fd);
            if (chunk_isempty(con->readbuf))
            {
                tocontinue = 0;
                break;
            }
            {
                int r = http_handle_forwarddata(con, 1);
                if (r != 0)
                {
                    log_debug("errforward: http_handle_forwarddata return %d", r);
                    goto errforward;
                }
            }
            if (r->content_sumlen == r->content_curlen)
            {
                r->stat = HTTP_STAT_COMMONDREQ;
                connection_req_reset(con);
            }
            break;
        case HTTP_STAT_ERR:
        case HTTP_STAT_HEADOVERFLOW:
            tocontinue = 0;
            //@todo err handle,the stat can't in
        }
    }

    return HTTP_HANDLE_OK;
    //erroverflow:
    //	return HTTP_HANDLE_ERR_OVERFLOW;
errurl:
    return HTTP_HANDLE_ERR_URL;
errforward:
    return HTTP_HANDLE_ERR_FORWARD;
errparse:
    return HTTP_HANDLE_ERR_FORMAT;
}