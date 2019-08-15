#include "log.h"
#include "timehandle.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>

#define LOG_BUF_MAX_LEN  1024

static enum log_mode log_mode = LOG_MODE_STDERR;
void log_set_mode( enum log_mode mode )
{
    int option = LOG_PID;

    log_mode = mode;
    if (log_mode == LOG_MODE_STDERR)
        option |= LOG_PERROR;
    openlog("urlserver", option, LOG_LOCAL0);
}

/** 
 * @brief 生成日志
 * 
 * @param[in] level 日志级别
 * @param[in] file 源文件名
 * @param[in] line 行号
 * @param[in] format 格式字符串
 */
void log_print( enum log_level level, const char *file, const int line, const char *format, ... )
{
    char str[LOG_BUF_MAX_LEN];
    char *p;
    char *levelstr;
    const char *pfile;
    int n;
    va_list ap;

    struct tm tm;
    char timestr[32];

    if( LOG_MODE_STDERR == log_mode )
    {
        localtime_r( &curtime, &tm );
        strftime( timestr, 32, "%Y-%m-%d %H:%M:%S ", &tm );
    }
    else timestr[0] = 0;

    int sys_log_level = LOG_INFO;
    switch( level )
    {
        case LOG_LEVEL_TRACE:
            levelstr = "TRACE";
            sys_log_level |= LOG_DEBUG;
            break;
        case LOG_LEVEL_DEBUG:
            levelstr = "DEBUG";
            sys_log_level |= LOG_DEBUG;
            break;
        case LOG_LEVEL_INFO:
            levelstr = "INFO ";
            break;
        case LOG_LEVEL_WARN:
            levelstr = "WARN ";
            sys_log_level |= LOG_WARNING;
            break;
        case LOG_LEVEL_ERROR:
            levelstr = "ERROR";
            sys_log_level |= LOG_ERR;
            break;
        default:
            levelstr = "UNKNOWN";
            break;
    }

    pfile = strrchr(file, '\\');
    if (!pfile) {
        pfile = strrchr(file, '/');
    }
    if (pfile) {
        pfile++;
    } else {
        pfile = file;
    }

    p = str;
    n = sprintf( p, "%s%s", timestr, levelstr );
    p += n;
    if( conf[LOG_LEVEL].value.num <= LOG_LEVEL_DEBUG )
    {
        n = sprintf( p, " [%s:%u]", pfile, line );
        p += n;
    }
    *p++ = ' ';

    va_start( ap, format );
    n = vsnprintf( p, LOG_BUF_MAX_LEN-(p-str), format, ap );
    p += n;
    va_end( ap );

    str[LOG_BUF_MAX_LEN-1] = 0;
    if( LOG_MODE_SYSLOG == log_mode )
        syslog( sys_log_level, "%s", str );
    else
        fprintf( stderr, "%s\n", str );
}
