#ifndef _IPTC_MGR_H_
#define _IPTC_MGR_H_

#include <stdint.h>

typedef struct iptc_mgr iptc_mgr_t;


iptc_mgr_t *iptc_mgr_init( char *eth, uint32_t dip, uint16_t dport );

int iptc_entry_add( iptc_mgr_t *mgr, uint16_t dport );

int iptc_entry_del( iptc_mgr_t *mgr, uint16_t dport );

void iptc_mgr_destroy( iptc_mgr_t *mgr );

#endif
