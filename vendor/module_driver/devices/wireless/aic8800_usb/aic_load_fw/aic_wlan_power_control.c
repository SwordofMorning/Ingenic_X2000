#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <utils/gpio.h>

static int wlan_power = -1;
static int wlan_reg_on = -1;

module_param_gpio_named(gpio_wlan_power, wlan_power, 0644);
module_param_gpio_named(gpio_wlan_reg_on, wlan_reg_on, 0644);

int aic_wlan_power_on(void)
{
    if (wlan_power >= 0) {
        if (gpio_request_one(wlan_power, GPIOF_DIR_OUT|GPIOF_INIT_LOW, "wlan_power") < 0) {
            char gpio[10];
            gpio_to_str(wlan_power, gpio);
            printk(KERN_ERR "gpio_wlan_reg_on:%s request failed!\n", gpio);
            return -EINVAL;
        }
    }

    if (wlan_reg_on >= 0) {
        if (gpio_request_one(wlan_reg_on, GPIOF_DIR_OUT|GPIOF_INIT_LOW, "wlan_reg_on") < 0) {
            char gpio[10];
            gpio_to_str(wlan_reg_on, gpio);
            printk(KERN_ERR "gpio_wlan_reg_on:%s request failed!\n", gpio);
            return -EINVAL;
        }
    }

    if (wlan_power >= 0) {
        gpio_set_value(wlan_power, 0);
        msleep(10);
        gpio_set_value(wlan_power, 1);
        msleep(10);
    }

    if (wlan_reg_on >= 0) {
        gpio_set_value(wlan_reg_on, 0);
        msleep(10);
        gpio_set_value(wlan_reg_on, 1);
        msleep(10);
    }

    return 0;
}


void aic_wlan_power_off(void)
{
    if (wlan_power >= 0) {
        gpio_set_value(wlan_power, 0);
        gpio_free(wlan_power);
    }

    if (wlan_reg_on >= 0) {
        gpio_set_value(wlan_reg_on, 0);
        gpio_free(wlan_reg_on);
    }
}

EXPORT_SYMBOL(aic_wlan_power_on);
EXPORT_SYMBOL(aic_wlan_power_off);
