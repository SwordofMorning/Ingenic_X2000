#ifndef _LIBUTILS2_BOOT_TIME_H_
#define _LIBUTILS2_BOOT_TIME_H_

#include <stdint.h>

/**
 * @brief 获取系统启动到现在的时间，单位微秒
 * @return 系统启动到现在的时间,单位微秒
 */
uint64_t boot_time_usecs(void);

/**
 * @brief 获取系统启动到现在的时间，适用于打印
 * @return 系统启动到现在的时间
 */
double boot_time_secs(void);

#endif /*  */