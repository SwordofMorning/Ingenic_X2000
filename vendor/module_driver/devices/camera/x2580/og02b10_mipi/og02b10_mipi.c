/*
 * og02b10.c
 *
 * Copyright (C) 2022 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Settings:
 * sboot        resolution      fps     interface              mode
 *   0          1600*1280       60        mipi_2lane           linear
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <soc/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <camera/hal/camera_sensor.h>


#define MCLK                            24 * 1000 * 1000

#define OG02B10_DEVICE_NAME             "og02b10"
#define OG02B10_DEVICE_I2C_ADDR         0x60
#define OG02B10_SUPPORT_FPS_SCLK        160*1000*1000

#define OG02B10_DEVICE_WIDTH            1600
#define OG02B10_DEVICE_HEIGHT           1280

#define SENSOR_OUTPUT_MIN_FPS           5
#define SENSOR_OUTPUT_MAX_FPS           60
#define OG02B10_CHIP_ID_H               (0x23)
#define OG02B10_CHIP_ID_M               (0x11)
#define OG02B10_CHIP_ID_L               (0xa0)
#define OG02B10_REG_END                 0xffff
#define OG02B10_REG_DELAY               0x0000

#define RAW8    0
#define FPS_30  0


static char *sensor_name = "og02b10";
static int pwdn_gpio     = -1;
static int power_gpio    = -1;
static int i2c_bus_num   = -1;
static int i2c_addr      = 0x60;
static int cam_bus_num   = 0;

module_param_gpio(power_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(sensor_name, charp, 0644);
module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, int, 0644);
module_param(cam_bus_num, int, 0644);

static struct sensor_attr og02b10_sensor_attr;
static struct i2c_client *i2c_dev;

struct regval_list {
    uint16_t reg_num;
    unsigned char value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
    unsigned int index;
    unsigned char reg3508;
    unsigned char reg3509;
    unsigned int gain;
};

struct again_lut og02b10_again_lut[] = {
    {0x00, 0x01, 0x00, 0},     // x1
    {0x01, 0x01, 0x10, 5731},
    {0x02, 0x01, 0x20, 11136},
    {0x03, 0x01, 0x30, 16248},
    {0x04, 0x01, 0x40, 21097},
    {0x05, 0x01, 0x50, 25710},
    {0x06, 0x01, 0x60, 30109},
    {0x07, 0x01, 0x70, 34312},
    {0x08, 0x01, 0x80, 38336},
    {0x09, 0x01, 0x90, 42195},
    {0x0a, 0x01, 0xa0, 45904},
    {0x0b, 0x01, 0xb0, 49472},
    {0x0c, 0x01, 0xc0, 52910},
    {0x0d, 0x01, 0xd0, 56228},
    {0x0e, 0x01, 0xe0, 59433},
    {0x0f, 0x01, 0xf0, 62534},
    {0x10, 0x02, 0x00, 65536}, //x2
    {0x11, 0x02, 0x20, 71267},
    {0x12, 0x02, 0x40, 76672},
    {0x13, 0x02, 0x60, 81784},
    {0x14, 0x02, 0x80, 86633},
    {0x15, 0x02, 0xa0, 91246},
    {0x16, 0x02, 0xc0, 95645},
    {0x17, 0x02, 0xe0, 99848},
    {0x18, 0x03, 0x00, 103872},
    {0x19, 0x03, 0x20, 107731},
    {0x1a, 0x03, 0x40, 111440},
    {0x1b, 0x03, 0x60, 115008},
    {0x1c, 0x03, 0x80, 118446},
    {0x1d, 0x03, 0xa0, 121764},
    {0x1e, 0x03, 0xc0, 124969},
    {0x1f, 0x03, 0xe0, 128070},
    {0x20, 0x04, 0x00, 131072}, //x4
    {0x21, 0x04, 0x40, 136803},
    {0x22, 0x04, 0x80, 142208},
    {0x23, 0x04, 0xc0, 147320},
    {0x24, 0x05, 0x00, 152169},
    {0x25, 0x05, 0x40, 156782},
    {0x26, 0x05, 0x80, 161181},
    {0x27, 0x05, 0xc0, 165384},
    {0x28, 0x06, 0x00, 169408},
    {0x29, 0x06, 0x40, 173267},
    {0x2a, 0x06, 0x80, 176976},
    {0x2b, 0x06, 0xc0, 180544},
    {0x2c, 0x07, 0x00, 183982},
    {0x2d, 0x07, 0x40, 187300},
    {0x2e, 0x07, 0x80, 190505},
    {0x2f, 0x07, 0xc0, 193606},
    {0x30, 0x08, 0x00, 196608}, //x8
    {0x31, 0x08, 0x80, 202339},
    {0x32, 0x09, 0x00, 207744},
    {0x33, 0x09, 0x80, 212856},
    {0x34, 0x0a, 0x00, 217705},
    {0x35, 0x0a, 0x80, 222318},
    {0x36, 0x0b, 0x00, 226717},
    {0x37, 0x0b, 0x80, 230920},
    {0x38, 0x0c, 0x00, 234944},
    {0x39, 0x0c, 0x80, 238803},
    {0x3a, 0x0d, 0x00, 242512},
    {0x3b, 0x0d, 0x80, 246080},
    {0x3c, 0x0e, 0x00, 249518},
    {0x3d, 0x0e, 0x80, 252836},
    {0x3e, 0x0f, 0x00, 256041},
    {0x3f, 0x0f, 0x80, 259142}, // x15.5
};

static struct regval_list og02b10_init_regs_1600_1280_60fps_mipi[] = {
    {0x0103, 0x01},
    {0x0100, 0x00},
    {0x010c, 0x02},
    {0x010b, 0x01},

    /* PLL */
    {0x0300, 0x01},
    {0x0302, 0x32},
    {0x0303, 0x00},
    {0x0304, 0x03},
    {0x0305, 0x02},
    {0x0306, 0x01},
    {0x030d, 0x5a},
    {0x030e, 0x04},

    {0x3001, 0x02},
    {0x3004, 0x00},
    {0x3005, 0x00},
    {0x3006, 0x0a},
    {0x3011, 0x0d},
    {0x3014, 0x04},
    {0x301c, 0xf0},
    {0x3020, 0x20},
    {0x302c, 0x00},
    {0x302d, 0x00},
    {0x302e, 0x00},
    {0x302f, 0x03},
    {0x3030, 0x10},
    {0x303f, 0x03},

    /* SCCB */
    {0x3103, 0x00},
    {0x3106, 0x08},
    {0x31ff, 0x01},

    /* AE Ctrl */
    {0x3501, 0x05}, // expo_h8
    {0x3502, 0x7c},
    {0x3506, 0x00},
    {0x3507, 0x00},

    {0x3620, 0x67},
    {0x3633, 0x78},
    {0x3662, 0x65}, //[1]0-raw10, 1-raw8; [2]mipi-2lane-en
