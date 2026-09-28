/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * BF20A6
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera/camera_sensor.h>

#define BF20A6_DEVICE_NAME              "bf20a6-dvp"
#define BF20A6_DEVICE_I2C_ADDR          0x6E

#define BF20A6_CHIP_ID_H                0x20
#define BF20A6_CHIP_ID_L                0xa6

#define BF20A6_REG_END                  0xffff
#define BF20A6_REG_DELAY                0xfffe

static int power_gpio       = -1;   // ?
static int reset_gpio       = -1;   // ?
static int pwdn_gpio        = -1;   // ?
static int i2c_bus_num      = -1;   // 0

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr bf20a6_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

//BF20A6_YUV_DVP_VGA_XCLK24M_VCLK24M_max30fps_V2.1.1_20220720
//XCLK:24M; PLCK: 12M
//Width: 640; Height: 480
//行长：800  帧长：500
//Max fps: 30fps
static struct regval_list bf20a6_dvp_init_regs_640_480_30fps[] = {
    {0xf2,0x01},
    {0x12,0x20},
    {0x3a,0x00},
    {0xe1,0x92},
    {0xe3,0x02},
    {0xe0,0x00},
    {0x2a,0x98},

    {0x0e,0x47},
    {0x0f,0x60},
    {0x10,0x57},
    {0x11,0x60},
    {0x30,0x61},
    {0x62,0xcd},
    {0x63,0x1a},
    {0x64,0x38},
    {0x65,0x52},
    {0x66,0x68},
    {0x67,0xc2},
    {0x68,0xa7},
    {0x69,0xab},
    {0x6a,0xad},
    {0x6b,0xa9},
    {0x6c,0xc4},
    {0x6d,0xc5},
    {0x6e,0x18},
    {0xc0,0x20},
    {0xc1,0x24},
    {0xc2,0x29},
    {0xc3,0x25},
    {0xc4,0x28},
    {0xc5,0x2a},
    {0xc6,0x41},
    {0xca,0x23},
    {0xcd,0x34},
    {0xce,0x32},
    {0xcf,0x35},
    {0xd0,0x6c},
    {0xd1,0x6e},
    {0xd2,0xcb},

    {0xe4,0x73},
    {0xe5,0x22},
    {0xe6,0x24},
    {0xe7,0x64},
    {0xe8,0xa2},//DVP:a2;  SPI:f2        VDDIO=1.8V,E8[2]=1;VDDIO=2.8V,E8[2]=0;
    {0x4a,0x00},
    {0x00,0x03},
    {0x1f,0x02},
    {0x22,0x02},
    {0x0c,0x31},

    {0x00,0x00},
    {0x60,0x81},
    {0x61,0x81},

    {0xa0,0x08},
    {0x01,0x1a},
    {0x01,0x1a},
    {0x01,0x1a},
    {0x02,0x15},
    {0x02,0x15},
    {0x02,0x15},
    {0x13,0x08},
    {0x8a,0x96},
    {0x8b,0x06},
    {0x87,0x18},

    {0x34,0x48},//lens
    {0x35,0x40},
    {0x36,0x40},

    {0x71,0x44},
    {0x72,0x48},
    {0x74,0xa2},
    {0x75,0xa9},
    {0x78,0x12},
    {0x79,0xa0},
    {0x7a,0x94},
    {0x7c,0x97},
    {0x40,0x30},
    {0x41,0x30},
    {0x42,0x28},
    {0x43,0x1f},
    {0x44,0x1c},
    {0x45,0x16},
    {0x46,0x13},
    {0x47,0x10},
    {0x48,0x0D},
    {0x49,0x0C},
    {0x4B,0x0A},
    {0x4C,0x0B},
    {0x4E,0x09},
    {0x4F,0x08},
    {0x50,0x08},

