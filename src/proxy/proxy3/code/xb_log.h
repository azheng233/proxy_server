/**
 * @file xb_log.h
 * @brief 日志模块头文件
 * @author sxp@xdja.com
 * @version 0.0.1
 * @date 2014-04-03
 */
#ifndef _XB_LOG_H_
#define _XB_LOG_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#ifdef _LOG_COMPILE
#define _LOG_EXTERN __declspec(dllexport) extern
#else
#define _LOG_EXTERN __declspec(dllimport) extern
#endif
#else
#define _LOG_EXTERN extern
#endif

/**
 * @defgroup xb_log
 * @brief 日志模块
 *
 * 提供日志记录功能。
 * @todo 一期先使用 syslog 实现，后期重构为线程安全的。
 *
 * 使用方法：
 * - 程序启动时调用 xb_log_open() 打开日志
 * - 使用 xb_log_setup() 设置日志级别，比该级别低的日志将不会输出
 * - 在程序中使用 xb_log_info() xb_log_debug() 等记录日志，参数格式类似 printf
 * - 程序退出前调用 xb_log_close() 关闭日志
 *
 * @{
 */

/// 日志级别
enum log_level {
    LOG_LEVEL_TRACE,    ///< 跟踪，显示日志最详细 /**/
    LOG_LEVEL_DEBUG,    ///< 调试，跟踪关键路径 /**/
    LOG_LEVEL_INFO,     ///< 通知，默认级别，记录一般信息 /**/
    LOG_LEVEL_WARN,     ///< 警告，记录异常信息 /**/
    LOG_LEVEL_ERROR,    ///< 错误，记录操作失败信息 /**/
    LOG_LEVEL_FATAL,    ///< 致命，记录严重错误 /**/
};


/**
 * @brief 打开日志
 *
 * 只需初始化时打开一次
 *
 * @param[in] ident 程序标识
 * @param[in] tostderr 是否同时输出到标准错误
 * @param[in] hasthreadid 日志中是否包含线程标识
 */
_LOG_EXTERN void xb_log_open(const char *ident, int tostderr, int hasthreadid);

/**
 * @brief 关闭日志
 *
 * 程序退出前关闭日志
 */
_LOG_EXTERN void xb_log_close();

/**
 * @brief 设置日志级别
 *
 * 比设置的级别低的日志将不会输出
 *
 * @param[in] level 日志级别，形如 "INFO" 的字符串，不区分大小写
 */
_LOG_EXTERN void xb_log_setup(const char *level);

#define current_log_level_eq(level) \
    (_log_level == level)

#define current_log_level_le(level) \
    (_log_level <= level)

#define current_log_level_ge(level) \
    (_log_level >= level)

_LOG_EXTERN enum log_level _log_level;


_LOG_EXTERN void _log_line(enum log_level log_level, const char *file, int line, const char *format, ...);
_LOG_EXTERN void _log_packet(enum log_level log_level, const uint8_t *data, size_t len);

#define _log_line_level(level, fmt, ...) \
    (level<_log_level ?0: _log_line(level, __FILE__, __LINE__, fmt, ##__VA_ARGS__))

#define xb_log_trace(fmt, ...) _log_line_level(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define xb_log_debug(fmt, ...) _log_line_level(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define xb_log_info(fmt, ...)  _log_line_level(LOG_LEVEL_INFO,  fmt, ##__VA_ARGS__)
#define xb_log_warn(fmt, ...)  _log_line_level(LOG_LEVEL_WARN,  fmt, ##__VA_ARGS__)
#define xb_log_error(fmt, ...) _log_line_level(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define xb_log_fatal(fmt, ...) _log_line_level(LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)

#define _log_packet_level(level, data, len) \
    (level<_log_level ?0: _log_packet(level, data, len))

#define xb_trace_packet(data, len) _log_packet_level(LOG_LEVEL_TRACE, data, len)
#define xb_debug_packet(data, len) _log_packet_level(LOG_LEVEL_DEBUG, data, len)

///@}
#ifdef __cplusplus
}
#endif

/* vim:set fileencoding=utf-8 fileformat=unix syntax=c.doxygen */
#endif