#if RAW8
    {0x3662, 0x67},
#endif
    {0x3664, 0xb0},
    {0x3666, 0x70},
    {0x3670, 0x68},
    {0x3674, 0x10},
    {0x3675, 0x00},
    {0x367e, 0x90},
    {0x3680, 0x84},
    {0x3683, 0x96},
    {0x36a2, 0x04},
    {0x36a3, 0x80},
    {0x36b0, 0x00},
    {0x3700, 0x35},
    {0x3704, 0x39},
    {0x370a, 0x50},
    {0x3712, 0x00},
    {0x3713, 0x02},
    {0x3778, 0x00},
    {0x379b, 0x01},
    {0x379c, 0x10},
    /* timing */
    {0x3800, 0x00}, //x_start_h8
    {0x3801, 0x00},
    {0x3802, 0x00}, //y_start_h8
    {0x3803, 0x00},
    {0x3804, 0x06}, //x_end 0x64f(1615)
    {0x3805, 0x4f},
    {0x3806, 0x05}, //y_end 0x523(1315)
    {0x3807, 0x23},
    {0x3808, 0x06}, //x_output 0x640(1600)
    {0x3809, 0x40},
    {0x380a, 0x05}, //y_output 0x500(1280)
    {0x380b, 0x00},
    {0x380c, 0x03}, //HTS 0x3a8(936)
    {0x380d, 0xa8},
    {0x380e, 0x05}, //VTS 0x588(1416)
    {0x380f, 0x88},
#if FPS_30
    {0x380e, 0x0b}, //VTS 0xb10(2832)
    {0x380f, 0x10},
