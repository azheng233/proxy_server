#include <stdio.h>
#include <string.h>
#include "chunk.h"
#include "queue.h"
#include "connection.h"
#include "iprbtree.h"
#include "head.h"
#include "job.h"
#include "fdevents.h"
#include "log.h"
#include "msg_policy.h"
#include "policy_cache.h"
#include "hexstr.h"
#include "user_entry.h"

//测试宏定义开始：
//#define FOR_TEST_PPZ	1
//#define EQUEL_READ 	1	//一个源buffer中刚好有一个完整的IP用户信息数据
//#define LARGE_READ 	1	//一个源buffer中有多个IP用户信息数据；
//#define LITTLE_READ	1	//一个源buffer中仅仅是一个完整的IP用户信息数据的一小部分；
//测试宏定义结束


//#define __DEBUG__	1
#define GW_RBUFFER_OK	1
#define GW_RBUFFER_NOT_OK	0
#define GW_BUFFER_HEADER_LEN	5//3
#define GW_BUFFER_LEN_POSITION	3//1
#define POLICY_SUBMIT_TYPE	0x91
#define POLICY_DELETE_TYPE	0x90
#define GW_MSG_BEGIN    0xc5
#define GW_MSG_END      0x5c
#define GW_MSG_PROTROL_VERSION  0x01

extern connection_t *plcy_conn;
#ifdef ENABLE_IPFILTE_MANAGER
extern connection_t *ipf_conn;
#endif
static buffer_rebuild_observ_t BufferRebuildObserv;
static buffer_state_t	BufferState; 

#ifdef __DEBUG__
static int print_cur_observ()
{
	printf("analysize (%d)\n", BufferRebuildObserv.next_buffer_analysized);
	printf("offset (%d)\n", BufferRebuildObserv.next_buffer_offset);
	printf("left header len (%d)\n", BufferRebuildObserv.needed_header_len);
	printf("left data len (%d)\n", BufferRebuildObserv.needed_data_len);
	return 0;
}

static char *TipBufferState(buffer_state_t state)
{
	switch(state)
	{
		case ANALYSIZE_BUFFER_HEADER:
			return "ANALYSIZE_BUFFER_HEADER";
		case READ_BUFFER_DATA:
			return "READ_BUFFER_DATA";
		case REBUILD_BUFFER_SUCCESS:
			return "REBUILD_BUFFER_SUCCESS";
		case NO_ENOUGH_BUFFER:
			return "NO_ENOUGH_BUFFER";
		default:
			return "unkown state";
	}
	return NULL;
}

static int print_buffer_content(buffer_t *buf)
{
	if(buf==NULL)
		return -1;

	int i;
	printf("++++++++++++++++++++++++++++\n\n");
	for(i=0; i<(int)buf->used; i++)
	{
		printf("(%c)[%x]\t", *(buf->data+i), (int)*(buf->data+i));
	}
	printf("\n\n++++++++++++++++++++++++++++\n\n");
	return 0;
}
#endif
static int repair_buffer_observ(int analysized, int offset, int left_header_len, int left_data_len)
{
	if(analysized >=0 )
		BufferRebuildObserv.next_buffer_analysized = analysized;
	if(offset >=0 )
		BufferRebuildObserv.next_buffer_offset = offset;
	if(left_header_len >=0 )
		BufferRebuildObserv.needed_header_len = left_header_len;
	if(left_data_len >=0 )
		BufferRebuildObserv.needed_data_len = left_data_len;
	return 0;	
}

static void log_user_io(const char *io, uint8_t *ip, uint8_t *sn, size_t snlen)
{
    char ipstr[32];
    char snstr[SN_MAX_LEN * 2 + 1];
    int slen = SN_MAX_LEN * 2;

    if (NULL == io || NULL == ip || NULL == sn)
        return;

    hex2str(sn, snlen, snstr, slen);

    if (0 == ip[0] && 0 == ip[1])
        sprintf(ipstr, "port: %hu", ntohs(*(uint16_t*)(ip + 2)));
    else
        sprintf(ipstr, "ip: %hhu.%hhu.%hhu.%hhu", ip[0], ip[1], ip[2], ip[3]);

    log_info("user %s, sn: %s, %s", io, snstr, ipstr);
}

