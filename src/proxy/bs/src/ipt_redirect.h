#ifndef _IPT_REDIRECT_H_
#define _IPT_REDIRECT_H_

#include <stddef.h>
#include <stdint.h>

int ipt_redirect_init();
void ipt_redirect_clear();

int ipt_redirect_add_url( char *url, size_t len );
int ipt_redirect_del_url( char *url, size_t len );

int ipt_redirect_add_port( uint16_t port );
int ipt_redirect_del_port( uint16_t port );

#endif
