#include "iptc_mgr.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IPT_TAB "iptables -t nat"
#define IPT_ADD " -I PREROUTING 1"
#define IPT_DEL " -D PREROUTING"
#define IPT_CMD " -p tcp -i %s ! -d %hhu.%hhu.%hhu.%hhu --dport %hu -j REDIRECT --to-port %hu"

#define ipt_gen_cmd( mgr, opt, port ) \
    sprintf( (mgr)->cmd, IPT_TAB IPT_##opt IPT_CMD, (mgr)->interface,   \
            (mgr)->dip[0], (mgr)->dip[1], (mgr)->dip[2], (mgr)->dip[3], \
            port, (mgr)->dport )

#define ipt_cmd_add( mgr, port )  ipt_gen_cmd( mgr, ADD, port )
#define ipt_cmd_del( mgr, port )  ipt_gen_cmd( mgr, DEL, port )


#define INTERFACE_LEN_MAX  16

struct iptc_mgr {
    char interface[INTERFACE_LEN_MAX];
    uint8_t dip[4];
    uint16_t dport;
    char cmd[sizeof(IPT_TAB)+sizeof(IPT_ADD)+sizeof(IPT_DEL)+sizeof(IPT_CMD)];
};

iptc_mgr_t *iptc_mgr_init( char *eth, uint32_t dip, uint16_t dport )
{
    iptc_mgr_t *m;

    m = malloc( sizeof(iptc_mgr_t) );
    if( NULL==m )
        return NULL;

    strncpy( m->interface, eth, INTERFACE_LEN_MAX );
    memcpy( m->dip, &dip, 4 );
    m->dport = dport;
    m->cmd[0] = 0;
    return m;
}

void iptc_mgr_destroy( iptc_mgr_t *mgr )
{
    if( mgr )
        free( mgr );
}


#define iptc_entry_opt( mgr, dport, opt ) \
    int ret;                                                        \
                                                                    \
    if( NULL==(mgr) || 0==(dport) )                                 \
        return -1;                                                  \
                                                                    \
    ipt_cmd_##opt( (mgr), (dport) );                                \
                                                                    \
    ret = system( (mgr)->cmd );                                     \
    if( ret )                                                       \
    {                                                               \
        log_error( #opt " cmd failed %d, %s", ret, (mgr)->cmd );    \
        return -1;                                                  \
    }                                                               \
    return 0;

int iptc_entry_add( iptc_mgr_t *mgr, uint16_t dport )
{
    iptc_entry_opt( mgr, dport, add )
}

int iptc_entry_del( iptc_mgr_t *mgr, uint16_t dport )
{
    iptc_entry_opt( mgr, dport, del )
}
