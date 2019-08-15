#ifndef _CHUNK_IO_H_
#define _CHUNK_IO_H_

#include "connection.h"

enum ioret {
    IOOK = 0,
    IOERR_AGN,
    IOERR_MOR,
    IOERR_RW,
    IOERR_FULL,
    IOERR_EMPTY,
    IOERR_MEM,
    IOERR_CLOSE,
    IOERR_OTHER
};

#define CONN_WRITE_CNTMAX  1024
#define SSL_READ_BUFFER_MIN_SIZE  2048

typedef int (*fun_handle_parse)( connection_t * );

int conn_handle_in( connection_t *conn, fun_handle_parse parse_handle );

int conn_handle_out( connection_t *conn );

#endif
