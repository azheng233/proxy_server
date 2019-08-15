#include "monitor_log.h"
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

static char device_id[MLOG_DEVID_LEN] = {0};
#define LOG_STR_BUF_LEN 1024

void mlog_set_devid( char *devid )
{
    strncpy( device_id, devid, MLOG_DEVID_LEN );
    device_id[MLOG_DEVID_LEN-1] = 0;
}

void mlog_service_start( char *name )
{
    syslog( LOG_INFO, "%s|15|%s service start", device_id, name );
}

void mlog_service_stop( char *name )
{
    syslog( LOG_INFO, "%s|16|%s service stop", device_id, name );
}

void mlog_log_url( char *sn, char *url, enum url_action action )
{
    // syslog( LOG_INFO, "%s|26|%s|%s|%d", device_id, sn, url, action );
}
// 级联监控使用26类型，记录url和流量
void mlog_user_access( char *sn, char *url, enum url_action action, size_t upload, size_t download )
{
    syslog( LOG_INFO, "%s|26|%s|%s|%d|%u|%u", device_id, sn, url, action, (unsigned int)upload, (unsigned int)download );
}

void mlog_log_appstat( char *url, int state )
{
    int l = LOG_INFO;
    if( ! state )
        l |= LOG_WARNING;
    syslog( l, "%s|28||%s|%d", device_id, url, state );
}

void mlog_usrdef( unsigned mark, char *format, ... )
{
    static char str[LOG_STR_BUF_LEN];
    va_list ap;

    va_start( ap, format );
    vsnprintf( str, LOG_STR_BUF_LEN, format, ap );
    va_end( ap );

    syslog( LOG_INFO, "%s|%u|%s", device_id, mark, str );
}

