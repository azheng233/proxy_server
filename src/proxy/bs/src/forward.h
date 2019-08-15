#ifndef _FORWARD_H_
#define _FORWARD_H_

#include "connection.h"

#define HTTP_STAT_COMMONDREQ 2
#define HTTP_STAT_HEADREQ 3
#define HTTP_STAT_BODYREQ 4
#define HTTP_STAT_ERR 5
#define HTTP_STAT_HEADOVERFLOW 6



#define HTTP_HANDLE_OK 0
//too long request
#define HTTP_HANDLE_ERR_OVERFLOW 1
//url deny
#define HTTP_HANDLE_ERR_URL	2
//forward error
#define HTTP_HANDLE_ERR_FORWARD 3
//the req format err
#define HTTP_HANDLE_ERR_FORMAT 4

int connection_req_init( connection_t *con );
void connection_req_free( connection_t *con );
//int http_handle_forwarddata(connection_t *con);
int http_handle_parseandforwarddata( connection_t *con );

char * get_prev_url( connection_t *c );
char * get_req_url( connection_t *c, size_t *len );

#endif

