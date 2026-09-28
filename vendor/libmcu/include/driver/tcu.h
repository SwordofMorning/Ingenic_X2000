#ifndef _DRIVER_TCU_H_
#define _DRIVER_TCU_H_
#include <soc/tcu.h>

int tcu_init(void);
void tcu_deinit(void);

/* 关于各模式的配置函数，请查看<soc/tcu.h> */
int tcu_enable(struct tcu_config *config);
int tcu_disable(struct tcu_config *config);

/* 当配置为计数模式时，得到计数值 */
int tcu_get_count(struct tcu_config *config);

/* 当配置捕获模式时，得到捕获到的周期和高电平时间 */
int tcu_get_capture(struct tcu_config *config, int *high_level_time, int *period_time);
int tcu_get_capture_noirq(struct tcu_config *config, int *high_level_time, int *period_time);

#endif /* _DRIVER_TCU_H_ */