#define log_user_login( ip, sn, snlen )  log_user_io( "login", (ip), (sn), (snlen) )
#define log_user_logout( ip, sn, snlen )  log_user_io( "logout", (ip), (sn), (snlen) )

static int deel_gw_buffer(buffer_t *buf)
{
    if (!buf) {
        log_warn("gateway user entry message buffer is null");
        return -1;
    }

    uint8_t *p = buf->data;
    if (GW_MSG_BEGIN != *p) {
        log_warn("gateway user entry message error, begin with 0x%02x", *p);
        return -1;
    }
    p++;
    uint8_t type = *p++;
    if (GW_MSG_PROTROL_VERSION != *p) {
        log_warn("gateway user entry message error, version 0x%02x wrong", *p);
        return -1;
    }
    p++;
    uint16_t msglen = ntohs(*(uint16_t*)p);
    if (msglen > buf->used) {
        log_warn("gateway user entry message error, length %hu bigger than buffer len %u", msglen, buf->used);
        return -1;
    }
    p += sizeof(uint16_t);
    uint8_t *ip = p;
    p += 4;
    uint8_t snlen = *p++;
    uint8_t *sn = p;
    p += snlen;

    if (GW_MSG_END != *p) {
        log_warn("gateway user entry message error, end with 0x%02x", *p);
        return -1;
    }
    msglen -= GW_BUFFER_HEADER_LEN + 1;

    switch (type)
    {
    case POLICY_SUBMIT_TYPE:
        log_user_login(ip, sn, snlen);
        policy_cache_userlogin(sn, snlen, *(uint32_t*)ip);
        break;
    case POLICY_DELETE_TYPE:
        log_user_logout(ip, sn, snlen);
        policy_cache_userlogout(sn, snlen, *(uint32_t*)ip);
        break;
    default:
        log_warn("unkown gateway user entry message, type 0x%hhx", type);
        break;
    }

#ifndef ENABLE_IPFILTE_MANAGER
    buffer_del(buf);
#else
    // forward to ipfilter
    buffer_put(ipf_conn->writebuf, buf);
    job_addconn(FDEVENT_OUT, ipf_conn);
    buf = NULL;
#endif
    return 0;
}

static int recompute_buffer_state()
{
	if(BufferRebuildObserv.next_buffer_analysized)
		BufferState = REBUILD_BUFFER_SUCCESS;
	else if(BufferRebuildObserv.needed_header_len > 0)
		BufferState = ANALYSIZE_BUFFER_HEADER;
	else if(BufferRebuildObserv.needed_data_len > 0)
		BufferState = READ_BUFFER_DATA;
	
	return 0;
}

