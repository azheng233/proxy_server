#include "waitconn.h"
#include "connection.h"
#include "job.h"
#include "log.h"
#include "fdevents.h"


int waitqueue_init( waitqueue_t *wq )
{
    queue_init( &(wq->queue) );
    wq->num = 0;
    return 0;
}


void waitqueue_clear( waitqueue_t *wq )
{
    waitnode_t *node;
    queue_t *q = wq->queue.next;
    while( q != &(wq->queue) )
    {
        node = queue_data( q, waitnode_t, queue );
        q = q->next;
        waitconn_del( node->thisconn );
    }

    queue_init( &(wq->queue) );
    wq->num = 0;
}


int waitconn_add( connection_t *headconn, connection_t *conn, int events, int waitevents )
{
    if( NULL==headconn || NULL==conn )
        return WQ_ERR;

    waitnode_t *node = &(conn->wqnode);
    if( conn_isinwaitqueue( conn ) )
    {
        // 一个connection只能等待另外的一个connection
        if( headconn != node->headconn )
            return WQ_ERR;
        node->waitevents |= waitevents;
        node->events |= events;
        return WQ_OK;
    }

    node->headconn = headconn;
    node->waitevents = waitevents;
    node->thisconn = conn;
    node->events = events;

    queue_insert_tail( &(headconn->waitq.queue), &(node->queue) );
    headconn->waitq.num++;
    return WQ_OK;
}


int waitconn_del( connection_t *conn )
{
    if( NULL==conn )
        return WQ_ERR;

    waitnode_t *node = &(conn->wqnode);
    if( conn_isinwaitqueue( conn ) )
    {
        queue_remove( &(node->queue) );
        node->headconn->waitq.num--;
    }
    node->headconn = NULL;
    return WQ_OK;
}


int waitconn_wake( connection_t *headconn, int waitevents )
{
    if( NULL == headconn )
        return WQ_ERR;

    waitnode_t *node = NULL;
    queue_t *q = headconn->waitq.queue.next;
    while( q != &(headconn->waitq.queue) )
    {
        node = queue_data( q, waitnode_t, queue );
        q = q->next;
        if( waitevents & node->waitevents )
        {
            job_addconn( node->events, node->thisconn );
            ev_modfd( node->thisconn->fd, node->events | (node->thisconn->lsnevents & FDEVENT_LT), node->thisconn );
            //node->waitevents &= ~waitevents;
            //if( 0 == node->waitevents )
            waitconn_del( node->thisconn );
        }
    }

    if( NULL != node )
        log_debug( "connection %d wakeup %d", headconn->fd, node->thisconn->fd );
    return WQ_OK;
}


int waitnode_init( waitnode_t *wqnode )
{
    if( NULL == wqnode )
        return WQ_ERR;

    wqnode->headconn = NULL;
    wqnode->waitevents = 0;
    wqnode->thisconn = NULL;    // should be set outside
    wqnode->events = 0;
    queue_init( &(wqnode->queue) );
    return WQ_OK;
}
