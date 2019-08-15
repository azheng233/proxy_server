/**
 * @file log.h
 * @brief 打印日志和调试信息
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#ifndef _LOG_H_
#define _LOG_H_

#include "readconf.h"

enum log_level {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
};

enum log_mode {
    LOG_MODE_STDERR,
    LOG_MODE_SYSLOG
};

/// 设置日志模式，守护进程时使用syslog
void log_set_mode(enum log_mode mode);

void log_print(enum log_level level, const char *file, const int line, const char *format, ...);

#define log_with_level( level, format, ... ) \
    (level)<conf[LOG_LEVEL].value.num ?0: log_print( (level), __FILE__, __LINE__, format, ##__VA_ARGS__ )

#define log_trace( format, ... ) \
    log_with_level( LOG_LEVEL_TRACE, format, ##__VA_ARGS__ )

#define log_debug( format, ... ) \
    log_with_level( LOG_LEVEL_DEBUG, format, ##__VA_ARGS__ )

#define log_info( format, ... ) \
    log_with_level( LOG_LEVEL_INFO, format, ##__VA_ARGS__ )

#define log_warn( format, ... ) \
    log_with_level( LOG_LEVEL_WARN, format, ##__VA_ARGS__ )

#define log_error( format, ... ) \
    log_with_level( LOG_LEVEL_ERROR, format, ##__VA_ARGS__ )

#endif
