/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * GC032A
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera_sensor.h>

#define GC032A_DEVICE_NAME          "gc032a-dvp"
#define GC032A_DEVICE_I2C_ADDR      0x21
#define GC032A_CHIP_ID_H            0x23
#define GC032A_CHIP_ID_L            0x2a
#define GC032A_SUPPORT_SCLK         24*1000*1000
#define ENDMARKER                   {0xff, 0xff}

static int power_gpio   = -1;       //
static int reset_gpio   = -1;       //PA30
static int pwdn_gpio    = -1;       //PA31
static int i2c_bus_num  = -1;       //i2c 0

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr gc032a_sensor_attr;

struct regval_list {
    unsigned char reg_num;
    unsigned char value;
};

static struct regval_list gc032a_init_regs_320_240_dvp_25fps[] = {
    /*System*/
    {0xf3, 0xff},
    {0xf5, 0x06},
    {0xf7, 0x01},
    {0xf8, 0x03},
    {0xf9, 0xce},
    {0xfa, 0x00},
    {0xfc, 0x02},
    {0xfe, 0x02},
    {0x81, 0x03},

    {0xfe, 0x00},
    {0x77, 0x64},
    {0x78, 0x40},
    {0x79, 0x60},
    /*ANALOG & CISCTL*/
    {0xfe, 0x00},
    {0x03, 0x01},
    {0x04, 0xce},
    {0x05, 0x01},
    {0x06, 0xad},
    {0x07, 0x00},
    {0x08, 0x10},
    {0x0a, 0x00},
    {0x0c, 0x00},
    {0x0d, 0x01},
    {0x0e, 0xe8}, // height 488
    {0x0f, 0x02},
    {0x10, 0x88}, // width 648
    {0x17, 0x54},
    {0x19, 0x08},
    {0x1a, 0x0a},
    {0x1f, 0x40},
    {0x20, 0x30},
    {0x2e, 0x80},
    {0x2f, 0x2b},
    {0x30, 0x1a},
    {0xfe, 0x02},
    {0x03, 0x02},
    {0x05, 0xd7},
    {0x06, 0x60},
    {0x08, 0x80},
    {0x12, 0x89},

    /*blk*/
    {0xfe, 0x00},
    {0x18, 0x02},
    {0xfe, 0x02},
    {0x40, 0x22},
    {0x45, 0x00},
    {0x46, 0x07},
    {0x49, 0x20},
    {0x4b, 0x3c},
    {0x50, 0x20},
    {0x42, 0x10},

    /*isp*/
    {0xfe, 0x01},
    {0x0a, 0xc5},
    {0x45, 0x00},
    {0xfe, 0x00},
    {0x40, 0xff},
    {0x41, 0x25},
    {0x42, 0xcf},
    {0x43, 0x10},
    {0x44, 0x83},
    {0x46, 0x23},
    {0x49, 0x03},
    {0x52, 0x02},
    {0x54, 0x00},
    {0xfe, 0x02},
    {0x22, 0xf6},

    /*Shading*/
    {0xfe, 0x01},
    {0xc1, 0x38},
    {0xc2, 0x4c},
    {0xc3, 0x00},
    {0xc4, 0x32},
    {0xc5, 0x24},
    {0xc6, 0x16},
    {0xc7, 0x08},
    {0xc8, 0x08},
    {0xc9, 0x00},
    {0xca, 0x20},
    {0xdc, 0x8a},
    {0xdd, 0xa0},
    {0xde, 0xa6},
    {0xdf, 0x75},

    /*AWB*/
    {0xfe, 0x01},
    {0x7c, 0x09},
    {0x65, 0x06},
    {0x7c, 0x08},
    {0x56, 0xf4},
    {0x66, 0x0f},
    {0x67, 0x84},
    {0x6b, 0x80},
    {0x6d, 0x12},
    {0x6e, 0xb0},
    {0x86, 0x00},
    {0x87, 0x00},
    {0x88, 0x00},
    {0x89, 0x00},
    {0x8a, 0x00},
    {0x8b, 0x00},
    {0x8c, 0x00},
    {0x8d, 0x00},
    {0x8e, 0x00},
    {0x8f, 0x00},
    {0x90, 0x00},
    {0x91, 0x00},
    {0x92, 0xf4},
    {0x93, 0xd5},
    {0x94, 0x50},
    {0x95, 0x0f},
    {0x96, 0xf4},
    {0x97, 0x2d},
    {0x98, 0x0f},
    {0x99, 0xa6},
    {0x9a, 0x2d},
    {0x9b, 0x0f},
    {0x9c, 0x59},
    {0x9d, 0x2d},
    {0x9e, 0xaa},
    {0x9f, 0x67},
    {0xa0, 0x59},
    {0xa1, 0x00},
    {0xa2, 0x00},
    {0xa3, 0x0a},
    {0xa4, 0x00},
    {0xa5, 0x00},
    {0xa6, 0xd4},
    {0xa7, 0x9f},
    {0xa8, 0x55},
    {0xa9, 0xd4},
    {0xaa, 0x9f},
    {0xab, 0xac},
    {0xac, 0x9f},
    {0xad, 0x55},
    {0xae, 0xd4},
    {0xaf, 0xac},
    {0xb0, 0xd4},
    {0xb1, 0xa3},
    {0xb2, 0x55},
    {0xb3, 0xd4},
    {0xb4, 0xac},
    {0xb5, 0x00},
    {0xb6, 0x00},
    {0xb7, 0x05},
    {0xb8, 0xd6},
    {0xb9, 0x8c},

