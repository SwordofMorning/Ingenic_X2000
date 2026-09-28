/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * SC030IOT
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera_sensor.h>

#define SC030IOT_DEVICE_NAME          "sc030iot-dvp"
#define SC030IOT_DEVICE_I2C_ADDR      0x68
#define SC030IOT_CHIP_ID_H            0x9a
#define SC030IOT_CHIP_ID_L            0x46
#define SC030IOT_SUPPORT_SCLK         24*1000*1000
#define ENDMARKER                     {0xff, 0xff}

static int power_gpio   = -1;       //PB12
static int reset_gpio   = -1;       //PA30
static int pwdn_gpio    = -1;       //
static int i2c_bus_num  = -1;       //i2c 0

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr sc030iot_sensor_attr;

struct regval_list {
    unsigned char reg_num;
    unsigned char value;
};

static struct regval_list sc030iot_init_regs_320_240_dvp_25fps[] = {
    {0xf0, 0x30},
    {0x01, 0xff},
    {0x02, 0xff},
    {0x22, 0x07},
    {0x19, 0xff},
    {0x3f, 0x82},
    {0x30, 0x02},
    {0xf0, 0x01},
    {0x70, 0x00},
    {0x71, 0x80},
    {0x72, 0x20},
    {0x73, 0x00},
    {0x74, 0xe0},
    {0x75, 0x10},
    {0x76, 0x81},
    {0x77, 0x8c},
    {0x78, 0xe1},
    {0x79, 0x31},
    {0xf5, 0x01},
    {0xf4, 0x0a},
    {0xf0, 0x36},
    {0x37, 0x79},
    {0xea, 0x09},
    {0x31, 0x82},
    {0x3e, 0x60},
    {0x30, 0xf0},
    {0x33, 0x33},
    {0xf0, 0x32},
    {0x48, 0x02},
    {0xf0, 0x33},
    {0x02, 0x12},
    {0x7c, 0x02},
    {0x7d, 0x0e},
    {0xa2, 0x04},
    {0x5e, 0x06},
    {0x5f, 0x0a},
    {0x0b, 0x58},
    {0x06, 0x38},
    {0xf0, 0x32},
    {0x48, 0x02},
    {0xf0, 0x39},
    {0x02, 0x70},
    {0xf0, 0x45},
    {0x09, 0x1c},
    {0xf0, 0x37},
    {0x22, 0x0d},
    {0xf0, 0x33},
    {0x33, 0x10},
    {0xb1, 0x80},
    {0x34, 0x40},
    {0x0b, 0x54},
    {0xb2, 0x78},
    {0xf0, 0x36},
    {0x11, 0x80},
    {0xf0, 0x30},
    {0x38, 0x44},
    {0xf0, 0x33},
    {0xb3, 0x51},
    {0x01, 0x10},
    {0x0b, 0x6c},
    {0x06, 0x24},
    {0xf0, 0x36},
    {0x31, 0x82},
    {0x3e, 0x60},
    {0x30, 0xf0},
    {0x33, 0x33},
    {0xf0, 0x34},
    {0x9f, 0x02},
    {0xa6, 0x40},
    {0xa7, 0x47},
    {0xe8, 0x5f},
    {0xa8, 0x51},
    {0xa9, 0x44},
    {0xe9, 0x36},
    {0xf0, 0x33},
    {0xb3, 0x51},
    {0x64, 0x17},
    {0x90, 0x01},
    {0x91, 0x03},
    {0x92, 0x07},
    {0x01, 0x10},
    {0x93, 0x10},
    {0x94, 0x10},
    {0x95, 0x10},
    {0x96, 0x01},
    {0x97, 0x07},
    {0x98, 0x1f},
    {0x99, 0x10},
    {0x9a, 0x20},
    {0x9b, 0x28},
    {0x9c, 0x28},
    {0xf0, 0x36},
    {0x70, 0x54},
    {0xb6, 0x40},
    {0xb7, 0x41},
    {0xb8, 0x43},
    {0xb9, 0x47},
    {0xba, 0x4f},
    {0xb0, 0x8b},
    {0xb1, 0x8b},
    {0xb2, 0x8b},
    {0xb3, 0x9b},
    {0xb4, 0xb8},
    {0xb5, 0xf0},
    {0x7e, 0x41},
    {0x7f, 0x47},
    {0x77, 0x80},
    {0x78, 0x84},
    {0x79, 0x8a},
    {0xa0, 0x47},
    {0xa1, 0x5f},
    {0x96, 0x43},
    {0x97, 0x44},
    {0x98, 0x54},
    {0xf0, 0x00},
    {0xf0, 0x01},
    {0x73, 0x00},
    {0x74, 0xe0},
    {0x70, 0x00},
    {0x71, 0x80},
    {0xf0, 0x36},
    {0x37, 0x74},
    {0xf0, 0x3f},
    {0x03, 0x91},
    {0xf0, 0x36},   //cvbs_off
    {0x11, 0x80},
    {0xf0, 0x01},
    {0x79, 0xc1},
    {0xf0, 0x37},
    {0x24, 0x21},
    {0xf0, 0x36},
    {0x41, 0x60},  //driver capability
    {0xf0, 0x32},
    {0x0e, 0x04},
    {0x0f, 0x18},
    {0xf0, 0x00},
    {0x72, 0x38},
    {0x7a, 0x80},
    {0x7c, 0x04},
    {0x7e, 0x25},
    {0x85, 0x18},
    {0x9b, 0x35},
    {0x9e, 0x20},
    {0xd0, 0x66},
    {0xd1, 0x34},
    {0Xd3, 0x44},
    {0xd6, 0x44},
    {0xb0, 0x41},
    {0xb2, 0x48},
    {0xb3, 0xf4},
    {0xb4, 0x0b},
    {0xb5, 0x78},
    {0xba, 0xff},
    {0xbb, 0xc0},
    {0xbc, 0x90},
    {0xbd, 0x3a},
    {0xc1, 0x67},
    {0xf0, 0x01},
    {0x20, 0x11},
    {0x23, 0x90},
    {0x24, 0x15},
    {0x25, 0x87},
    {0xbc, 0x9f},
    {0xbd, 0x3a},
    {0x48, 0xe6},
    {0x49, 0xc0},
    {0x4a, 0xd0},
    {0x4b, 0x48},
    {0xf0, 0x00},
    {0x71, 0x92},
    {0x7c, 0x03},
    {0x84, 0xb4},
    {0xf0, 0x33},
    {0x14, 0x95},

