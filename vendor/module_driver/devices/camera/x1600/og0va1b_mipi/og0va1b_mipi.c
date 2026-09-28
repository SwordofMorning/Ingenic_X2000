/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * OG0VA1B
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera/camera_sensor.h>

#define OG0VA1B_DEVICE_NAME              "og0va1b-mipi"
#define OG0VA1B_DEVICE_I2C_ADDR          0x60

#define OG0VA1B_CHIP_ID_H                0xC7
#define OG0VA1B_CHIP_ID_L                0x56

#define OG0VA1B_REG_CHIP_ID_HIGH        0x300A
#define OG0VA1B_REG_CHIP_ID_LOW         0x300B

#define OG0VA1B_REG_END                  0xffff
#define OG0VA1B_REG_DELAY                0xfffe

static int power_gpio       = -1;   /* GPIO_PA(31) */
static int reset_gpio       = -1;   /* -1 */
static int pwdn_gpio        = -1;   // -1
static int i2c_bus_num      = -1;   // 0

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr og0va1b_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

/*
 * width      = 640
 * height     = 480
 * SlaveID    = 0x60
 * mclk       = 24MHz
 * avdd       = 2800mV
 * dovdd      = 1800mV
 * dvdd       = 1800mV
 */
static struct regval_list og0va1b_init_regs_640_480_240fps_mipi[] = {
    {0x0103, 0x01},
    {0x0302, 0x31},
    {0x0304, 0x01},
    {0x0305, 0xe0},
    {0x0306, 0x00},
    {0x0326, 0xd8},
    {0x3006, 0x0e},
    {0x300d, 0x08},
    {0x3018, 0xf0},
    {0x301c, 0xf0},
    {0x3020, 0x20},
    {0x3040, 0x0f},
    {0x3022, 0x01},
    {0x3107, 0x40},
    {0x3216, 0x01},
    {0x3217, 0x00},
    {0x3218, 0xc0},
    {0x3219, 0x55},
    {0x3500, 0x00},
    {0x3501, 0x01},
    {0x3502, 0xfe},
    {0x3506, 0x01},
    {0x3507, 0x50},
    {0x3508, 0x01},
    {0x3509, 0x00},
    {0x350a, 0x01},
    {0x350b, 0x00},
    {0x350c, 0x00},
    {0x3541, 0x00},
    {0x3542, 0x40},
    {0x3605, 0x90},
    {0x3606, 0x41},
    {0x3612, 0x00},
    {0x3620, 0x08},
    {0x3630, 0x17},
    {0x3631, 0x99},
    {0x3639, 0x88},
    {0x3668, 0x00},
    {0x3674, 0x00},
    {0x3677, 0x3f},
    {0x368f, 0x06},
    {0x36a2, 0x19},
    {0x36a4, 0xf1},
    {0x36a5, 0x2d},
    {0x3706, 0x30},
    {0x370d, 0x72},
    {0x3713, 0x86},
    {0x3715, 0x03},
    {0x3716, 0x00},
    {0x376d, 0x24},
    {0x3770, 0x3a},
    {0x3778, 0x00},
    {0x37a8, 0x03},
    {0x37a9, 0x00},
    {0x37df, 0x7d},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x02},
    {0x3805, 0x8f},
    {0x3806, 0x01},
    {0x3807, 0xef},
    {0x3808, 0x02},
    {0x3809, 0x80},
    {0x380a, 0x01},
    {0x380b, 0xe0},
    {0x380c, 0x01},
    {0x380d, 0x78},
    {0x380e, 0x02},
    {0x380f, 0x0c},
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
    {0x3820, 0x40},
    {0x3821, 0x00},
    {0x3823, 0x00},
    {0x3826, 0x00},
    {0x3827, 0x00},
    {0x382b, 0x52},
    {0x384a, 0xa2},
    {0x3858, 0x00},
    {0x3859, 0x00},
    {0x3860, 0x00},
    {0x3861, 0x00},
    {0x3866, 0x0c},
    {0x3867, 0x07},
    {0x3884, 0x00},
    {0x3885, 0x08},
    {0x3888, 0x50},
    {0x3893, 0x6c},
    {0x3898, 0x00},
    {0x389a, 0x04},
    {0x389b, 0x01},
    {0x389c, 0x0b},
    {0x389d, 0xdc},
    {0x389f, 0x08},
    {0x38a0, 0x00},
    {0x38a1, 0x00},
    {0x38b1, 0x04},
    {0x38b2, 0x00},
    {0x38b3, 0x08},
    {0x38c1, 0x46},
    {0x38c9, 0x02},
    {0x38d4, 0x06},
    {0x38d5, 0x5a},
    {0x38d6, 0x08},
    {0x38d7, 0x3a},
    {0x391e, 0x00},
    {0x391f, 0x00},
    {0x3920, 0xa5},
    {0x3921, 0x00},
    {0x3922, 0x00},
    {0x3923, 0x00},
    {0x3924, 0x05},
    {0x3925, 0x00},
    {0x3926, 0x00},
    {0x3927, 0x00},
    {0x3928, 0x1a},
    {0x3929, 0x01},
    {0x392a, 0xb4},
    {0x392b, 0x00},
    {0x392c, 0x10},
    {0x392f, 0x40},
    {0x3a06, 0x06},
    {0x3a07, 0x78},
    {0x3a08, 0x08},
    {0x3a09, 0x80},
    {0x3a52, 0x00},
    {0x3a53, 0x01},
    {0x3a54, 0x0c},
    {0x3a55, 0x04},
    {0x3a58, 0x0c},
    {0x3a59, 0x04},
    {0x4000, 0xcf},
    {0x4003, 0x40},
    {0x4008, 0x04},
    {0x4009, 0x13},
    {0x400a, 0x02},
    {0x400b, 0x34},
    {0x4010, 0x71},
    {0x4042, 0xc3},
    {0x4306, 0x04},
    {0x4307, 0x12},
    {0x4500, 0x70},
    {0x4509, 0x00},
    {0x450b, 0x83},
    {0x4604, 0x68},
    {0x481b, 0x44},
    {0x481f, 0x30},
    {0x4823, 0x44},
    {0x4825, 0x35},
    {0x4837, 0x11},
    {0x4f00, 0x04},
    {0x4f10, 0x04},
    {0x4f21, 0x01},
    {0x4f22, 0x00},
    {0x4f23, 0x54},
    {0x4f24, 0x51},
    {0x4f25, 0x41},
    {0x5000, 0x3f},
    {0x5001, 0x80},
    {0x500a, 0x00},
    {0x5100, 0x00},
    {0x5111, 0x20},
    {0x0100, 0x00},
    {OG0VA1B_REG_END, 0x00},    /* END MARKER */

};

