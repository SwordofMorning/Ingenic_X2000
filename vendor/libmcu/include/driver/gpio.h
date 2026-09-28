#ifndef _DRIVER_GPIO_H_
#define _DRIVER_GPIO_H_

#include <soc/gpio.h>

void gpio_init(void);

void gpio_direction_output(int gpio, int value);

void gpio_direction_input(int gpio);

void gpio_set_value(int gpio, int value);

int gpio_get_value(int gpio);

int gpio_set_func(int gpio, enum gpio_function func);

void gpio_set_schmitt(int gpio, int enable);

#endif /* _DRIVER_GPIO_H_ */
