#include "get_port_from_url.h"
#include "log.h"
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>

uint32_t get_interface_ip( const char *name )
{
    int sock;
    struct ifreq ifr;
    int ret;

    sock = socket( AF_INET, SOCK_DGRAM, 0 );
    if( sock < 0 )
        return -1;

    strncpy( ifr.ifr_name, name, IFNAMSIZ );

    ret = ioctl( sock, SIOCGIFADDR, &ifr );
    if( ret < 0 )
    {
        log_error( "ioctl sock %d SIOCGIFADDR failed, %s", sock, strerror(errno) );
        goto errfd;
    }

    close( sock );
    return ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr;

errfd:
    close( sock );
    return -1;
}