static struct regval_list og0va1b_regs_stream_on[] = {
    {0x0100, 0x01},
   // {OG0VA1B_REG_DELAY, 0x17},
    {OG0VA1B_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list og0va1b_regs_stream_off[] = {
    {0x0100, 0x00},
    {OG0VA1B_REG_END, 0x00},  /* END MARKER */
};

static int og0va1b_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "og0va1b: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int og0va1b_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "og0va1b(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int og0va1b_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != OG0VA1B_REG_END) {
        if (vals->reg_num == OG0VA1B_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = og0va1b_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int og0va1b_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != OG0VA1B_REG_END) {
        if (vals->reg_num == OG0VA1B_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = og0va1b_read(i2c, vals->reg_num, &val);
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

static int og0va1b_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = og0va1b_read(i2c, OG0VA1B_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != OG0VA1B_CHIP_ID_H) {
        printk(KERN_ERR "og0va1b read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = og0va1b_read(i2c, OG0VA1B_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;

    if (l != OG0VA1B_CHIP_ID_L) {
        printk(KERN_ERR "og0va1b read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "og0va1b get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "og0va1b_reset");
        if (ret) {
            printk(KERN_ERR "og0va1b: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "og0va1b_pwdn");
        if (ret) {
            printk(KERN_ERR "og0va1b: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "og0va1b_power");
        if (ret) {
            printk(KERN_ERR "og0va1b: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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

static void og0va1b_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk();
}

static int og0va1b_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 0);
        m_msleep(5);
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 0);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(5);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
    }


    ret = og0va1b_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "og0va1b: failed to detect\n");
       og0va1b_power_off();
        return ret;
    }

    ret = og0va1b_write_array(i2c_dev, og0va1b_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "og0va1b: failed to init regs\n");
        og0va1b_power_off();
        return ret;
    }
    ret = og0va1b_read_array(i2c_dev, og0va1b_sensor_attr.sensor_info.private_init_setting);

    return 0;
}

static int og0va1b_stream_on(void)
{
    int ret;

    ret = og0va1b_write_array(i2c_dev, og0va1b_regs_stream_on);
    if (ret)
        printk(KERN_ERR "og0va1b: failed to stream on\n");

    return ret;
}

static void og0va1b_stream_off(void)
{
    int ret = og0va1b_write_array(i2c_dev, og0va1b_regs_stream_off);
    if (ret)
        printk(KERN_ERR "og0va1b: failed to stream on\n");
}

static struct sensor_attr og0va1b_sensor_attr = {
    .device_name                = OG0VA1B_DEVICE_NAME,
    .cbus_addr                  = OG0VA1B_DEVICE_I2C_ADDR,

    // .dma_mode                   = SENSOR_DATA_DMA_MODE_GREY, /* 是否单独提取Y数据 */
    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .lanes                  = 1,
        .clk                    = 300 * 1000 * 1000,  /* Hz */
    },

    .sensor_info = {
        .private_init_setting   = og0va1b_init_regs_640_480_240fps_mipi,
        .width                  = 640,
        .height                 = 480,
        .fmt                    = SENSOR_PIXEL_FMT_SRGGB10_1X10,

        .fps                    = 240 << 16 | 1,
    },

    .ops = {
        .power_on               = og0va1b_power_on,
        .power_off              = og0va1b_power_off,
        .stream_on              = og0va1b_stream_on,
        .stream_off             = og0va1b_stream_off,
    },
};


static int og0va1b_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int ret;

    ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(&og0va1b_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int og0va1b_remove(struct i2c_client *client)
{
    camera_unregister_sensor(&og0va1b_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id og0va1b_id[] = {
    { OG0VA1B_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, og0va1b_id);

static struct i2c_driver og0va1b_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = OG0VA1B_DEVICE_NAME,
    },
    .probe              = og0va1b_probe,
    .remove             = og0va1b_remove,
    .id_table           = og0va1b_id,
};

static struct i2c_board_info sensor_og0va1b_info = {
    .type               = OG0VA1B_DEVICE_NAME,
    .addr               = OG0VA1B_DEVICE_I2C_ADDR,
};

static __init int init_og0va1b(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "og0va1b: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&og0va1b_driver);
    if (ret) {
        printk(KERN_ERR "og0va1b: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_og0va1b_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "og0va1b: failed to register i2c device\n");
        i2c_del_driver(&og0va1b_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_og0va1b(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&og0va1b_driver);
}

module_init(init_og0va1b);
module_exit(exit_og0va1b);

MODULE_DESCRIPTION("x1600 og0va1b mipi driver");
MODULE_LICENSE("GPL");
