/*
 * Copyright (C) 2022 Ingenic Semiconductor Co., Ltd.
 *
 * BF2013
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera_sensor.h>

#define BF2013_DEVICE_NAME          "bf2013-dvp"
#define BF2013_DEVICE_I2C_ADDR      0x6e
#define BF2013_CHIP_ID_H            0x37
#define BF2013_CHIP_ID_L            0x03
#define ENDMARKER                   {0xff, 0xff}

#define BF2013_REG_DELAY            0xfe
#define BF2013_REG_END              0xff

static int power_gpio   = -1;       //
static int reset_gpio   = -1;       //PA30
static int pwdn_gpio    = -1;       //PA31
static int i2c_bus_num  = -1;       //i2c 0

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr bf2013_sensor_attr;

struct regval_list {
    unsigned char reg_num;
    unsigned char value;
};

/*
 *  在光线较亮时帧率最高达到25，在光线不足时可能帧率会降低。
 */
static struct regval_list bf2013_init_regs_640_480_dvp_25fps[] = {
    /*System*/
    //{0x12,0x80},   /* bit7置1是重置操作 会导致i2c通信失败 */
    {0x09,0x02},
    {0x15,0x02},
    {0x3a,0x00},
    {0x12,0x00},
    {0x1e,0x00},
    {0x13,0x00},
    {0x01,0x14},
    {0x02,0x21},
    {0x8c,0x01},
    {0x8d,0xcb},
    {0x87,0x20},
    {0x1b,0x80},
    {0x11,0x80},
    {0x2b,0x00},
    {0x92,0x05},
    {0x06,0xe0},
    {0x29,0x54},
    {0xeb,0x30},
    {0xbb,0x20},
    {0xf5,0x21},
    {0xe1,0x3c},
    {0x16,0x01},
    {0xe0,0x0b},
    {0x2f,0xf6},
    {0x1f,0x20},
    {0x22,0x20},
    {0x26,0x20},
    {0x33,0x20},
    {0x34,0x08},
    {0x35,0x50},
    {0x65,0x4a},
    {0x66,0x48},
    {0x36,0x05},
    {0x37,0xf6},
    {0x38,0x46},
    {0x9b,0xf6},
    {0x9c,0x46},
    {0xbc,0x01},
    {0xbd,0xf6},
    {0xbe,0x46},
    {0x70,0x6f},
    {0x72,0x3f},
    {0x73,0x3f},
    {0x74,0x27},
    {0x77,0x90},
    {0x79,0x48},
    {0x7a,0x1e},
    {0x7b,0x30},
    {0x24,0x70},
    {0x25,0x80},
    {0x80,0x55},
    {0x81,0x02},
    {0x82,0x14},
    {0x83,0x23},
    {0x9a,0x23},
    {0x84,0xff},
    {0x85,0xff},
    {0x86,0x30},
    {0x89,0x00},
    {0x8a,0x64},
    {0x8b,0x02},
    {0x8e,0x03},
    {0x8f,0x79},
    {0x94,0x0a},
    {0x96,0xa6},
    {0x97,0x0c},
    {0x98,0x18},
    {0x9d,0x93},
    {0x9e,0x7a},
    {0x3b,0x60},
    {0x3c,0x20},
    {0x39,0x80},
    {0x3f,0xb0},
    {0x40,0x9b},
    {0x41,0x88},
    {0x42,0x6e},
    {0x43,0x59},
    {0x44,0x4d},
    {0x45,0x45},
    {0x46,0x3e},
    {0x47,0x39},
    {0x48,0x35},
    {0x49,0x31},
    {0x4b,0x2e},
    {0x4c,0x2b},
    {0x4e,0x26},
    {0x4f,0x22},
    {0x50,0x1f},
    {0x51,0x05},
    {0x52,0x10},
    {0x53,0x0b},
    {0x54,0x15},
    {0x57,0x87},
    {0x58,0x72},
    {0x59,0x5f},
    {0x5a,0x7e},
    {0x5b,0x1f},
    {0x5c,0x0e},
    {0x5d,0x95},
    {0x60,0x24},
    {0x6a,0x01},
    {0x23,0x66},
    {0xa0,0x03},
    {0xa1,0x31},
    {0xa2,0x0e},
    {0xa3,0x27},
    {0xa4,0x08},
    {0xa5,0x25},
    {0xa6,0x06},
    {0xa7,0x80},
    {0xa8,0x7e},
    {0xa9,0x20},
    {0xaa,0x20},
    {0xab,0x20},
    {0xac,0x3c},
    {0xad,0xf0},
    {0xae,0x80},
    {0xaf,0x00},
    {0xc5,0x18},
    {0xc6,0x00},
    {0xc7,0x20},
    {0xc8,0x18},
    {0xc9,0x20},
    {0xca,0x17},
    {0xcb,0x1f},
    {0xcc,0x40},
    {0xcd,0x58},
    {0xee,0x4c},
    {0xb0,0xe0},
    {0xb1,0xc0},
    {0xb2,0xb0},
    {0xb3,0x88},
    {0x56,0x40},
    {0x13,0x07},
    {0x17,0x01},
    {0x18,0xA1},
    {0x03,0xF0},
    {0x19,0x00},
    {0x1A,0x78},
    ENDMARKER,  /* END MARKER */
};