    {0x5f,0x29},
    {0x23,0x33},
    {0xa1,0x10},//AWB
    {0xa2,0x0d},
    {0xa3,0x30},
    {0xa4,0x06},
    {0xa5,0x22},
    {0xa6,0x56},
    {0xa7,0x18},
    {0xa8,0x1a},
    {0xa9,0x12},
    {0xaa,0x12},
    {0xab,0x16},
    {0xac,0xb1},
    {0xba,0x12},
    {0xbb,0x12},
    {0xad,0x12},
    {0xae,0x56},
    {0xaf,0x0a},
    {0x3b,0x30},
    {0x3c,0x12},
    {0x3d,0x22},
    {0x3e,0x3f},
    {0x3f,0x28},
    {0xb8,0xc3},
    {0xb9,0xA3},
    {0x39,0x47},//pure color threshold
    {0x26,0x13},
    {0x27,0x16},
    {0x28,0x14},
    {0x29,0x18},
    {0xee,0x0d},

    {0x13,0x05},
    {0x24,0x3C},
    {0x81,0x20},
    {0x82,0x40},
    {0x83,0x30},
    {0x84,0x58},
    {0x85,0x30},
    {0x92,0x08},
    {0x86,0xA0},
    {0x8a,0x96},
    {0x91,0xff},
    {0x94,0x62},
    {0x9a,0x18},//outdoor threshold
    {0xf0,0x4e},
    {0x51,0x17},//color normal
    {0x52,0x03},
    {0x53,0x5F},
    {0x54,0x47},
    {0x55,0x66},
    {0x56,0x0F},
    {0x7e,0x14},
    {0x57,0x36},//A光color
    {0x58,0x2A},
    {0x59,0xAA},
    {0x5a,0xA8},
    {0x5b,0x43},
    {0x5c,0x10},
    {0x5d,0x00},
    {0x7d,0x36},
    {0x5e,0x10},

    {0xd6,0xa0},//contrast
    {0xd5,0x20},//低光加亮度
    {0xb0,0x84},//灰色区域降饱和度
    {0xb5,0x08},//低光降饱和度阈值
    {0xb1,0xc8},//saturation
    {0xb2,0xc0},
    {0xb3,0xd0},
    {0xb4,0xB0},

    {0x32,0x00},

    {0xa0,0x09},

    {0x00,0x03},

    {0x0b,0x02},

