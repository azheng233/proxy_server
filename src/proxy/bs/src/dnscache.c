#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "dnscache.h"
#include "rbtree.h"
#include "timehandle.h"

//time_t curtime=0;

static uint32_t crc32_table16[] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

static inline uint32_t crc32_short(unsigned char *p, int len)
{
    unsigned char    c;
    uint32_t  crc;

    crc = 0xffffffff;

    while (len--) {
        c = *p++;
        crc = crc32_table16[(crc ^ (c & 0xf)) & 0xf] ^ (crc >> 4);
        crc = crc32_table16[(crc ^ (c >> 4)) & 0xf] ^ (crc >> 4);
    }

    return crc ^ 0xffffffff;
}

//DNS CACHE的头
typedef struct dnstree_head_s
{
	int dnstree_node_sum;//the sum of node of hash
	int dnsname_sum;//the sum of node of name
	queue_t queue;//seq queue for timeout
	rbtree_t tree;
}dnstree_head_t;

//DNS CHACHE中的每个HASH结点
typedef struct dnstree_node_s
{
	rbtree_node_t rbnode;
	int dnsname_sum;
	queue_t dnsname_queue;
}dnstree_node_t;

//DNS CHACH的头
static dnstree_head_t dnstree_head;
//flag
static rbtree_node_t sen;

int dnscache_init()
{
	dnstree_head.dnstree_node_sum = 0;
	dnstree_head.dnsname_sum = 0;
	rbtree_sentinel_init( &sen );
	queue_init( &(dnstree_head.queue) );
	rbtree_init( &(dnstree_head.tree), &sen );
	return 0;
}

void dnscache_free()
{
	dnscache_empty();
}


dnsname_node_t *dnsname_node_new( dnstree_node_t *tnode )
{
	dnsname_node_t *p;
	p = malloc(sizeof(dnsname_node_t));
	p->creattime = curtime;//@todo curtime
	p->dnsname = NULL;
	p->dnstree_node = tnode;
	tnode->dnsname_sum++;
	dnstree_head.dnsname_sum++;
	queue_insert_tail(&(dnstree_head.queue),&(p->queue));
	queue_insert_tail(&(tnode->dnsname_queue),&(p->dnsname_queue));
	return p;
}

void dnsname_node_free(dnsname_node_t *p)
{
	if( p==NULL ) return;
	if( p->dnsname!=NULL ) free(p->dnsname);
	queue_remove( &(p->queue) );
	dnstree_head.dnsname_sum--;
	if( p->dnstree_node )
	{
		queue_remove( &(p->dnsname_queue) );
		((dnstree_node_t *)(p->dnstree_node))->dnsname_sum--;
	}
	free( p );
}

dnstree_node_t *dnstree_node_new(uint32_t hash )
{
	dnstree_node_t *dn;
	dn = malloc( sizeof(dnstree_node_t) );
	if( !dn ) return NULL;
	dn->dnsname_sum = 0;
	(dn->rbnode).key = hash;
	queue_init( &(dn->dnsname_queue) );
	rbtree_insert( &(dnstree_head.tree), &(dn->rbnode) );
	dnstree_head.dnstree_node_sum++;
	return dn;
}

int dnstree_node_free( dnstree_node_t *dn )
{
	if( dn->dnsname_sum>0 ) return -1;
	rbtree_delete( &(dnstree_head.tree), &(dn->rbnode) );
	dnstree_head.dnstree_node_sum--;
	free( dn );
	return 0;
}

dnstree_node_t *dnstree_node_find( int hash )
{
	rbtree_node_t *node = rbtree_find( &(dnstree_head.tree), hash );
	if( node )
        return queue_data( node, dnstree_node_t, rbnode );
    else
        return NULL;
}