#endif
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x08},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3816, 0x00},
    {0x3817, 0x01},
    {0x3818, 0x00},
    {0x3819, 0x05},
    {0x3820, 0x00}, //[2]flip, [1]v_binning
    {0x3821, 0x00}, //[2]mirror, [1]h_binning
    {0x382b, 0x52},
    {0x382c, 0x0a},
    {0x382d, 0xf8},
    /* global shutter */
    {0x3881, 0x44},
    {0x3882, 0x02},
    {0x3883, 0x8c},
    {0x3885, 0x07},
    {0x389d, 0x03},
    {0x38a6, 0x00},
    {0x38a7, 0x01},
    {0x38b3, 0x07},
    {0x38b1, 0x00},
    {0x38e5, 0x02},
    {0x38e7, 0x00},
    {0x38e8, 0x00},
    /* pwm ctrl */
    {0x3910, 0xff},
    {0x3911, 0xff},
    {0x3912, 0x08},
    {0x3913, 0x00},
    {0x3914, 0x00},
    {0x3915, 0x00},
    {0x391c, 0x00},
    {0x3920, 0xff},
    {0x3921, 0x80},
    {0x3922, 0x00},
    {0x3923, 0x00},
    {0x3924, 0x05},
    {0x3925, 0x00},
    {0x3926, 0x00},
    {0x3927, 0x00},
    {0x3928, 0x1a},
    {0x392d, 0x03},
    {0x392e, 0xa8},
    {0x392f, 0x08},
    /* BLC */
    {0x4001, 0x00},
    {0x4003, 0x40},
    {0x4008, 0x04},
    {0x4009, 0x1b},
    {0x400c, 0x04},
    {0x400d, 0x1b},
    {0x4010, 0xf4},
    {0x4011, 0x00},
    {0x4016, 0x00},
    {0x4017, 0x04},
    {0x4042, 0x11},
    {0x4043, 0x70},
    {0x4045, 0x00},

    {0x4409, 0x5f},
    {0x4509, 0x00},
    {0x450b, 0x00},
    {0x4600, 0x00},
    {0x4601, 0xa0},
    {0x4708, 0x09}, //polarity ctrl
    {0x470c, 0x81},
    {0x4710, 0x06},
    {0x4711, 0x00},
    /* mipi ctrl */
    {0x4800, 0x00},
#if RAW8
    {0x4814, 0x6a},
