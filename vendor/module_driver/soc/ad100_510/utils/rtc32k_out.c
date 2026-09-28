#include <linux/module.h>
#include <asm/atomic.h>
#include <linux/gpio.h>
#include <utils/gpio.h>
#include <linux/spinlock.h>
#include <linux/moduleparam.h>


static int rtc32k_init_on = 0;

module_param(rtc32k_init_on, int, 0644);

#define rtc_gpio GPIO_PE(00)
#define rtc_gpio_func GPIO_FUNC_0

void ingenic_rtc32k_enable(void)
{

}

void ingenic_rtc32k_disable(void)
{

}

void rtc32k_init(void)
{

}

void rtc32k_exit(void)
{

}

EXPORT_SYMBOL(rtc32k_exit);
EXPORT_SYMBOL(ingenic_rtc32k_enable);
EXPORT_SYMBOL(ingenic_rtc32k_disable);