    {0xf0, 0x36},
    {0x40, 0x03},    //pclk delay

    // {0xf0, 0x01},
    // {0x00, 0x80},    //test mode
    ENDMARKER,  /* END MARKER */
};

static struct regval_list sc030iot_regs_stream_on[] = {
    {0xf0, 0x31},
    {0x00, 0x01},
    ENDMARKER,
};

static struct regval_list sc030iot_regs_stream_off[] = {
    {0xf0, 0x31},
    {0x00, 0x00},
    ENDMARKER,
};

static struct regval_list sc030iot_chip_id_regs[] = {
    {0xf0, 0x00},
    {0xf7, 0x00},
    {0xf8, 0x00},
    ENDMARKER,
};

static int sc030iot_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
{
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0) {
        printk(KERN_ERR "sc030iot: failed to write reg: %x\n", (int)reg);
    }

    return ret;
}

static int sc030iot_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    unsigned char buf[1] = {reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr  = i2c->addr,
            .flags = 0,
            .len   = 1,
            .buf   = buf,
        },
        [1] = {
            .addr  = i2c->addr,
            .flags = I2C_M_RD,
            .len   = 1,
            .buf   = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0) {
        printk(KERN_ERR "sc030iot(%x): failed to read reg: %x\n", i2c->addr, (int)reg);
    }

    return ret;
}

static int sc030iot_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != 0xff) {
        ret = sc030iot_write(i2c, vals->reg_num, vals->value);
        if (ret < 0) {
            return ret;
        }
        vals++;
    }

    return 0;
}

static inline int sc030iot_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != 0xff) {
        if (vals->reg_num == 0xf0) {
            ret = sc030iot_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        } else {
            ret = sc030iot_read(i2c, vals->reg_num, &vals->value);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "{0x%x, 0x%x}\n", vals->reg_num, vals->value);

        vals++;
    }

    return 0;
}

