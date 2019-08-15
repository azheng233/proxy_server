/** 
 * @file waitconn.h
 * @brief connection之间等待事件
 * @author sunxp
 * @version 01
 * @date 2010-07-21
 */
#ifndef _WAIT_CONN_H_
#define _WAIT_CONN_H_

#include "queue.h"

#define WQ_OK   0
#define WQ_ERR  -1

#define WAITEVENT_CANWRITE (1<<0)
#define WAITEVENT_CANREAD  (1<<1)

typedef struct waitqueue_st {
    unsigned int num;
    queue_t queue;
} waitqueue_t;

typedef struct waitnode_st {
    struct connection *headconn;
    int waitevents;
    struct connection *thisconn;
    int events;
    queue_t queue;
} waitnode_t;


int waitqueue_init( waitqueue_t *wq );

void waitqueue_clear( waitqueue_t *wq );


int waitconn_add( struct connection *headconn, struct connection *conn, int events, int waitevents );

int waitconn_del( struct connection *conn );

int waitconn_wake( struct connection *headconn, int waitevents );


int waitnode_init( waitnode_t *wqnode );

#define conn_isinwaitqueue(conn) \
    ( NULL != (conn)->wqnode.headconn )

#endif
