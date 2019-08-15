 /** 
 * @file timehandle.h
 * @brief 时间和定时器声明
 * @author sunxp
 * @version 0.1
 * @date 2010-08-03
 */
#ifndef _TIMEHANDLE_H_
#define _TIMEHANDLE_H_

#include <time.h>

/**
 * @defgroup timer
 * @brief 时间和定时处理
 *
 * 设置定时器，定时更新当前时间，判断超时并处理超时事件
 */
///@{

/// 定时器超时间隔
#define TIMER_INTERVAL  1

/// 当前事件，每秒更新
extern time_t curtime;

int time_init();

int time_handle();

//thread unsafe
const char *time_gmt();

///@}
#endif