#endif
    {0x481f, 0x30},
    {0x4837, 0x14},
    /* psv ctrl */
    {0x4f00, 0x00},
    {0x4f07, 0x00},
    {0x4f08, 0x03},
    {0x4f09, 0x08},
    {0x4f0c, 0x05},
    {0x4f0d, 0xb4},
    {0x4f10, 0x00},
    {0x4f11, 0x00},
    {0x4f12, 0x07},
    {0x4f13, 0xe2},
    /* ISP */
    {0x5000, 0x9f},
    {0x5001, 0x20},
    {0x5026, 0x00},
    {0x5c00, 0x00},
    {0x5c01, 0x2c},
    {0x5c02, 0x00},
    {0x5c03, 0x7f},
    {0x5e00, 0x00},
    {0x5e01, 0x41},

    {0x38b1, 0x02},
    {0x3880, 0x00},

    // {0x0100, 0x01},
    {OG02B10_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list og02b10_regs_stream_on[] = {
    {0x0100,0x01},
    {OG02B10_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list og02b10_regs_stream_off[] = {
    {0x0100,0x00},
    {OG02B10_REG_END, 0x00},    /* END MARKER */
};

static int og02b10_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
{
    unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 3,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "og02b10: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int og02b10_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
{
    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = i2c->addr,
            .flags  = 0,
            .len    = 2,
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
        printk(KERN_ERR "og02b10(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int og02b10_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != OG02B10_REG_END) {
        if (vals->reg_num == OG02B10_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = og02b10_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int og02b10_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != OG02B10_REG_END) {
        if (vals->reg_num == OG02B10_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = og02b10_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        if (vals->value != val) {
            printk("reg = 0x%04x, val = 0x%02x\n",vals->reg_num, val);
        }

        vals++;
    }
    return 0;
}

static int og02b10_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char m = 1;
    unsigned char l = 1;

    ret = og02b10_read(i2c, 0x300a, &h);
    if (ret < 0)
        return ret;
    if (h != OG02B10_CHIP_ID_H) {
        printk(KERN_ERR "og02b10 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = og02b10_read(i2c, 0x300b, &m);
    if (ret < 0)
        return ret;
    if (m != OG02B10_CHIP_ID_M) {
        printk(KERN_ERR "og02b10 read chip id medium failed:0x%x\n", m);
        return -ENODEV;
    }

    ret = og02b10_read(i2c, 0x300c, &l);
    if (ret < 0)
        return ret;
    if (l != OG02B10_CHIP_ID_L) {
        printk(KERN_ERR "og02b10 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }

    printk(KERN_DEBUG "og02b10 get chip id = %02x%02x%02x\n", h, m, l);

    return 0;
}

static void og02b10_power_off(void)
{
    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 0);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int og02b10_power_on(void)
{
    int ret, retry;

    camera_enable_sensor_mclk(cam_bus_num, MCLK);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(5);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
    }


    for (retry = 0; retry < 5; retry++) {
        ret = og02b10_detect(i2c_dev);
        if (!ret)
            break;
        m_msleep(10);
    }

    if (retry >= 5) {
        printk(KERN_ERR "og02b10: failed to detect\n");
        goto fail;
    }

    ret = og02b10_write_array(i2c_dev, og02b10_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "og02b10: failed to init regs\n");
        goto fail;
    }

    return 0;

fail:
    og02b10_power_off();
    return ret;
}

static int og02b10_stream_on(void)
{
    int ret;

    ret = og02b10_write_array(i2c_dev, og02b10_regs_stream_on);
    if (ret)
        printk(KERN_ERR "og02b10: failed to stream on\n");

    return ret;
}

static void og02b10_stream_off(void)
{
    int ret;

    ret = og02b10_write_array(i2c_dev, og02b10_regs_stream_off);
    if (ret)
        printk(KERN_ERR "og02b10: failed to stream on\n");
}

static int og02b10_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = og02b10_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;

    return ret;
}

static int og02b10_s_register(struct sensor_dbg_register *reg)
{
    return og02b10_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

unsigned int og02b10_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = og02b10_again_lut;
    while(lut->gain <= og02b10_sensor_attr.sensor_info.max_again) {
        if(isp_gain == 0) {
            *sensor_again = 0;
            return lut[0].gain;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->index;
            return (lut - 1)->gain;
        } else {
            if((lut->gain == og02b10_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }

        lut++;
    }

    return 0;
}

unsigned int og02b10_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return 0;
}

inline static int og02b10_set_expo(int value)
{
    int ret = 0;

    return ret;
}

static int og02b10_set_integration_time(int value)
{
    int ret = 0;
    int it = value & 0xffff;

    ret += og02b10_write(i2c_dev, 0x3501, (unsigned char)((it >> 8) & 0xff));
    ret += og02b10_write(i2c_dev, 0x3502, (unsigned char)(it & 0xff));
    if (ret < 0)
        return ret;

    return 0;
}

static int og02b10_set_analog_gain(int value)
{
    int ret = 0;
    int again = value & 0xffff;
    struct again_lut *val_lut = og02b10_again_lut;

    /*set analog gain*/
    ret += og02b10_write(i2c_dev, 0x3508, val_lut[again].reg3508);
    ret += og02b10_write(i2c_dev, 0x3509, val_lut[again].reg3509);
    if (ret < 0)
        return ret;

    return 0;
}

inline static int og02b10_set_digital_gain(int value)
{
    return 0;
}

inline static int og02b10_set_fps(int fps)
{
    struct sensor_info *sensor_info = &og02b10_sensor_attr.sensor_info;
    unsigned int sclk = OG02B10_SUPPORT_FPS_SCLK;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "warn: og02b10 fps(0x%x) no in range\n", fps);
        return -1;
    }

    ret = og02b10_read(i2c_dev, 0x380c, &tmp);
    hts = tmp;
    ret += og02b10_read(i2c_dev, 0x380d, &tmp);
    if (ret < 0) {
        printk(KERN_ERR "err: og02b10_read err\n");
        return ret;
    }

    hts = (hts << 8) + tmp;

    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = og02b10_write(i2c_dev, 0x380f, (unsigned char)(vts & 0xff));
    ret += og02b10_write(i2c_dev, 0x380e, (unsigned char)(vts >> 8));
    if (ret < 0) {
        printk(KERN_ERR "err: og02b10_write err\n");
        return ret;
    }
    printk(KERN_ERR "hts: %x, vts: %x\n", hts, vts);

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;

}

static struct sensor_attr og02b10_sensor_attr = {
    .device_name                = OG02B10_DEVICE_NAME,
    .cbus_addr                  = OG02B10_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 2,
        .clk                    = 400,  /* Mbps/lane */
    },

    .isp_clk_rate               = 300 * 1000 * 1000,

#if FPS_30
       .sensor_info = {
        .private_init_setting   = og02b10_init_regs_1600_1280_60fps_mipi,
        .width                  = OG02B10_DEVICE_WIDTH,
        .height                 = OG02B10_DEVICE_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 30 << 16 | 1,
        .total_width            = 0x3a8,
        .total_height           = 0xb10,

        .min_integration_time   = 2,
        .max_integration_time   = 0xb10 - 12,
        .max_again              = 259142,
        .max_dgain              = 0,
    },
#else
    .sensor_info = {
        .private_init_setting   = og02b10_init_regs_1600_1280_60fps_mipi,
        .width                  = OG02B10_DEVICE_WIDTH,
        .height                 = OG02B10_DEVICE_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR10_1X10,

        .fps                    = 60 << 16 | 1,
        .total_width            = 0x3a8,
        .total_height           = 0x588,

        .min_integration_time   = 2,
        .max_integration_time   = 0x588 - 12,
        .max_again              = 259142,
        .max_dgain              = 0,
    },
#endif

    .ops = {
        .power_on               = og02b10_power_on,
        .power_off              = og02b10_power_off,
        .stream_on              = og02b10_stream_on,
        .stream_off             = og02b10_stream_off,
        .get_register           = og02b10_g_register,
        .set_register           = og02b10_s_register,

        .set_integration_time   = og02b10_set_integration_time,
        .alloc_again            = og02b10_alloc_again,
        .set_analog_gain        = og02b10_set_analog_gain,
        .alloc_dgain            = og02b10_alloc_dgain,
        .set_digital_gain       = og02b10_set_digital_gain,
        .set_fps                = og02b10_set_fps,

    },
};

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "og02b10_pwdn");
        if (ret) {
            printk(KERN_ERR "og02b10: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "og02b10_power");
        if (ret) {
            printk(KERN_ERR "og02b10: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            if (pwdn_gpio != -1)
                gpio_free(pwdn_gpio);
            return ret;
        }
    }

        return 0;
}

static void deinit_gpio(void)
{
    if (power_gpio != -1)
        gpio_free(power_gpio);

    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);
}

static int og02b10_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    og02b10_sensor_attr.cbus_addr = i2c_addr,
    ret = camera_register_sensor(cam_bus_num, &og02b10_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }
    printk("probe ok ------->og02b10\n");

    return 0;
}

static int og02b10_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &og02b10_sensor_attr);
    deinit_gpio();

    return 0;
}

