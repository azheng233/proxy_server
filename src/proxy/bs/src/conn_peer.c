#include "conn_peer.h"
#include "fdevents.h"
#include "job.h"
#include "chunk.h"
#include <stdlib.h>

connhead_t connhead;

int conn_signal_peerclose( connection_t *conn )
{
    set_conn_status( conn, CONSTAT_PEERCLOSED );
    conn->peer = NULL;

    int ret = job_addconn( FDEVENT_PEERCLOSE, conn );
    if( ret !=  JOB_OK )
        return CONN_ERR;
    return CONN_OK;
}


int conn_signal_thisclose( connection_t *conn )
{
    set_conn_status( conn, CONSTAT_CLOSED );

    connection_t *peerconn = conn->peer;
    if( NULL != peerconn )
        conn_signal_peerclose( peerconn );

    if( job_addconn( FDEVENT_CLOSE, conn ) != JOB_OK )
        return CONN_ERR;
    return CONN_OK;
}


int conn_handle_peerclose( connection_t *conn )
{
    if( ! chunk_isempty( conn->writebuf ) )
    {
        set_conn_status( conn, CONSTAT_CLOSING_WRITE );
        return CONN_AGN;
    }
    return CONN_OK;
}


int conn_handle_thisclose( connection_t *conn )
{
    job_delconn( conn );
    conn_del_queue( &connhead, conn );
    ev_delfd( conn->fd, conn );
    connection_del( conn );
    return CONN_OK;
}


void connections_destroy()
{
    conn_queue_destroy( &connhead );
}