    /*CC*/
    {0xfe, 0x01},
    {0xd0, 0x40},
    {0xd1, 0xf8},
    {0xd2, 0x00},
    {0xd3, 0xfa},
    {0xd4, 0x45},
    {0xd5, 0x02},

    {0xd6, 0x30},
    {0xd7, 0xfa},
    {0xd8, 0x08},
    {0xd9, 0x08},
    {0xda, 0x58},
    {0xdb, 0x02},
    {0xfe, 0x00},

    /*Gamma*/
    {0xfe, 0x00},
    {0xba, 0x00},
    {0xbb, 0x04},
    {0xbc, 0x0a},
    {0xbd, 0x0e},
    {0xbe, 0x22},
    {0xbf, 0x30},
    {0xc0, 0x3d},
    {0xc1, 0x4a},
    {0xc2, 0x5d},
    {0xc3, 0x6b},
    {0xc4, 0x7a},
    {0xc5, 0x85},
    {0xc6, 0x90},
    {0xc7, 0xa5},
    {0xc8, 0xb5},
    {0xc9, 0xc2},
    {0xca, 0xcc},
    {0xcb, 0xd5},
    {0xcc, 0xde},
    {0xcd, 0xea},
    {0xce, 0xf5},
    {0xcf, 0xff},

    /*Auto Gamma*/
    {0xfe, 0x00},
    {0x5a, 0x08},
    {0x5b, 0x0f},
    {0x5c, 0x15},
    {0x5d, 0x1c},
    {0x5e, 0x28},
    {0x5f, 0x36},
    {0x60, 0x45},
    {0x61, 0x51},
    {0x62, 0x6a},
    {0x63, 0x7d},
    {0x64, 0x8d},
    {0x65, 0x98},
    {0x66, 0xa2},
    {0x67, 0xb5},
    {0x68, 0xc3},
    {0x69, 0xcd},
    {0x6a, 0xd4},
    {0x6b, 0xdc},
    {0x6c, 0xe3},
    {0x6d, 0xf0},
    {0x6e, 0xf9},
    {0x6f, 0xff},

    /*Gain*/
    {0xfe, 0x00},
    {0x70, 0x50},

    /*AEC*/
    {0xfe, 0x00},
    {0x4f, 0x01},
    {0xfe, 0x01},
    {0x0d, 0x00},
    {0x12, 0xa0},
    {0x13, 0x3a},
    {0x44, 0x04},
    {0x1f, 0x30},
    {0x20, 0x40},
    {0x26, 0x9a},
    {0x3e, 0x20},
    {0x3f, 0x2d},
    {0x40, 0x40},
    {0x41, 0x5b},
    {0x42, 0x82},
    {0x43, 0xb7},
    {0x04, 0x0a},
    {0x02, 0x79},
    {0x03, 0xc0},

    /*measure window*/
    {0xfe, 0x01},
    {0xcc, 0x08},
    {0xcd, 0x08},
    {0xce, 0xa4},
    {0xcf, 0xec},

    /*DNDD*/
    {0xfe, 0x00},
    {0x81, 0xb8},
    {0x82, 0x12},
    {0x83, 0x0a},
    {0x84, 0x01},
    {0x86, 0x50},
    {0x87, 0x18},
    {0x88, 0x10},
    {0x89, 0x70},
    {0x8a, 0x20},
    {0x8b, 0x10},
    {0x8c, 0x08},
    {0x8d, 0x0a},

    /*Intpee*/
    {0xfe, 0x00},
    {0x8f, 0xaa},
    {0x90, 0x9c},
    {0x91, 0x52},
    {0x92, 0x03},
    {0x93, 0x03},
    {0x94, 0x08},
    {0x95, 0x44},
    {0x97, 0x00},
    {0x98, 0x00},

    /*ASDE*/
    {0xfe, 0x00},
    {0xa1, 0x30},
    {0xa2, 0x41},
    {0xa4, 0x30},
    {0xa5, 0x20},
    {0xaa, 0x30},
    {0xac, 0x32},

