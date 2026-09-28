/*
 * Copyright (C) 2021 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Camera gpio
 *
 */

#ifndef __X2600_CAMERA_GPIO_H__
#define __X2600_CAMERA_GPIO_H__

int camera_mclk_gpio_init(int gpio);
int camera_mclk_gpio_deinit(int gpio);

#endif /* __X2600_CAMERA_GPIO_H__ */