static int try_read_rbuffer(chunk_t *chunk, buffer_t **buf)
{
	if(chunk == NULL)
	{
		log_warn( "chunk is null" );
		return GW_RBUFFER_NOT_OK;
	}
	
#ifdef __DEBUG__ 
	printf("GET NEW BUFFER START ...................................................................................................\n\n");
#endif
	buffer_t *tmp_buf = NULL;
	while(1)
	{
		if(BufferState != REBUILD_BUFFER_SUCCESS)//当上次循环已经产生一个完整的新的buffer时，这次循环就不再读取下一个源buffer了！
		{
			tmp_buf = buffer_get(chunk);
			if(tmp_buf == NULL)
				BufferState = NO_ENOUGH_BUFFER;		
		}

#ifdef __DEBUG__
		printf("CURRENT BufferState(%s)\n", TipBufferState(BufferState));
		print_cur_observ();
		print_buffer_content(tmp_buf);
#endif
		//getchar();
		switch(BufferState)
		{
			case ANALYSIZE_BUFFER_HEADER://还有头字节需要分析,可能在buffer开头去读开始的字节，可能是在buffer中间去读开始的字节，可能是在buffer开头去读头部的中间字节
						     //可能情况：（1）新buffer头部读取完之后，还有数据剩余；
						     //			<1> 剩余数据仅能读取data的一段；
						     //			<2> 剩余数据刚好读完data的全部信息；
						     //			<3> 剩余剩余读完data全部信息之后，还有数据；
						     //		 （2）新buffer头部读取完之后，没有有数据剩余；
						     //		 （3）新buffer头部信息没有读取完毕；
				{
					int tmp_need_head_len =  BufferRebuildObserv.needed_header_len;
					int tmp_start_buffer_offset = BufferRebuildObserv.next_buffer_offset;
					int tmp_left_total_len = tmp_buf->used - tmp_start_buffer_offset;
					if(tmp_left_total_len <= 0)//不会发生，出错！
					{
						log_warn("error tmp_start_buffer_offset in data");
						return GW_RBUFFER_NOT_OK;
					}

					unsigned char *p_des = &(BufferRebuildObserv.three_header_char[GW_BUFFER_HEADER_LEN-tmp_need_head_len]);
					unsigned char *p_src = tmp_buf->data;
					
					p_src +=  tmp_start_buffer_offset;//偏移到buffer头部信息处
					if(tmp_left_total_len >= tmp_need_head_len)
					{
						if(tmp_need_head_len)
						{
							memcpy(p_des, p_src, tmp_need_head_len);	//由此，已获得新的buffer剩余data字节数	
							p_des += tmp_need_head_len;
							*p_des = '\0';
							tmp_left_total_len -= tmp_need_head_len;
							p_src += tmp_need_head_len;
						}

						uint16_t *p_int = (uint16_t *)(p_des-(GW_BUFFER_HEADER_LEN-GW_BUFFER_LEN_POSITION));
						int new_buffer_data_len = ntohs( (uint16_t )*p_int);//将字符串转化为整数
						log_trace("new gw data len %d", new_buffer_data_len);
						
						*buf = buffer_new((size_t)new_buffer_data_len);//当头部信息全部读完之后才创建新的buffer 
						if(*buf == NULL)
						{
							return GW_RBUFFER_NOT_OK;
						}
						unsigned char *pd_data = (*buf)->data;
						(*buf)->used = new_buffer_data_len;//填入有效长度；
						new_buffer_data_len -= GW_BUFFER_HEADER_LEN; //减去头部字节，剩余数据字节
						memcpy(pd_data, BufferRebuildObserv.three_header_char, GW_BUFFER_HEADER_LEN);//将头部GW_BUFFER_HEADER_LEN个字节的信息拷贝到新的buffer里面去；
						pd_data += GW_BUFFER_HEADER_LEN;//往后偏移GW_BUFFER_HEADER_LEN个字节

						if(tmp_left_total_len > new_buffer_data_len)//读完一个新buffer后，源buffer里还有数据
						{
							memcpy(pd_data, p_src, new_buffer_data_len);
							repair_buffer_observ(1, (p_src - tmp_buf->data +new_buffer_data_len), GW_BUFFER_HEADER_LEN, -1);//新的buffer头部出现，最后一个参数值在这种情况下无效！
							BufferRebuildObserv.p_data_spy = pd_data+new_buffer_data_len;

						}else if(tmp_left_total_len == new_buffer_data_len)//源buffer里刚好有一个新buffer数据信息
						{
							memcpy(pd_data, p_src, new_buffer_data_len);	
							repair_buffer_observ(1, 0, GW_BUFFER_HEADER_LEN, 0);
							BufferRebuildObserv.p_data_spy = pd_data+new_buffer_data_len;
						}else//源buffer中的数据不够新buffer信息
						{
							memcpy(pd_data, p_src, tmp_left_total_len);	
							repair_buffer_observ(0, 0, 0, (new_buffer_data_len-tmp_left_total_len));
							BufferRebuildObserv.p_data_spy = pd_data+tmp_left_total_len;
							//数据不够组成一个新buffer的话，从新循环
						}
					}
					else
					{
						memcpy(p_des, p_src, tmp_left_total_len);
						repair_buffer_observ(0, 0, (tmp_need_head_len-tmp_left_total_len), -1);//头部还没读完,若刚刚读完，则状态不变
						//数据不够组成一个新buffer的话，从新循环
					}	
					//fprintf(stderr, "================line(%d)-----fucntion(%s)------file(%s)\n", __LINE__, __FUNCTION__, __FILE__);
				}
				break;
			case READ_BUFFER_DATA://还有新buffer data数据要读取,必定是从一个buffer的开头去读的!不会是从buffer的中间去读！
					      //可能情况：（1）新buffer data数据读取完之后，还有数据剩余；
					      //	  （2）新buffer data数据读取完之后，没有数据剩余；
					      //	  （3）新buffer data数据没有读完；
					      
				{
					int tmp_total_left_len = tmp_buf->used;
					int tmp_need_data_len = BufferRebuildObserv.needed_data_len;
					unsigned char *ps_data = tmp_buf->data;
					unsigned char *pd_data = BufferRebuildObserv.p_data_spy;

					if(pd_data==NULL || ps_data==NULL)//不会发生，否则出错
					{
						log_warn("error BufferRebuildObserv.p_data_spy");
						return GW_RBUFFER_NOT_OK;
					}
					if(tmp_total_left_len > tmp_need_data_len)
					{
						memcpy(pd_data, ps_data, tmp_need_data_len);
						repair_buffer_observ(1, tmp_need_data_len, GW_BUFFER_HEADER_LEN, -1);//下一个新的buffer出现在源buffer中间位置；
						pd_data += tmp_need_data_len;
					
					}else if(tmp_total_left_len == tmp_need_data_len)
					{
						memcpy(pd_data, ps_data, tmp_need_data_len);
						repair_buffer_observ(1, 0, GW_BUFFER_HEADER_LEN, 0);
						pd_data += tmp_need_data_len;
					}else
					{
						memcpy(pd_data, ps_data, tmp_total_left_len);
						repair_buffer_observ(0, 0, 0, (tmp_need_data_len-tmp_total_left_len));
						pd_data += tmp_total_left_len; 
						//数据不够组成一个新buffer的话，从新循环
					}
					BufferRebuildObserv.p_data_spy = pd_data;
				}
				break;//有完整的一个新buffer产生的话，切换循环状态
			case REBUILD_BUFFER_SUCCESS://（1）送出成功构建的buffer,
						    //（2）重置循环状态
				{
#ifdef __DEBUG__
					print_buffer_content(*buf);
#endif
					log_trace("success gw buf end!");
					repair_buffer_observ(0, -1, -1, -1);
				}
				recompute_buffer_state();
				return GW_RBUFFER_OK;
			case NO_ENOUGH_BUFFER: //
					       //（1）将不完整的buffer放到原来的chunk上，并处理相关细节
					       //（2）并清除BufferState状态，以备下次调用该函数的应用
				{
					recompute_buffer_state();//根据情况处理未完成的新buffer	
					switch(BufferState)
					{
						case ANALYSIZE_BUFFER_HEADER://因为头部字节没有读完，所以新的buffer还没有创建起来，这时要创建新的buffer，并将头部字节填入
							{
								int tmp_need_head_len = BufferRebuildObserv.needed_header_len;
								if(tmp_need_head_len <GW_BUFFER_HEADER_LEN)//等于GW_BUFFER_HEADER_LEN的话，说明还没有读下一个新buffer的头部呢！
								{
									*buf = buffer_new(GW_BUFFER_HEADER_LEN-tmp_need_head_len);
									if(*buf==NULL)
										return GW_RBUFFER_NOT_OK;

									(*buf)->used = GW_BUFFER_HEADER_LEN-tmp_need_head_len;
									memcpy((*buf)->data, BufferRebuildObserv.three_header_char, GW_BUFFER_HEADER_LEN-tmp_need_head_len);
								}
							
							}
							break;
						case READ_BUFFER_DATA://已经读了data数据的情况，但是data数据没有读完！这时，其实已经创建了一个新的buffer了！
							{
								(*buf)->used = BufferRebuildObserv.p_data_spy - (*buf)->data; 
#ifdef __DEBUG__
								print_buffer_content(*buf);
#endif
								log_warn("Need not create new buffer!Created Buf len %d", (*buf)->used );
							}
							break;
						default:
							break;
					}
					if(*buf)
					{
						buffer_put(chunk, *buf);
						log_warn("I have resume the buffer, and have hung it up!");
					}
					repair_buffer_observ(0, 0, 0, 0);
				}
				return GW_RBUFFER_NOT_OK;
			default:
				return GW_RBUFFER_NOT_OK;
		}
        recompute_buffer_state();
        if ((BufferState == REBUILD_BUFFER_SUCCESS) && (BufferRebuildObserv.next_buffer_offset > 0)) {
            if (tmp_buf) {
                log_debug("resume last tmp_buf");
                buffer_put_head(chunk, tmp_buf);
            }
        } else {
            buffer_del(tmp_buf);//从源chunk上去buffer，用过之后如果没有用处则释放
        }
	}
	return GW_RBUFFER_NOT_OK;
}

