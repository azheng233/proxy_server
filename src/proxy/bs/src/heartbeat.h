#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_

void hb_init( const char *name, const char *path );

int hb_write_pid();

int hb_heartbeat();

#endif
