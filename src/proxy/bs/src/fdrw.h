#ifndef _FDRW_H_
#define _FDRW_H_

#include "connection.h"

#define FDRW_OK		0
#define FDRW_CLS	1
#define FDRW_NET	2
#define FDRW_AGN	3
#define FDRW_MEM	4
#define FDRW_PCK	5
#define FDRW_OTH	9
#define FDRW_PUS	99
//还有数据
#define FDRW_MOR	10
//没有更多的数据了
#define FDRW_NOM	11
//没有数据要写
#define FDRW_NOD	12

int readfd_common( connection_t *conn );
int writefd_handle_common( connection_t *conn );

int rwbuf_init( connection_t *conn, size_t rsize );
void rwbuf_free( connection_t *conn );

#endif

