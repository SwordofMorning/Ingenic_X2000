/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * PAG7930
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera/camera_sensor.h>

#define PAG7930_DEVICE_NAME              "pag7930-mipi"
#define PAG7930_DEVICE_I2C_ADDR          0x40

#define PAG7930_CHIP_ID_H                 (0x78)
#define PAG7930_CHIP_ID_L                 (0x30)
#define PAG7930_REG_END                   0xff
#define PAG7930_REG_DELAY                 0xfe

static int power_gpio       = -1;   /* GPIO_PB(12) */
static int reset_gpio       = -1;   /* GPIO_PA(30) */
static int i2c_bus_num      = -1;   // 0

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param(i2c_bus_num, int, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr pag7930_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

/*
 * SensorName RAW_60fps
 * width=1280
 * height=800
 * SlaveID=0x40
 * mclk=24
 * avdd=
 * dovdd=
 * dvdd=
 */
static struct regval_list pag7930_init_regs_1280_800_30fps_mipi[] = {
    {0xEF, 0x00},
    {0x32, 0xFF},
    {0x33, 0x0F},
    {0x78, 0x01},
    {0x64, 0x00},
    {0x4C, 0x4A},
    {0x4D, 0xCD},
    {0x4E, 0x11},
    {0x4F, 0x00},
    {0x2B, 0x00},
    {0x27, 0xC3},
    {0x29, 0xA3},
    {0x45, 0x00},
    {0xEF, 0x01},
    {0xC4, 0x04},
    {0xC5, 0x00},
    {0xEF, 0x02},
    {0x11, 0x03},
    {0x02, 0xFF},
    {0x03, 0x08},
    {0xEF, 0x03},
    {0x3B, 0x00},
    {0x0F, 0x01},
    {0x06, 0x04},
    {0x10, 0x02},
    {0x11, 0x01},
    {0x43, 0x0E},
    {0x49, 0x06},
    {0x50, 0x07},
    {0x51, 0x00},
    {0x52, 0x14},
    {0x53, 0x00},
    {0x17, 0x07},
    {0x18, 0x05},
    {0x1B, 0x05},
    {0x1C, 0x06},
    {0x4F, 0x01},
    {0x07, 0x10},
    {0x08, 0x05},
    {0x04, 0x30},
    {0x05, 0x03},
    {0xED, 0x01},
    {0xEF, 0x04},
    {0x13, 0x01},
    {0x16, 0xF2},
    {0x17, 0xE4},
    {0x18, 0xD5},
    {0x19, 0xC5},
    {0x1A, 0xB9},
    {0x1B, 0xB4},
    {0x1C, 0xBE},
    {0x1D, 0xE5},
    {0x1E, 0x3E},
    {0x1F, 0x29},
    {0x20, 0x15},
    {0x21, 0x46},
    {0x22, 0x54},
    {0x23, 0x00},
    {0x30, 0x31},
    {0x31, 0x64},
    {0x35, 0x07},
    {0x36, 0x0B},
    {0x3C, 0x4B},
    {0x3D, 0x01},
    {0x42, 0x00},
    {0x43, 0x1D},
    {0x44, 0x00},
    // gain [1, 16]
    {0x45, 0xD6},  // 0x1d   0xe8  0x104  0x12c 0x160  0x1d6
    {0x46, 0x01},  //  1.0   8.0    8.8    10    12    16
    {0x47, 0xD0},
    {0x48, 0x07},
    {0x49, 0x00},
    {0x4A, 0xF2},
    {0x4B, 0xB1},
    {0x4C, 0x11},
    {0x2A, 0x00},
    {0xEF, 0x00},
    {0x16, 0x3C},
    {0x17, 0x78},
    {0x18, 0x02},
    {0x2F, 0x6B},
    {0x37, 0x04},
    {0x38, 0x17},
    {0x39, 0x77},
    {0x3B, 0x1D},
    {0x3F, 0x01},
    {0x66, 0x01},
    {0xCD, 0x24},
    {0xCE, 0x10},
    {0xEF, 0x01},
    {0x06, 0x00},
    {0x07, 0x3C},
    {0x0A, 0x00},
    {0x0B, 0xBF},
    {0x0F, 0x54},
    {0x11, 0x50},
    {0x13, 0x55},
    {0x0D, 0x00},
    {0x0E, 0x00},
    {0x10, 0x00},
    {0x12, 0x00},
    {0x16, 0x50},
    {0x18, 0x96},
    {0x19, 0x00},
    {0x1E, 0x81},
    {0x20, 0xC2},
    {0x21, 0x11},
    {0x31, 0x0A},
    {0x39, 0x0A},
    {0x3C, 0x00},
    {0x3D, 0x05},
    {0x3F, 0x30},
    {0x40, 0x0C},
    {0x42, 0x00},
    {0x4D, 0x02},
    {0x4E, 0x00},
    {0x4F, 0x21},
    {0x50, 0x00},
    {0x51, 0x21},
    {0x84, 0x5A},
    {0x85, 0x00},
    {0x86, 0x46},
    {0x87, 0x64},
    {0x88, 0x00},
    {0x89, 0x5A},
    {0x8A, 0x6E},
    {0x8B, 0x00},
    {0x8C, 0x5A},
    {0x8D, 0x8C},
    {0x8E, 0x00},
    {0x8F, 0x82},
    {0x9D, 0x3C},
    {0xD1, 0x92},
    {0xD2, 0x02},
    {0xEF, 0x02},
    {0x44, 0x00},
    {0x45, 0x00},
    {0x46, 0x00},
    {0x47, 0xFF},
    {0x4A, 0x00},
    {0x4B, 0x04},
    {0x5B, 0x00},
    {0x83, 0x08},
    {0x84, 0x07},
    {0x87, 0x08},
    {0x88, 0x07},
    {0x92, 0x11},
    {0x93, 0x01},
    {0x94, 0x60},
    {0x95, 0x17},
    {0xB9, 0x08},
    {0xBA, 0x0B},
    {0xBF, 0x02},
    {0xC0, 0x03},
    {0xD1, 0x39},
    {0xE3, 0xAD},
    {0xE4, 0x02},
    {0xE7, 0x70},
    {0xE8, 0x03},
    {0xEF, 0x03},
    {0x07, 0x00},
    {0x08, 0x05},
    {0x04, 0x20},
    {0x05, 0x03},
    {0xED, 0x01},
    {0xEF, 0x01},
    {0xC6, 0x80},
    {0xC7, 0x02},
    {0xC8, 0x20},
    {0xC9, 0x03},
    {0xC4, 0x08},
    {0xC2, 0x08},
    {0xCB, 0x0A},
    {0xEF, 0x02},
    {0x23, 0x20},
    {0x24, 0x03},
    {0x21, 0x80},
    {0x22, 0x02},
    {0x19, 0x2E},
    {0x1A, 0x03},
    {0x2D, 0x20},
    {0x2E, 0x03},
    {0x2F, 0x90},
    {0x30, 0x00},
    {0xEF, 0x04},
    {0x02, 0x80},
    {0x03, 0x02},
    {0x04, 0x20},
    {0x05, 0x03},
    {0xEF, 0x00},
    {0xEB, 0x80},
    {0xEF, 0x00},

    // {0x30, 0x01},
    // {0xEF, 0x00},
    // {0x36, 0x81},
    // {0x3C, 0x08},
    // {0x37, 0x65},
    // {0x66, 0x29},
    // {0x64, 0x00},
    // {0x33, 0x0F},
    // {0x30, 0x01},
    // {0x78, 0x01},
    // {0xEB, 0x80},
    {PAG7930_REG_DELAY, 3},
    {PAG7930_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list pag7930_regs_stream_on[] = {
    {0xEF, 0x00},
    {0x36, 0x81},
    {0x3C, 0x08},
    {0x37, 0x65},
    {0x66, 0x29},
    {0x64, 0x00},
    {0x33, 0x0F},
    {0x30, 0x01},
    {0x78, 0x01},
    {0xEB, 0x80},
    {PAG7930_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list pag7930_regs_stream_off[] = {
    {0xEF, 0x00},
    {0x78, 0x00},
    {0x30, 0x00},
    {0x64, 0x11},
    {0x33, 0x0B},
    {0xEE, 0x16},
    {0x66, 0x15},
    {0x37, 0x9A},
    {0x3C, 0x04},
    {0x36, 0x42},
    {0xEB, 0x80},
    {PAG7930_REG_END, 0x00},	/* END MARKER */
};

static int pag7930_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
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
        printk(KERN_ERR "pag7930: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int pag7930_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = i2c->addr,
            .flags  = 0,
            .len    = 1,
            .buf    = &reg,
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
        printk(KERN_ERR "pag7930(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int pag7930_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != PAG7930_REG_END) {
        if (vals->reg_num == PAG7930_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = pag7930_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int pag7930_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != PAG7930_REG_END) {
        if (vals->reg_num == PAG7930_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = pag7930_read(i2c, vals->reg_num, &val);
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

static int pag7930_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = pag7930_read(i2c, 0x01, &h);
    if (ret < 0)
        return ret;
    if (h != PAG7930_CHIP_ID_H) {
        printk(KERN_ERR "pag7930 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = pag7930_read(i2c, 0x00, &l);
    if (ret < 0)
        return ret;

    if (l != PAG7930_CHIP_ID_L) {
        printk(KERN_ERR "pag7930 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "pag7930 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "pag7930_reset");
        if (ret) {
            printk(KERN_ERR "pag7930: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "pag7930_power");
        if (ret) {
            printk(KERN_ERR "pag7930: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    return 0;

err_power_gpio:
    if (reset_gpio != -1)
        gpio_free(reset_gpio);
err_reset_gpio:
    return ret;
}

static void deinit_gpio(void)
{
    if (power_gpio != -1)
        gpio_free(power_gpio);

    if (reset_gpio != -1)
        gpio_free(reset_gpio);
}

static void pag7930_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 1);

    camera_disable_sensor_mclk();
}

static int pag7930_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (reset_gpio != -1) {
        gpio_direction_output(reset_gpio, 1);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(40);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(40);
    }

    ret = pag7930_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "pag7930: failed to detect\n");
       pag7930_power_off();
        return ret;
    }

    ret = pag7930_write_array(i2c_dev, pag7930_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "pag7930: failed to init regs\n");
        pag7930_power_off();
        return ret;
    }

    return 0;
}

static int pag7930_stream_on(void)
{
    int ret;

    ret = pag7930_write_array(i2c_dev, pag7930_regs_stream_on);
    if (ret)
        printk(KERN_ERR "pag7930: failed to stream on\n");

    return ret;
}

static void pag7930_stream_off(void)
{
    int ret = pag7930_write_array(i2c_dev, pag7930_regs_stream_off);
    if (ret)
        printk(KERN_ERR "pag7930: failed to stream on\n");
}

static struct sensor_attr pag7930_sensor_attr = {
    .device_name                = PAG7930_DEVICE_NAME,
    .cbus_addr                  = PAG7930_DEVICE_I2C_ADDR,

    .dma_mode                   = SENSOR_DATA_DMA_MODE_RAW, /* 是否单独提取Y数据 */
    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .lanes                  = 2,
        .clk                    = 600 * 1000 * 1000,  /* ns */
    },

    .sensor_info = {
        .private_init_setting   = pag7930_init_regs_1280_800_30fps_mipi,
        .width                  = 1280,
        .height                 = 800,
        .fmt                    = SENSOR_PIXEL_FMT_Y10_1X10,
        .fps                    = 30 << 16 | 1,
    },

    .ops = {
        .power_on               = pag7930_power_on,
        .power_off              = pag7930_power_off,
        .stream_on              = pag7930_stream_on,
        .stream_off             = pag7930_stream_off,
    },
};


static int pag7930_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int ret;

    ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(&pag7930_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int pag7930_remove(struct i2c_client *client)
{
    camera_unregister_sensor(&pag7930_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id pag7930_id[] = {
    { PAG7930_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, pag7930_id);

static struct i2c_driver pag7930_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = PAG7930_DEVICE_NAME,
    },
    .probe              = pag7930_probe,
    .remove             = pag7930_remove,
    .id_table           = pag7930_id,
};

static struct i2c_board_info sensor_pag7930_info = {
    .type               = PAG7930_DEVICE_NAME,
    .addr               = PAG7930_DEVICE_I2C_ADDR,
};

static __init int init_pag7930(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "pag7930: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&pag7930_driver);
    if (ret) {
        printk(KERN_ERR "pag7930: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_pag7930_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "pag7930: failed to register i2c device\n");
        i2c_del_driver(&pag7930_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_pag7930(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&pag7930_driver);
}

module_init(init_pag7930);
module_exit(exit_pag7930);

MODULE_DESCRIPTION("x1600 pag7930 mipi driver");
MODULE_LICENSE("GPL");