dnsname_node_t *dnsname_node_find( char *f_dnsname, int f_dnslen, dnstree_node_t *tnode )
{
	dnsname_node_t *d;
	queue_t *t;
	if( f_dnslen<=0 ) return NULL;

	t = (tnode->dnsname_queue).next;
	while( t!= (&(tnode->dnsname_queue)))
	{
		d = queue_data( t, dnsname_node_t, dnsname_queue );
		if( f_dnslen==d->dnsnamelen )
		{
			if( strncmp( f_dnsname, d->dnsname, d->dnsnamelen )==0 ) return d;
		}
		t = t->next;
	}

    return NULL;
}



dnsname_node_t *dnscache_find( char *f_dnsname, int f_dnslen )
{
	//find hash
	uint32_t hash;
	dnstree_node_t *dtn;
	dnsname_node_t *dnn;
	hash = crc32_short( (unsigned char *)f_dnsname, f_dnslen );
	if( !(dtn=dnstree_node_find(hash)) ) return NULL;
	//find name
	if( !(dnn=dnsname_node_find(f_dnsname,f_dnslen,dtn)) ) return NULL;
	return dnn;
}

int dnscache_insert( char *f_dnsname, int f_dnslen, unsigned char *f_ip )
{
	uint32_t hash;
	dnstree_node_t *dtn;
	dnsname_node_t *dnn;
	int len;
	//find tree node, if no then new
	hash = crc32_short( (unsigned char *)f_dnsname, f_dnslen );
	if( !(dtn=dnstree_node_find(hash)) )
		if( !(dtn=dnstree_node_new(hash)) ) return -1;
	
	//insert name node
	if( (dnn=dnsname_node_find(f_dnsname,f_dnslen,dtn)) ) dnsname_node_free( dnn );
	if( !(dnn=dnsname_node_new(dtn)) ) return -1;
	if( (len=(f_dnslen+1)%64) ) len+=64-(f_dnslen+1);
	if( !(dnn->dnsname=malloc(len)) ) return -1;
	memcpy( dnn->dnsname, f_dnsname, f_dnslen );
	dnn->dnsname[f_dnslen] = '\0';
	dnn->dnsnamelen = f_dnslen;
	dnn->ip[0]=f_ip[0];dnn->ip[1]=f_ip[1];dnn->ip[2]=f_ip[2];dnn->ip[3]=f_ip[3];
	return 0;
}

int dnscache_del( dnsname_node_t *dnn )
{
	dnstree_node_t *dtn;
	dtn = dnn->dnstree_node;
	dnsname_node_free( dnn );
	if( dtn )
	{
		if( dtn->dnsname_sum==0 ) dnstree_node_free( dtn );
	}
	return 0;
}

void dnscache_timeout( int tout )
{
	dnsname_node_t *d;
	queue_t *t;

	t = (dnstree_head.queue).prev;
	while( t!= (&(dnstree_head.queue)) )
	{
		d = queue_data( t, dnsname_node_t, queue );
		t = t->prev;
		if( (curtime-d->creattime)>tout ) dnscache_del( d );
		else break;
	}
}

void dnscache_empty()
{
	dnscache_timeout(-1);
}
//
//int main()
//{
//	//init
//	int ret;
//	dnsname_node_t *dnt;
//	int i;
//	unsigned char ip[4];
//	while(1)
//	{
//	ret = dnscache_init();
//	dnscache_insert( "www.sina.com", 13, ip );
//	dnscache_insert( "www.sina.con", 13, ip );
//	dnscache_insert( "www.sina.cod", 13, ip );
//	dnscache_insert( "www.sina.co1", 13, ip );
//	for(i=0;i<1024;i++);
//		dnt = dnscache_find( "www.sina.co12", 14 );
//	if( dnt )
//	printf("dnstree_head.dnsname_sum:%d,find=%s\n",dnstree_head.dnsname_sum,dnt->dnsname);
//	dnscache_empty();
//	}
//
//
//	//int dnscache_init();
////void dnscache_free();
////void dnscache_empty();
////dnsname_node_t *dnscache_find( char *f_dnsname, int f_dnslen );
////int dnscache_insert( char *f_dnsname, int f_dnslen, unsigned char *f_ip );
////int dnscache_del( dnsname_node_t *dnn );
////void dnscache_timeout( int tout );
//}
