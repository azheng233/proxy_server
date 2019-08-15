#ifndef _DNSCACHE_H_
#define _DNSCACHE_H_
#include <time.h>
#include "queue.h"
//DNS CHACH中的每个名称结点
typedef struct dnsname_node_s
{
	queue_t queue;
	queue_t dnsname_queue;
	char *dnsname;
	int dnsnamelen;
	unsigned char ip[4];
	time_t creattime;
	void *dnstree_node;
}dnsname_node_t;

int dnscache_init();
void dnscache_free();
void dnscache_empty();
dnsname_node_t *dnscache_find( char *f_dnsname, int f_dnslen );
int dnscache_insert( char *f_dnsname, int f_dnslen, unsigned char *f_ip );
int dnscache_del( dnsname_node_t *dnn );
void dnscache_timeout( int tout );
#endif
