#ifndef _LICENSE_H_
#define _LICENSE_H_

#include <time.h>
#define DEVID_LEN_MAX 256

int license_verify( char *license_file );

/** 
 * @brief 验证有效期
 * 
 * @param[in] nowtime 当前时间
 * 
 * @retval 0 未过期
 * @retval <0 小于起始日期
 * @retval >0 大于终止日期
 */
int license_verify_date( time_t nowtime );

char *license_get_devid();

#endif