    /*YCP*/
    {0xfe, 0x00},
    {0xd1, 0x3c},
    {0xd2, 0x3c},
    {0xd3, 0x38},
    {0xd6, 0xf4},
    {0xd7, 0x1d},
    {0xdd, 0x73},
    {0xde, 0x84},

    /*Banding*/
    {0xfe, 0x00},
    {0x05, 0x01},
    {0x06, 0xad},
    {0x07, 0x00},
    {0x08, 0x10},

    {0xfe, 0x01},
    {0x25, 0x00},
    {0x26, 0x9a},

    {0x27, 0x01},
    {0x28, 0xce},
    {0x29, 0x02},
    {0x2a, 0x68},
    {0x2b, 0x02},
    {0x2c, 0x68},
    {0x2d, 0x07},
    {0x2e, 0xd2},
    {0x2f, 0x0b},
    {0x30, 0x6e},
    {0x31, 0x0e},
    {0x32, 0x70},
    {0x33, 0x12},
    {0x34, 0x0c},
    {0x3c, 0x30},

    /*Analog&Cisctl*/
    {0xfe, 0x00},
    {0x05, 0x01},
    {0x06, 0xa0},
    {0x07, 0x00},
    {0x08, 0x20},
    {0x0a, 0x78},
    {0x0c, 0xa0},
    {0x0d, 0x00}, //window_height [8]
    {0x0e, 0xf8}, //window_height [7:0] 248
    {0x0f, 0x01}, //window_width [9:8]
    {0x10, 0x48}, //window_width [7:0]  328

    {0x55, 0x00},
    {0x56, 0xf0}, // 240
    {0x57, 0x01},
    {0x58, 0x40}, // 320

    /*SPI*/
    {0xfe, 0x03},
    {0x5b, 0x40},
    {0x5c, 0x01},
    {0x5d, 0xf0},
    {0x5e, 0x00},

    /*AEC*/
    {0xfe, 0x01},
    {0x25, 0x00}, //step
    {0x26, 0x63},
    {0x27, 0x01},
    {0x28, 0x29},
    {0x29, 0x01},
    {0x2a, 0x29},
    {0x2b, 0x01},
    {0x2c, 0x29},
    {0x2d, 0x01},
    {0x2e, 0x29},
    {0x2f, 0x01},
    {0x30, 0x29},
    {0x31, 0x01},
    {0x32, 0x29},
    {0x33, 0x01},
    {0x34, 0x29},
    {0x3c, 0x00},

    /*measure window*/
    {0xfe, 0x01},
    {0xcc, 0x04},
    {0xcd, 0x04},
    {0xce, 0x72},
    {0xcf, 0x52},
    {0xfe, 0x00},
    ENDMARKER,  /* END MARKER */
};

static struct regval_list gc032a_regs_stream_on[] = {
    {0xfe, 0x00},
    {0xf3, 0xff},
    ENDMARKER,
};

static struct regval_list gc032a_regs_stream_off[] = {
    {0xfe, 0x00},
    {0xf3, 0x0},
    ENDMARKER,
};

static struct regval_list gc032a_chip_id_regs[] = {
    {0xfe, 0x00},
    {0xf0, 0x00},
    {0xf1, 0x00},
    ENDMARKER,
};

static int gc032a_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
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
        printk(KERN_ERR "gc032a: failed to write reg: %x\n", (int)reg);
    }

    return ret;
}

static int gc032a_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
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
        printk(KERN_ERR "gc032a(%x): failed to read reg: %x\n", i2c->addr, (int)reg);
    }

    return ret;
}

static int gc032a_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != 0xff) {
        ret = gc032a_write(i2c, vals->reg_num, vals->value);
        if (ret < 0) {
            return ret;
        }
        vals++;
    }

    return 0;
}

static inline int gc032a_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != 0xff) {
        if (vals->reg_num == 0xfe) {
            ret = gc032a_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        } else {
            ret = gc032a_read(i2c, vals->reg_num, &vals->value);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "{0x%x, 0x%x}\n", vals->reg_num, vals->value);

        vals++;
    }

    return 0;
}

