/** 
 * @file options.h
 * @brief 命令行选项处理
 * @author sunxp
 * @version 0.2
 * @date 2010-08-03
 */
#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include "build.h"

/**
 * @defgroup option
 * @brief 命令行选项处理
 */
///@{

/// 程序版本
#define VERSION SERVER_VERSION
/// 默认配置文件
#define CONFIG_FILE_DEF  "./url.conf"

/// 配置文件
extern char *config_file;
/// 是否精灵化
extern int daemonize;

void options_parse( int argc, char *argv[] );

void print_bar( const char *name );
void print_version( const char *name );

///@}
#endif