static const struct i2c_device_id og02b10_id[] = {
    { OG02B10_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, og02b10_id);

static struct i2c_board_info sensor_og02b10_info = {
    .type = OG02B10_DEVICE_NAME,
    .addr = OG02B10_DEVICE_I2C_ADDR,
};

static struct i2c_driver og02b10_driver = {
    .driver = {
        .owner   = THIS_MODULE,
        .name    = OG02B10_DEVICE_NAME,
    },
    .probe       = og02b10_probe,
    .remove      = og02b10_remove,
    .id_table    = og02b10_id,
};

static __init int init_og02b10(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "og02b10: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    og02b10_driver.driver.name = sensor_name;
    strcpy(sensor_og02b10_info.type, sensor_name);

    if (i2c_addr != -1){
        sensor_og02b10_info.addr = i2c_addr;
    }

    int ret = i2c_add_driver(&og02b10_driver);
    if (ret) {
        printk(KERN_ERR "og02b10: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_og02b10_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "og02b10: failed to register i2c device\n");
        i2c_del_driver(&og02b10_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_og02b10(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&og02b10_driver);
}

module_init(init_og02b10);
module_exit(exit_og02b10);

MODULE_DESCRIPTION("A low-level driver for og02b10 sensor");
MODULE_LICENSE("GPL");