#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdint.h>

#define MSG_OK   0
#define MSG_ERR -1
#define MSG_ALT -2

#define MSG_HEAD_SIZE  7

#define MSG_PROTOCOL_VERSION_MAJOR  0x01
#define MSG_PROTOCOL_VERSION_MINOR3 0x03
#define MSG_PROTOCOL_VERSION_MINOR4 0x04

enum content_type {
    MSG_REQUEST = 14,
    MSG_AUDITLOG = 16,
    MSG_CONTROL = 21
};

enum request_type {
    REQ_POLICY_ALL = 10,
    REQ_POLICY_ONE = 11,
    REQ_POLICY_RESPONSE = 12
};

enum auditlog_type{
    AUDIT_LOG_URL = 10,
    AUDIT_LOG_FLOW_ONE = 11,
    AUDIT_LOG_FLOW_ALL = 12
};

enum control_type{
    CTRL_REQUEST = 1
};

#define MSG_CHECK_PROTOCOL_VERSION( ver ) \
    ( MSG_PROTOCOL_VERSION_MAJOR==ver[0] && (MSG_PROTOCOL_VERSION_MINOR3==ver[1] || MSG_PROTOCOL_VERSION_MINOR4==ver[1]))

void msg_add_head( uint8_t *head, enum content_type type, uint32_t dlen );

#endif