    {BF20A6_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list bf20a6_regs_stream_on[] = {
    {0xe0,0x00},
    {BF20A6_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list bf20a6_regs_stream_off[] = {
    {0xe0,0x01},
    {BF20A6_REG_END, 0x00},  /* END MARKER */
};


static int bf20a6_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
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
        printk(KERN_ERR "bf20a6: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int bf20a6_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    unsigned char buf[1] = {reg};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = i2c->addr,
            .flags  = 0,
            .len    = 1,
            .buf    = buf,
        },
        [1] = {
            .addr   = i2c->addr,
            .flags  = I2C_M_RD,
            .len    = 1,
            .buf    = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "bf20a6(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int bf20a6_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != BF20A6_REG_END) {
        if (vals->reg_num == BF20A6_REG_DELAY) {
            m_msleep(vals->value);
        } else {

            ret = bf20a6_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int bf20a6_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != BF20A6_REG_END) {
        if (vals->reg_num == BF20A6_REG_DELAY) {
            m_msleep(vals->value);
        } else {

            ret = bf20a6_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        if (val == vals->value)
            printk(KERN_ERR "reg = 0x%02x, val = 0x%02x  == 0x%02x\n",vals->reg_num, val, vals->value);
        else
            printk(KERN_ERR "reg = 0x%02x, val = 0x%02x  != 0x%02x\n",vals->reg_num, val, vals->value);

        vals++;
    }

    return 0;
}

static int bf20a6_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = bf20a6_read(i2c, 0xfc, &h);
    if (ret < 0)
        return ret;
    if (h != BF20A6_CHIP_ID_H) {
        printk(KERN_ERR "bf20a6 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = bf20a6_read(i2c, 0xfd, &l);
    if (ret < 0)
        return ret;

    if (l != BF20A6_CHIP_ID_L) {
        printk(KERN_ERR "bf20a6 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "bf20a6 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    ret = dvp_init_select_gpio();
    if (ret) {
        printk(KERN_ERR "bf20a6: failed to init dvp pins\n");
        return ret;
    }

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "bf20a6_reset");
        if (ret) {
            printk(KERN_ERR "bf20a6: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "bf20a6_pwdn");
        if (ret) {
            printk(KERN_ERR "bf20a6: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "bf20a6_power");
        if (ret) {
            printk(KERN_ERR "bf20a6: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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
    dvp_deinit_gpio();

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

static void bf20a6_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk();
}

static int bf20a6_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(50);
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(10);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(30);
    }

    ret = bf20a6_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "bf20a6: failed to detect\n");
        bf20a6_power_off();
        return ret;
    }

    ret = bf20a6_write_array(i2c_dev, bf20a6_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "bf20a6: failed to init regs\n");
        bf20a6_power_off();
        return ret;
    }

    return 0;
}

static int bf20a6_stream_on(void)
{
    int ret;

    ret = bf20a6_write_array(i2c_dev, bf20a6_regs_stream_on);
    if (ret)
        printk(KERN_ERR "bf20a6: failed to stream on\n");

    return ret;
}

static void bf20a6_stream_off(void)
{
    int ret = bf20a6_write_array(i2c_dev, bf20a6_regs_stream_off);
    if (ret)
        printk(KERN_ERR "bf20a6: failed to stream on\n");
}

static struct sensor_attr bf20a6_sensor_attr = {
    .device_name                = BF20A6_DEVICE_NAME,
    .cbus_addr                  = BF20A6_DEVICE_I2C_ADDR,

    .dvp = {
        .hsync_polarity         = POLARITY_HIGH_ACTIVE,
        .vsync_polarity         = POLARITY_HIGH_ACTIVE,
        .pclk_polarity          = POLARITY_SAMPLE_RISING,
        .img_scan_mode          = DVP_IMG_SCAN_PROGRESS,
    },

    .sensor_info = {
        .private_init_setting   = bf20a6_dvp_init_regs_640_480_30fps,
        .width                  = 640,
        .height                 = 480,
        .fmt                    = SENSOR_PIXEL_FMT_UYVY8_2X8,

        .fps                    = 30 << 16 | 1,
    },

    .ops = {
        .power_on               = bf20a6_power_on,
        .power_off              = bf20a6_power_off,
        .stream_on              = bf20a6_stream_on,
        .stream_off             = bf20a6_stream_off,
    },
};


static int bf20a6_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int ret;

    ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(&bf20a6_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int bf20a6_remove(struct i2c_client *client)
{
    camera_unregister_sensor(&bf20a6_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id bf20a6_id[] = {
    { BF20A6_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, bf20a6_id);

static struct i2c_driver bf20a6_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = BF20A6_DEVICE_NAME,
    },
    .probe              = bf20a6_probe,
    .remove             = bf20a6_remove,
    .id_table           = bf20a6_id,
};

static struct i2c_board_info sensor_bf20a6_info = {
    .type               = BF20A6_DEVICE_NAME,
    .addr               = BF20A6_DEVICE_I2C_ADDR,
};

static __init int init_bf20a6(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "bf20a6: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&bf20a6_driver);
    if (ret) {
        printk(KERN_ERR "bf20a6: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_bf20a6_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "bf20a6: failed to register i2c device\n");
        i2c_del_driver(&bf20a6_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_bf20a6(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&bf20a6_driver);
}

module_init(init_bf20a6);
module_exit(exit_bf20a6);

MODULE_DESCRIPTION("x2600 bf20a6 dvp driver");
MODULE_LICENSE("GPL");
