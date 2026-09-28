/******************************************************************************
 *
 * Copyright(c) 2013 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

#include <utils/gpio.h>

unsigned int power_on = -1;
module_param_gpio(power_on, 0644);

int platform_wifi_power_on(void)
{
	char gpio_str[10];
	int ret;

    if (power_on != -1) {
        ret = gpio_request(power_on, "rtl8188fu_power");
        if (ret) {
            printk(KERN_ERR "rtl8188fu: failed to request power pin: %s\n", gpio_to_str(power_on, gpio_str));
			return -1;
        }
    }
    gpio_direction_output(power_on, 0);
	return 0;
}

void platform_wifi_power_off(void)
{

	if (power_on != -1) {
		gpio_direction_output(power_on, 1);
        gpio_free(power_on);
	}
}
