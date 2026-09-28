/*
 * Copyright (C) 2021 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for Sensor
 *
 */

#ifndef __X2600_CAMERA_SENSOR_H__
#define __X2600_CAMERA_SENSOR_H__

#include <linux/list.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include "camera.h"

enum sensor_reg_ops {
    SENSOR_REG_OP_DATA = 1,
    SENSOR_REG_OP_DELAY,
    SENSOR_REG_OP_END,
};

struct sensor_reg_op {
    unsigned int flag;
    unsigned int reg;
    unsigned int val;
};

typedef enum {
    POLARITY_HIGH_ACTIVE,
    POLARITY_LOW_ACTIVE,
} dvp_sync_polarity;

typedef enum {
    POLARITY_SAMPLE_RISING,
    POLARITY_SAMPLE_FALLING,
} dvp_sample_polarity;

typedef enum {
    DVP_IMG_SCAN_PROGRESS,
    DVP_IMG_SCAN_INTERLACE,
} dvp_img_scan_mode;

struct dvp_bus {
    dvp_sample_polarity pclk_polarity;
    dvp_sync_polarity hsync_polarity;
    dvp_sync_polarity vsync_polarity;
    dvp_img_scan_mode img_scan_mode;
};

struct sensor_info {
    void *private_init_setting;

    /* The following attributes are determined by private_init_setting */
    int width;
    int height;
    sensor_pixel_fmt fmt;

    unsigned int fps;     /* fps = Numerator / denominator;  [31:16]:Numerator [15:0]:denominator */
};

struct sensor_dbg_register {
    unsigned long long reg;
    unsigned long long val;
    unsigned int size;      /* val size, unit:byte */
};

struct sensor_ctrl_ops {
    /* base */
    int (*power_on)(void);
    void (*power_off)(void);
    int (*stream_on)(void);
    void (*stream_off)(void);

    /* debug */
    int (*get_register)(struct sensor_dbg_register *reg);
    int (*set_register)(struct sensor_dbg_register *reg);
};

struct sensor_attr {
    char *device_name;
    unsigned int cbus_addr;
    struct camera_info info;
    struct dvp_bus dvp;
    struct sensor_info sensor_info;
    struct sensor_ctrl_ops ops;
};

enum frame_data_status {
    frame_status_free,
    frame_status_trans,
    frame_status_usable,
    frame_status_user,
};

struct frame_data {
    struct list_head link;
    void *addr;
    unsigned int status;
    struct frame_info info;
};

struct camera_device {
    struct sensor_attr *sensor;
    unsigned int is_power_on;
    unsigned int is_stream_on;
};

static inline void m_msleep(int ms)
{
    usleep_range(ms*1000, ms*1000);
}

static inline unsigned long long get_time_us(void)
{
    struct timespec64 ts;
    unsigned long long timestamp;

    ktime_get_ts64(&ts);
    timestamp = (unsigned long long)ts.tv_sec * USEC_PER_SEC + ts.tv_nsec / NSEC_PER_USEC;

    return timestamp;
}

int camera_register_sensor(struct sensor_attr *sensor);
void camera_unregister_sensor(struct sensor_attr *sensor);

void camera_enable_sensor_mclk(unsigned long clk_rate);
void camera_disable_sensor_mclk(void);

int dvp_init_select_gpio(void);
void dvp_deinit_gpio(void);

#endif /* __X2600_CAMERA_SENSOR_H__ */
