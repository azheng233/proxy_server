#ifndef __HEADER_H__
#define __HEADER_H__

typedef enum  buffer_state_e
{
	ANALYSIZE_BUFFER_HEADER=100,
	READ_BUFFER_DATA,
	REBUILD_BUFFER_SUCCESS,
	NO_ENOUGH_BUFFER,

}buffer_state_t;

typedef struct buffer_rebuild_observ_s
{       
	int next_buffer_analysized;//下一个新的buffer是否已经分析完毕
	int next_buffer_offset;//下一个新的buffer开头位置在data中的偏移
	int needed_header_len;//剩余策略头部长度，小于等于GW_BUFFER_HEADER_LEN
	int needed_data_len;//剩余策略信息长度
	unsigned char *p_data_spy;//从此处继续填入剩余的data数据；
	unsigned char three_header_char[4];

}buffer_rebuild_observ_t;

int gateway_rbuffer_read(struct connection *gateway_connect);

#endif