static int gc032a_detect(struct i2c_client *i2c)
{
    int ret;

    ret = gc032a_read_array(i2c, gc032a_chip_id_regs);
    if (ret < 0)
        return ret;

    if (gc032a_chip_id_regs[1].value != GC032A_CHIP_ID_H) {
        printk(KERN_ERR "gc032a read chip id failed:0x%x\n", gc032a_chip_id_regs[1].value);
        return -ENODEV;
    }
    if (gc032a_chip_id_regs[2].value != GC032A_CHIP_ID_L) {
        printk(KERN_ERR "gc032a read chip id failed:0x%x\n", gc032a_chip_id_regs[2].value);
        return -ENODEV;
    }

    printk(KERN_ERR "gc032a read chip id : h = 0x%x,l = 0x%x\n", gc032a_chip_id_regs[1].value, gc032a_chip_id_regs[2].value);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "gc032a_reset");
        printk(KERN_ERR "gc032a:  request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));

        if (ret) {
            printk(KERN_ERR "gc032a: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "gc032a_pwdn");
        printk(KERN_ERR "gc032a:  request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
        if (ret) {
            printk(KERN_ERR "gc032a: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc032a_power");
        printk(KERN_ERR "gc032a:  request gc032a_power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
        if (ret) {
            printk(KERN_ERR "gc032a: failed to request pwdn pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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

static void gc032a_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

        camera_disable_sensor_mclk();
}

static int gc032a_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(GC032A_SUPPORT_SCLK);
    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(50);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        m_msleep(50);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(50);
    }

    ret = gc032a_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "gc032a: failed to detect\n");
        gc032a_power_off();
        return ret;
    }
    printk(KERN_ERR "gc032a: gc032a detect success\n");

    m_msleep(10);
    ret = gc032a_write_array(i2c_dev, gc032a_sensor_attr.sensor_info.private_init_setting);
    // ret = gc032a_read_array(i2c_dev, gc032a_sensor_attr.sensor_info.private_init_setting);
    m_msleep(10);
    if (0 != ret) {
        printk(KERN_ERR "gc032a: failed to init regs\n");
        gc032a_power_off();
        return ret;
    }

    return 0;
}

static int gc032a_stream_on(void)
{
    int ret = gc032a_write_array(i2c_dev, gc032a_regs_stream_on);
    if (ret)
        printk(KERN_ERR "gc032a: failed to stream on\n");

    return ret;
}

static void gc032a_stream_off(void)
{
    int ret = gc032a_write_array(i2c_dev, gc032a_regs_stream_off);
    if (ret)
        printk(KERN_ERR "gc032a: failed to stream on\n");
}

static int gc032a_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = gc032a_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int gc032a_s_register(struct sensor_dbg_register *reg)
{
    return gc032a_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static struct sensor_attr gc032a_sensor_attr = {

    .device_name                = GC032A_DEVICE_NAME,
    .cbus_addr                  = GC032A_DEVICE_I2C_ADDR,

    .dma_mode                   = SENSOR_DATA_DMA_MODE_RAW,    // olny dvp.data_fmt is DVP_YUV422
    .dbus_type                  = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .pclk_polarity          = POLARITY_SAMPLE_RISING,
        .hsync_polarity         = POLARITY_HIGH_ACTIVE,
        .vsync_polarity         = POLARITY_HIGH_ACTIVE,
        .img_scan_mode          = DVP_IMG_SCAN_PROGRESS,
    },

    .sensor_info = {
        .private_init_setting   = gc032a_init_regs_320_240_dvp_25fps,
        .width                  = 320,
        .height                 = 240,
        .fmt                    = SENSOR_PIXEL_FMT_YUYV8_2X8,
        .fps                    = 30 << 16 | 1,
    },

    .ops = {
        .power_on               = gc032a_power_on,
        .power_off              = gc032a_power_off,
        .stream_on              = gc032a_stream_on,
        .stream_off             = gc032a_stream_off,
        .get_register           = gc032a_g_register,
        .set_register           = gc032a_s_register,
    },
};

static int gc032a_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        goto err_init_gpio;

    ret = dvp_init_select_gpio();
    if (ret)
        goto err_dvp_select_gpio;

    ret = camera_register_sensor(&gc032a_sensor_attr);
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

static int gc032a_remove(struct i2c_client *client)
{
    camera_unregister_sensor(&gc032a_sensor_attr);
    dvp_deinit_gpio();
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id gc032a_id[] = {
    { GC032A_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gc032a_id);

static struct i2c_driver gc032a_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = GC032A_DEVICE_NAME,
    },
    .probe              = gc032a_probe,
    .remove             = gc032a_remove,
    .id_table           = gc032a_id,
};

static struct i2c_board_info sensor_gc032a_info = {
    .type               = GC032A_DEVICE_NAME,
    .addr               = GC032A_DEVICE_I2C_ADDR,
};

static __init int init_gc032a(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gc032a: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&gc032a_driver);
    if (ret) {
        printk(KERN_ERR "gc032a: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gc032a_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gc032a: failed to register i2c device\n");
        i2c_del_driver(&gc032a_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_gc032a(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&gc032a_driver);
}

module_init(init_gc032a);
module_exit(exit_gc032a);

MODULE_DESCRIPTION("x1600 gc032a driver");
MODULE_LICENSE("GPL");

