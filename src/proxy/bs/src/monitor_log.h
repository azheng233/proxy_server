#ifndef _MONITOR_LOG_H_
#define _MONITOR_LOG_H_

#define MLOG_DEVID_LEN 256

#include "msg_audit.h"

void mlog_set_devid( char *devid );

void mlog_service_start();
void mlog_service_stop();

void mlog_log_url( char *sn, char *url, enum url_action action );
#define mlog_url_pass( sn, url )  mlog_log_url( (sn), (url), URL_PASS )
#define mlog_url_deny( sn, url )  mlog_log_url( (sn), (url), URL_DENY )

void mlog_user_access( char *sn, char *url, enum url_action action, size_t upload, size_t download );
#define mlog_user_access_done( sn, url, up, down )  mlog_user_access( sn, url, URL_PASS, up, down )
#define mlog_user_access_deny( sn, url, up, down )  mlog_user_access( sn, url, URL_DENY, up, down )


#define APP_STAT_DOWN  0
#define APP_STAT_LIVE  1
void mlog_log_appstat( char *url, int state );


void mlog_usrdef( unsigned mark, char *format, ... );
#define mlog_info( fmt, ... )   mlog_usrdef( 90, fmt, ##__VA_ARGS__ )
#define mlog_error( fmt, ... )  mlog_usrdef( 91, fmt, ##__VA_ARGS__ )
#define mlog_alert( fmt, ... )  mlog_usrdef( 92, fmt, ##__VA_ARGS__ )

#endif