static struct regval_list bf2013_regs_stream_on[] = {
    {0x09, 0x02},
    ENDMARKER,
};

static struct regval_list bf2013_regs_stream_off[] = {
    {0x09, 0x12},
    ENDMARKER,
};

static struct regval_list bf2013_chip_id_regs[] = {
    {0xfc, 0x00},
    {0xfd, 0x00},
    ENDMARKER,
};

static int bf2013_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
{
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "bf2013(%x): failed to write reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int bf2013_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
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
    if (ret < 0)
        printk(KERN_ERR "bf2013(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int bf2013_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != BF2013_REG_END) {
        if (vals->reg_num == BF2013_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = bf2013_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int bf2013_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != BF2013_REG_END) {
        if (vals->reg_num == BF2013_REG_DELAY) {

        } else {
            ret = bf2013_read(i2c, vals->reg_num, &vals->value);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "{0x%x, 0x%x}\n", vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static int bf2013_detect(struct i2c_client *i2c)
{
    int ret;

    ret = bf2013_read(i2c, bf2013_chip_id_regs[0].reg_num, &bf2013_chip_id_regs[0].value);
    if(ret < 0)
        return ret;

    if (bf2013_chip_id_regs[0].value != BF2013_CHIP_ID_H) {
        printk(KERN_ERR "bf2013 read 1 chip id failed:{0x%x, 0x%x}\n", bf2013_chip_id_regs[0].reg_num, bf2013_chip_id_regs[0].value);
        return -ENODEV;
    }

    ret = bf2013_read(i2c, bf2013_chip_id_regs[1].reg_num, &bf2013_chip_id_regs[1].value);
    if(ret < 0)
        return ret;

    if (bf2013_chip_id_regs[1].value != BF2013_CHIP_ID_L) {
        printk(KERN_ERR "bf2013 read 2 chip id failed:{0x%x, 0x%x}\n", bf2013_chip_id_regs[1].reg_num, bf2013_chip_id_regs[1].value);
        return -ENODEV;
    }
    //printk(KERN_ERR "bf2013 read chip id : h = 0x%x,l = 0x%x\n", bf2013_chip_id_regs[0].value, bf2013_chip_id_regs[1].value);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "bf2013_reset");
        if (ret) {
            printk(KERN_ERR "bf2013: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "bf2013_pwdn");
        if (ret) {
            printk(KERN_ERR "bf2013: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "bf2013_power");
        if (ret) {
            printk(KERN_ERR "bf2013: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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
}

static void bf2013_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk();
}

static int bf2013_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(24*1000*1000);
    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (pwdn_gpio != -1) {
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(10);
    }

    if (reset_gpio != -1) {
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    ret = bf2013_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "bf2013: failed to detect\n");
        bf2013_power_off();
        return ret;
    }
    //printk(KERN_ERR "bf2013: bf2013 detect success\n");
    ret = bf2013_write_array(i2c_dev, bf2013_sensor_attr.sensor_info.private_init_setting);
    //ret = bf2013_read_array(i2c_dev, bf2013_sensor_attr.sensor_info.private_init_setting);
    if (0 != ret) {
        printk(KERN_ERR "bf2013: failed to init regs\n");
        bf2013_power_off();
        return ret;
    }

    return 0;
}

static int bf2013_stream_on(void)
{
    int ret = bf2013_write_array(i2c_dev, bf2013_regs_stream_on);
    if (ret)
        printk(KERN_ERR "bf2013: failed to stream on\n");

    return ret;
}

static void bf2013_stream_off(void)
{
    int ret = bf2013_write_array(i2c_dev, bf2013_regs_stream_off);
    if (ret)
        printk(KERN_ERR "bf2013: failed to stream on\n");
}

static int bf2013_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = bf2013_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int bf2013_s_register(struct sensor_dbg_register *reg)
{
    return bf2013_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static struct sensor_attr bf2013_sensor_attr = {

    .device_name                = BF2013_DEVICE_NAME,
    .cbus_addr                  = BF2013_DEVICE_I2C_ADDR,

    .dma_mode                   = SENSOR_DATA_DMA_MODE_RAW,    // olny dvp.data_fmt is DVP_YUV422
    .dbus_type                  = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .pclk_polarity          = POLARITY_SAMPLE_RISING,
        .hsync_polarity         = POLARITY_HIGH_ACTIVE,
        .vsync_polarity         = POLARITY_HIGH_ACTIVE,
        .img_scan_mode          = DVP_IMG_SCAN_PROGRESS,
    },

    .sensor_info = {
        .private_init_setting   = bf2013_init_regs_640_480_dvp_25fps,
        .width                  = 640,
        .height                 = 480,
        .fmt                    = SENSOR_PIXEL_FMT_YUYV8_2X8,
        .fps                    = 25 << 16 | 1,
    },

    .ops = {
        .power_on               = bf2013_power_on,
        .power_off              = bf2013_power_off,
        .stream_on              = bf2013_stream_on,
        .stream_off             = bf2013_stream_off,
        .get_register           = bf2013_g_register,
        .set_register           = bf2013_s_register,
    },
};

static int bf2013_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        goto err_init_gpio;

    ret = dvp_init_select_gpio();
    if (ret)
        goto err_dvp_select_gpio;

    ret = camera_register_sensor(&bf2013_sensor_attr);
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

static int bf2013_remove(struct i2c_client *client)
{
    camera_unregister_sensor(&bf2013_sensor_attr);
    dvp_deinit_gpio();
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id bf2013_id[] = {
    { BF2013_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, bf2013_id);

static struct i2c_driver bf2013_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = BF2013_DEVICE_NAME,
    },
    .probe              = bf2013_probe,
    .remove             = bf2013_remove,
    .id_table           = bf2013_id,
};

static struct i2c_board_info sensor_bf2013_info = {
    .type               = BF2013_DEVICE_NAME,
    .addr               = BF2013_DEVICE_I2C_ADDR,
};

static __init int init_bf2013(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "bf2013: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&bf2013_driver);
    if (ret) {
        printk(KERN_ERR "bf2013: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_bf2013_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "bf2013: failed to register i2c device\n");
        i2c_del_driver(&bf2013_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_bf2013(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&bf2013_driver);
}

module_init(init_bf2013);
module_exit(exit_bf2013);

MODULE_DESCRIPTION("x1600 bf2013 driver");
MODULE_LICENSE("GPL");
