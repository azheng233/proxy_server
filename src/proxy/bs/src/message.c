#include "message.h"
#include "readconf.h"
#include <arpa/inet.h>

void msg_add_head( uint8_t *head, enum content_type type, uint32_t dlen )
{
    uint8_t *p = head;
    uint8_t *pv;

    *p++ = type;
    pv = p;

    pv[0] = MSG_PROTOCOL_VERSION_MAJOR;
    pv[1] = MSG_PROTOCOL_VERSION_MINOR3;

    if (type == MSG_REQUEST) {
        if (conf[POLICY_VERSION].value.num == 14) {
            pv[0] = MSG_PROTOCOL_VERSION_MAJOR;
            pv[1] = MSG_PROTOCOL_VERSION_MINOR4;
        }
    }

    p += 2;
    *(uint32_t*)p = htonl( dlen );
}
