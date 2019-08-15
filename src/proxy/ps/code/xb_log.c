#define _LOG_COMPILE

#include "xb_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#ifndef LOG_LINE_MAX_LEN
# define LOG_LINE_MAX_LEN 1024
#endif

#ifndef PACKET_DUMP_MAX_LEN
# define PACKET_DUMP_MAX_LEN 4096
#endif

enum log_level _log_level = LOG_LEVEL_INFO;

static const char *log_level_to_name[] = {
    [LOG_LEVEL_TRACE] = "TRACE",
    [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_INFO]  = "INFO",
    [LOG_LEVEL_WARN]  = "WARN",
    [LOG_LEVEL_ERROR] = "ERROR",
    [LOG_LEVEL_FATAL] = "FATAL",
};

static int log_level_to_sys[] = {
    [LOG_LEVEL_TRACE] = LOG_DEBUG,
    [LOG_LEVEL_DEBUG] = LOG_DEBUG,
    [LOG_LEVEL_INFO]  = LOG_INFO,
    [LOG_LEVEL_WARN]  = LOG_WARNING,
    [LOG_LEVEL_ERROR] = LOG_ERR,
    [LOG_LEVEL_FATAL] = LOG_CRIT,
};

static inline void log_line(int level, const char *line)
{
    syslog(level, "%s", line);
}

static void log_open(const char *ident, int tostderr, int hasthreadid)
{
    int option = LOG_PID;

    if (tostderr)
        option |= LOG_PERROR;

    ///@todo support hasthreadid

    openlog(ident, option, LOG_USER);
}

static void log_close()
{
    closelog();
}

static enum log_level name_to_log_level(const char *level_name)
{
    enum log_level l;

    switch (level_name[0]) {
        case 't': case 'T':
            l = LOG_LEVEL_TRACE;
            break;
        case 'd': case 'D':
            l = LOG_LEVEL_DEBUG;
            break;
        case 'i': case 'I':
            l = LOG_LEVEL_INFO;
            break;
        case 'w': case 'W':
            l = LOG_LEVEL_WARN;
            break;
        case 'e': case 'E':
            l = LOG_LEVEL_ERROR;
            break;
        case 'f': case 'F':
            l = LOG_LEVEL_FATAL;
            break;
        default:
            xb_log_warn("unknown log_level %s to convert", level_name);
            l = LOG_LEVEL_INFO;
            break;
    }
    return l;
}

static inline void log_level_set(enum log_level level)
{
    _log_level = level;
}

void xb_log_setup(const char *level)
{
    log_level_set(name_to_log_level(level));
}

void xb_log_open(const char *ident, int tostderr, int hasthreadid)
{
    log_open(ident, tostderr, hasthreadid);
}

void xb_log_close()
{
    log_close();
}


void _log_line(enum log_level log_level, const char *file, int line, const char *format, ...)
{
    char buf[LOG_LINE_MAX_LEN];
    va_list args;
    char *p;

    p = buf;
    p += snprintf(p, sizeof(buf), "[%s:%u] %s ", file, line, log_level_to_name[log_level]);

    va_start(args, format);
    vsnprintf(p, buf + sizeof(buf) - p, format, args);
    va_end(args);

    log_line(log_level_to_sys[log_level], buf);
}


#define CHAR_PRE_LINE 16
#define DUMP_LINE_LEN (1 + 8 + 2 + (1 + ((2 + 1) * CHAR_PRE_LINE) + 2 + CHAR_PRE_LINE + 1))

#define char_visiable(c) \
    (c > 32 && c < 128 ? c : '.')

void _log_packet(enum log_level level, const uint8_t *data, size_t len)
{
    char line[DUMP_LINE_LEN];
    char *p = line;
    size_t i, j;
    unsigned index;
    unsigned char c;

    if (!data || len==0 || len>PACKET_DUMP_MAX_LEN)
        return;

    index = 0;
    p += sprintf(p, " %08X: ", index);

    for (i = 0; i < len; i++) {
        if (i % CHAR_PRE_LINE == 0)
            p += sprintf(p, " ");

        p += sprintf(p, "%02x ", data[i]);

        if ((i + 1) % CHAR_PRE_LINE == 0) {
            p += sprintf(p, "  ");
            for (j = 0; j <= i % CHAR_PRE_LINE; j++) {
                if (i-15+j >= len)
                    break;

                c = char_visiable(data[i-15+j]);
                p += sprintf(p, "%c", c);
            }

            log_line(log_level_to_sys[level], line);
            index += CHAR_PRE_LINE;
            p = line;
            p += sprintf(p, " %08X: ", index);
        }
    }

    if (len % CHAR_PRE_LINE != 0) {
        for (j = i % CHAR_PRE_LINE; j < CHAR_PRE_LINE; j++) {
            p += sprintf(p, "   ");
        }

        p += sprintf(p, "  ");
        for (j = i & ~0xf; j < len; j++) {
            c = char_visiable(data[j]);
            p += sprintf(p, "%c", c);
        }

        log_line(log_level_to_sys[level], line);
    }
}