static int sc030iot_detect(struct i2c_client *i2c)
{
    int ret;

    ret = sc030iot_read_array(i2c, sc030iot_chip_id_regs);
    if (ret < 0)
        return ret;

    if (sc030iot_chip_id_regs[1].value != SC030IOT_CHIP_ID_H) {
        printk(KERN_ERR "sc030iot read chip id failed:0x%x\n", sc030iot_chip_id_regs[1].value);
        return -ENODEV;
    }
    if (sc030iot_chip_id_regs[2].value != SC030IOT_CHIP_ID_L) {
        printk(KERN_ERR "sc030iot read chip id failed:0x%x\n", sc030iot_chip_id_regs[2].value);
        return -ENODEV;
    }

    printk(KERN_ERR "sc030iot read chip id : h = 0x%x,l = 0x%x\n", sc030iot_chip_id_regs[1].value, sc030iot_chip_id_regs[2].value);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "sc030iot_reset");
        printk(KERN_ERR "sc030iot:  request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));

        if (ret) {
            printk(KERN_ERR "sc030iot: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "sc030iot_pwdn");
        printk(KERN_ERR "sc030iot:  request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
        if (ret) {
            printk(KERN_ERR "sc030iot: failed to request sc030iot_pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "sc030iot_power");
        printk(KERN_ERR "sc030iot:  request sc030iot_power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
        if (ret) {
            printk(KERN_ERR "sc030iot: failed to request sc030iot_power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    return 0;

err_power_gpio:
    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);
err_pwdn_gpio:
    if (reset_gpio != -1)
        gpio_free(reset_gpio);
err_reset_gpio:

    return ret;
}

static void deinit_gpio(void)
{
    if (power_gpio != -1)
        gpio_free(power_gpio);

    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    if (reset_gpio != -1)
        gpio_free(reset_gpio);

    dvp_deinit_gpio();
}

static void sc030iot_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

        camera_disable_sensor_mclk();
}

static int sc030iot_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(SC030IOT_SUPPORT_SCLK);
    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(50);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 0);
        m_msleep(50);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(50);
    }

    ret = sc030iot_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "sc030iot: failed to detect\n");
        sc030iot_power_off();
        return ret;
    }
    printk(KERN_ERR "sc030iot: sc030iot detect success\n");

    m_msleep(10);
    ret = sc030iot_write_array(i2c_dev, sc030iot_sensor_attr.sensor_info.private_init_setting);
    // ret = sc030iot_read_array(i2c_dev, sc030iot_sensor_attr.sensor_info.private_init_setting);
    m_msleep(10);
    if (0 != ret) {
        printk(KERN_ERR "sc030iot: failed to init regs\n");
        sc030iot_power_off();
        return ret;
    }

    return 0;
}

static int sc030iot_stream_on(void)
{
    int ret = sc030iot_write_array(i2c_dev, sc030iot_regs_stream_on);
    if (ret)
        printk(KERN_ERR "sc030iot: failed to stream on\n");

    return ret;
}

static void sc030iot_stream_off(void)
{
    int ret = sc030iot_write_array(i2c_dev, sc030iot_regs_stream_off);
    if (ret)
        printk(KERN_ERR "sc030iot: failed to stream on\n");
}

static int sc030iot_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = sc030iot_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int sc030iot_s_register(struct sensor_dbg_register *reg)
{
    return sc030iot_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static struct sensor_attr sc030iot_sensor_attr = {

    .device_name                = SC030IOT_DEVICE_NAME,
    .cbus_addr                  = SC030IOT_DEVICE_I2C_ADDR,

    .dma_mode                   = SENSOR_DATA_DMA_MODE_RAW,    // olny dvp.data_fmt is DVP_YUV422
    .dbus_type                  = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .pclk_polarity          = POLARITY_SAMPLE_RISING,
        .hsync_polarity         = POLARITY_HIGH_ACTIVE,
        .vsync_polarity         = POLARITY_HIGH_ACTIVE,
        .img_scan_mode          = DVP_IMG_SCAN_PROGRESS,
    },

    .sensor_info = {
        .private_init_setting   = sc030iot_init_regs_320_240_dvp_25fps,
        .width                  = 640,
        .height                 = 480,
        .fmt                    = SENSOR_PIXEL_FMT_YUYV8_2X8,
        .fps                    = 30 << 16 | 1,
    },

    .ops = {
        .power_on               = sc030iot_power_on,
        .power_off              = sc030iot_power_off,
        .stream_on              = sc030iot_stream_on,
        .stream_off             = sc030iot_stream_off,
        .get_register           = sc030iot_g_register,
        .set_register           = sc030iot_s_register,
    },
};

static int sc030iot_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        goto err_init_gpio;

    ret = dvp_init_select_gpio();
    if (ret)
        goto err_dvp_select_gpio;

    ret = camera_register_sensor(&sc030iot_sensor_attr);
    if (ret)
        goto err_camera_register;

    return 0;

err_camera_register:
    dvp_deinit_gpio();
err_dvp_select_gpio:
    deinit_gpio();

err_init_gpio:

    return ret;
}

static int sc030iot_remove(struct i2c_client *client)
{
    camera_unregister_sensor(&sc030iot_sensor_attr);
    dvp_deinit_gpio();
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id sc030iot_id[] = {
    { SC030IOT_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sc030iot_id);

static struct i2c_driver sc030iot_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = SC030IOT_DEVICE_NAME,
    },
    .probe              = sc030iot_probe,
    .remove             = sc030iot_remove,
    .id_table           = sc030iot_id,
};

static struct i2c_board_info sensor_sc030iot_info = {
    .type               = SC030IOT_DEVICE_NAME,
    .addr               = SC030IOT_DEVICE_I2C_ADDR,
};

static __init int init_sc030iot(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "sc030iot: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&sc030iot_driver);
    if (ret) {
        printk(KERN_ERR "sc030iot: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_sc030iot_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "sc030iot: failed to register i2c device\n");
        i2c_del_driver(&sc030iot_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_sc030iot(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sc030iot_driver);
}

module_init(init_sc030iot);
module_exit(exit_sc030iot);

MODULE_DESCRIPTION("x1600 sc030iot driver");
MODULE_LICENSE("GPL");