#ifdef FOR_TEST_PPZ
int gateway_rbuffer_read(chunk_t *p_chunk)
{
#else
int gateway_rbuffer_read(struct connection *gateway_connect)
{
	if(gateway_connect == NULL)
	{
		log_warn( "gateway connection is NULL" );
		return -1;
	}
	chunk_t *p_chunk = (chunk_t *)gateway_connect->readbuf;
#endif
	if(p_chunk == NULL)
	{
		log_warn( "gateway readbuf is NULL" );
		return -1;
	}


	int read_rbuffer_ok = GW_RBUFFER_OK;

	memset(&BufferRebuildObserv, 0, sizeof(buffer_state_t));
	repair_buffer_observ(0, 0, GW_BUFFER_HEADER_LEN, 0);
	BufferState = ANALYSIZE_BUFFER_HEADER;

	while(read_rbuffer_ok == GW_RBUFFER_OK)
	{
		buffer_t *p_cur_buf=NULL;
		read_rbuffer_ok = try_read_rbuffer(p_chunk, &p_cur_buf);
		if(read_rbuffer_ok == GW_RBUFFER_OK)
		{
			deel_gw_buffer(p_cur_buf);
		}
		//getchar();
	}

	return 0;
}

#ifdef FOR_TEST_PPZ 
#define MAX_LEN 256

#ifdef EQUEL_READ
static int get_two_byte_len(unsigned char *src)
{
	if(src == NULL)
	return 0;

	int tmp_val = 0;
	short int *p = NULL;
	p = (short int *)src;//2010-7-28_8:52

	tmp_val = (short int)*p;

	return tmp_val;

}
#endif
int main()
{
	int chunk_size = 100;
	//int real_size = 20;

	chunk_t *gateway_chunk = chunk_create(chunk_size);
	if(gateway_chunk == NULL)
		return -1;
	

	FILE *fd = NULL;
	fd = fopen("./ipdata/ip_policy", "rb");
	if(fd == NULL)
	{
		printf("open ./ipdata/ip_policy error!\n");
		return -1;
	}
	
	fprintf(stderr, "================line(%d)-----fucntion(%s)------file(%s)\n", __LINE__, __FUNCTION__, __FILE__);
	char key_string[2][MAX_LEN];

#ifdef LITTLE_READ
#define CONST_READ_LEN 6
	while(!feof(fd))
	{
		buffer_t *p_buf = NULL;
		memset(key_string[0], 0, MAX_LEN);

		int read_len = 0;
		read_len = fread(key_string[0], 1, CONST_READ_LEN, fd);
	/*	
		
		if(read_len != CONST_READ_LEN)//测试最后一个IP信息在源buffer里不完整的情况
		{
			printf("file over!read_len =(%d)\n", read_len);
		       	break;	
		}*/
		p_buf = buffer_new(read_len);
		if(p_buf==NULL)
		{
			printf("buffer_new error!\n");
			return -1;
		}

		printf("read_len(%d)\n", read_len);
		memcpy(p_buf->data, key_string[0], read_len);
		p_buf->used = read_len;
		buffer_put(gateway_chunk, p_buf);

		if(read_len != CONST_READ_LEN)
		{
			printf("file over!read_len =(%d)\n", read_len);
		       	break;	
		}
	}
#endif

#ifdef LARGE_READ
#define CONST_READ_LEN 100

	while(!feof(fd))
	{
		buffer_t *p_buf = NULL;
		memset(key_string[0], 0, MAX_LEN);

		int read_len = 0;
		read_len = fread(key_string[0], 1, CONST_READ_LEN, fd);
		p_buf = buffer_new(read_len);
		if(p_buf==NULL)
		{
			printf("buffer_new error!\n");
			return -1;
		}

		printf("read_len(%d)\n", read_len);
		memcpy(p_buf->data, key_string[0], read_len);
		p_buf->used = read_len;
		buffer_put(gateway_chunk, p_buf);

		if(read_len != CONST_READ_LEN)
		{
			printf("file over!read_len =(%d)\n", read_len);
		       	break;	
		}
	}
#endif
#ifdef EQUEL_READ
	while(!feof(fd) && (ftell(fd)!=EOF))
	{
		buffer_t *p_buf = NULL;
		int total_len = 0;

		if (fread(key_string[0], 1, GW_BUFFER_HEADER_LEN, fd) != GW_BUFFER_HEADER_LEN)//读总长度，
			break;

		int i;
		for(i=0; i<GW_BUFFER_HEADER_LEN; i++)
		{
			printf("(%c)[%x]\t", key_string[0][i], (int)key_string[0][i]);
		}
		printf("\n");
		total_len = get_two_byte_len((unsigned char *)&key_string[0][GW_BUFFER_LEN_POSITION]);
		printf("IP_POLICY:total_len (%x:%x)(%d)\t", key_string[0][GW_BUFFER_LEN_POSITION], key_string[0][GW_BUFFER_LEN_POSITION+1],total_len);

		memset(key_string[0], 0, MAX_LEN);
		if (fread(key_string[0], 1, 4, fd)!=4)
			break;
		printf("IP(%s)\t", key_string[0]);

	//fprintf(stderr, "================line(%d)-----fucntion(%s)------file(%s)\n", __LINE__, __FUNCTION__, __FILE__);
		fseek(fd, -(GW_BUFFER_HEADER_LEN+4), SEEK_CUR);
		memset(key_string[1], 0, MAX_LEN);
		if((int)fread(key_string[1], 1, total_len, fd)!= (int)(total_len))
			break;

		p_buf = buffer_new(total_len);
		if(p_buf==NULL)
		{
			printf("buffer_new error!\n");
			return -1;
		}
		
		memcpy(p_buf->data, key_string[1], total_len);
		//printf("DATA:(%s)\n", p_buf->data);
		p_buf->used = total_len;

		buffer_put(gateway_chunk, p_buf);
		//getchar();
	}
#endif
	fprintf(stderr, "================line(%d)-----fucntion(%s)------file(%s)\n\n\n", __LINE__, __FUNCTION__, __FILE__);
	if(fd)
		fclose(fd);

#if 0
	//测试最后几个头字节被读的情况
	buffer_t *p_buf = NULL;
	p_buf = buffer_new(2);
	if(p_buf==NULL)
	{
		printf("buffer_new error!\n");
		return -1;
	}
	memcpy(p_buf->data, "M1", 2);
	printf("DATA:(%s)\n", p_buf->data);
	p_buf->used = 2;
	buffer_put(gateway_chunk, p_buf);
	///
#endif	
	gateway_rbuffer_read(gateway_chunk);

	chunk_destroy(gateway_chunk);
	
	return 0;
}
#endif
