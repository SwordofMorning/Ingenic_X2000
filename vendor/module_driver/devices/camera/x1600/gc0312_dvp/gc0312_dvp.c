/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * GC0312
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera_sensor.h>

#define GC0312_DEVICE_NAME              "gc0312-dvp"
#define GC0312_DEVICE_I2C_ADDR          0x21

#define GC0312_CHIP_ID_H                0xb3
#define GC0312_CHIP_ID_L                0x10

#define GC0312_REG_END                  0xff
#define GC0312_REG_DELAY                0xfffe

#define GC0312_SUPPORT_SCLK             (24*1000*1000)

static int power_gpio   = -1;       //
static int reset_gpio   = -1;       //PA30
static int pwdn_gpio    = -1;       //PA31
static int i2c_bus_num  = -1;       //i2c 0
static short i2c_addr   = -1;
static char *sensor_name = GC0312_DEVICE_NAME;

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, short, 0644);
module_param(sensor_name, charp, 0644);

static struct i2c_client *i2c_dev;

static struct sensor_attr gc0312_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

static struct regval_list gc0312_init_regs_640_480_dvp_30fps[] = {
    {0xfe,0xf0},
    {0xfe,0xf0},
    {0xfe,0x00},
    {0xfc,0x0e},
    {0xfc,0x0e},
    {0xf2,0x07},
    {0xf3,0x00}, // output_disable
    {0xf7,0x1b},
    {0xf8,0x04},
    {0xf9,0x0e},
    {0xfa,0x11},

    /////////////////////////////////////////////////
    /////////////////  CISCTL reg	/////////////////
    /////////////////////////////////////////////////
    {0x00,0x2f},
    {0x01,0x0f},
    {0x02,0x04},
    {0x03,0x03},
    {0x04,0x50},
    {0x09,0x00},
    {0x0a,0x00},
    {0x0b,0x00},
    {0x0c,0x04},
    {0x0d,0x01},
    {0x0e,0xe8},
    {0x0f,0x02},
    {0x10,0x88},
    {0x16,0x00},
    {0x17,0x14},
    {0x18,0x1a},
    {0x19,0x14},
    {0x1b,0x48},
    {0x1c,0x1c},
    {0x1e,0x6b},
    {0x1f,0x28},
    {0x20,0x8b},
    {0x21,0x49},
    {0x22,0xb0},
    {0x23,0x04},
    {0x24,0x16},
    {0x34,0x20},

    /////////////////////////////////////////////////
    ////////////////////   BLK	 ////////////////////
    /////////////////////////////////////////////////
    {0x26,0x23},
    {0x28,0xff},
    {0x29,0x00},
    {0x32,0x00},
    {0x33,0x10},
    {0x37,0x20},
    {0x38,0x10},
    {0x47,0x80},
    {0x4e,0x66},
    {0xa8,0x02},
    {0xa9,0x80},

    /////////////////////////////////////////////////
    //////////////////	ISP reg   ///////////////////
    /////////////////////////////////////////////////
    {0x40,0xff},
    {0x41,0x21},
    {0x42,0xcf},
    {0x44,0x02},
    {0x45,0xa8},
    {0x46,0x02}, //sync
    {0x4a,0x11},
    {0x4b,0x01},
    {0x4c,0x20},
    {0x4d,0x05},
    {0x4f,0x01},
    {0x50,0x01},
    {0x55,0x01},
    {0x56,0xe0},
    {0x57,0x02},
    {0x58,0x80},

    /////////////////////////////////////////////////
    ///////////////////   GAIN   ////////////////////
    /////////////////////////////////////////////////
    {0x70,0x70},
    {0x5a,0x84},
    {0x5b,0xc9},
    {0x5c,0xed},
    {0x77,0x74},
    {0x78,0x40},
    {0x79,0x5f},

    /////////////////////////////////////////////////
    ///////////////////   DNDD  /////////////////////
    /////////////////////////////////////////////////
    {0x82,0x14},
    {0x83,0x0b},
    {0x89,0xf0},

    /////////////////////////////////////////////////
    //////////////////   EEINTP  ////////////////////
    /////////////////////////////////////////////////
    {0x8f,0xaa},
    {0x90,0x8c},
    {0x91,0x90},
    {0x92,0x03},
    {0x93,0x03},
    {0x94,0x05},
    {0x95,0x65},
    {0x96,0xf0},

    /////////////////////////////////////////////////
    /////////////////////  ASDE  ////////////////////
    /////////////////////////////////////////////////
    {0xfe,0x00},

    {0x9a,0x20},
    {0x9b,0x80},
    {0x9c,0x40},
    {0x9d,0x80},

    {0xa1,0x30},
    {0xa2,0x32},
    {0xa4,0x80},
    {0xa5,0x28},
    {0xaa,0x30},
    {0xac,0x22},


    /////////////////////////////////////////////////
    ///////////////////   GAMMA   ///////////////////
    /////////////////////////////////////////////////
    {0xfe,0x00},//default
    {0xbf,0x08},
    {0xc0,0x16},
    {0xc1,0x28},
    {0xc2,0x41},
    {0xc3,0x5a},
    {0xc4,0x6c},
    {0xc5,0x7a},
    {0xc6,0x96},
    {0xc7,0xac},
    {0xc8,0xbc},
    {0xc9,0xc9},
    {0xca,0xd3},
    {0xcb,0xdd},
    {0xcc,0xe5},
    {0xcd,0xf1},
    {0xce,0xfa},
    {0xcf,0xff},

/*
    {0xfe,0x00},//big gamma
    {0xbf,0x08},
    {0xc0,0x1d},
    {0xc1,0x34},
    {0xc2,0x4b},
    {0xc3,0x60},
    {0xc4,0x73},
    {0xc5,0x85},
    {0xc6,0x9f},
    {0xc7,0xb5},
    {0xc8,0xc7},
    {0xc9,0xd5},
    {0xca,0xe0},
    {0xcb,0xe7},
    {0xcc,0xec},
    {0xcd,0xf4},
    {0xce,0xfa},
    {0xcf,0xff},
*/

/*
    {0xfe,0x00},//small gamma
    {0xbf,0x08},
    {0xc0,0x18},
    {0xc1,0x2c},
    {0xc2,0x41},
    {0xc3,0x59},
    {0xc4,0x6e},
    {0xc5,0x81},
    {0xc6,0x9f},
    {0xc7,0xb5},
    {0xc8,0xc7},
    {0xc9,0xd5},
    {0xca,0xe0},
    {0xcb,0xe7},
    {0xcc,0xec},
    {0xcd,0xf4},
    {0xce,0xfa},
    {0xcf,0xff},
*/
    /////////////////////////////////////////////////
    ///////////////////   YCP  //////////////////////
    /////////////////////////////////////////////////
    {0xd0,0x40},
    {0xd1,0x34},
    {0xd2,0x34},
    {0xd3,0x40},
    {0xd6,0xf2},
    {0xd7,0x1b},
    {0xd8,0x18},
    {0xdd,0x03},

    /////////////////////////////////////////////////
    ////////////////////   AEC   ////////////////////
    /////////////////////////////////////////////////
    {0xfe,0x01},
    {0x05,0x30},
    {0x06,0x75},
    {0x07,0x40},
    {0x08,0xb0},
    {0x0a,0xc5},
    {0x0b,0x11},
    {0x0c,0x00},
    {0x12,0x52},
    {0x13,0x38},
    {0x18,0x95},
    {0x19,0x96},
    {0x1f,0x20},
    {0x20,0xc0},
    {0x3e,0x40},
    {0x3f,0x57},
    {0x40,0x7d},
    {0x03,0x60},
    {0x44,0x02},

    /////////////////////////////////////////////////
    ////////////////////   AWB   ////////////////////
    /////////////////////////////////////////////////
    {0xfe,0x01},
    {0x1c,0x91},
    {0x21,0x15},
    {0x50,0x80},
    {0x56,0x04},
    {0x59,0x08},
    {0x5b,0x02},
    {0x61,0x8d},
    {0x62,0xa7},
    {0x63,0xd0},
    {0x65,0x06},
    {0x66,0x06},
    {0x67,0x84},
    {0x69,0x08},
    {0x6a,0x25},
    {0x6b,0x01},
    {0x6c,0x00},
    {0x6d,0x02},
    {0x6e,0xf0},
    {0x6f,0x80},
    {0x76,0x80},
    {0x78,0xaf},
    {0x79,0x75},
    {0x7a,0x40},
    {0x7b,0x50},
    {0x7c,0x0c},

    {0x90,0xc9},//stable AWB
    {0x91,0xbe},
    {0x92,0xe2},
    {0x93,0xc9},
    {0x95,0x1b},
    {0x96,0xe2},
    {0x97,0x49},
    {0x98,0x1b},
    {0x9a,0x49},
    {0x9b,0x1b},
    {0x9c,0xc3},
    {0x9d,0x49},
    {0x9f,0xc7},
    {0xa0,0xc8},
    {0xa1,0x00},
    {0xa2,0x00},
    {0x86,0x00},
    {0x87,0x00},
    {0x88,0x00},
    {0x89,0x00},
    {0xa4,0xb9},
    {0xa5,0xa0},
    {0xa6,0xba},
    {0xa7,0x92},
    {0xa9,0xba},
    {0xaa,0x80},
    {0xab,0x9d},
    {0xac,0x7f},
    {0xae,0xbb},
    {0xaf,0x9d},
    {0xb0,0xc8},
    {0xb1,0x97},
    {0xb3,0xb7},
    {0xb4,0x7f},
    {0xb5,0x00},
    {0xb6,0x00},
    {0x8b,0x00},
    {0x8c,0x00},
    {0x8d,0x00},
    {0x8e,0x00},
    {0x94,0x55},
    {0x99,0xa6},
    {0x9e,0xaa},
    {0xa3,0x0a},
    {0x8a,0x00},
    {0xa8,0x55},
    {0xad,0x55},
    {0xb2,0x55},
    {0xb7,0x05},
    {0x8f,0x00},
    {0xb8,0xcb},
    {0xb9,0x9b},

/*
    {0xa4,0xb9}, //default AWB
    {0xa5,0xa0},
    {0x90,0xc9},
    {0x91,0xbe},
    {0xa6,0xb8},
    {0xa7,0x95},
    {0x92,0xe6},
    {0x93,0xca},
    {0xa9,0xbc},
    {0xaa,0x95},
    {0x95,0x23},
    {0x96,0xe7},
    {0xab,0x9d},
    {0xac,0x80},
    {0x97,0x43},
    {0x98,0x24},
    {0xae,0xb7},
    {0xaf,0x9e},
    {0x9a,0x43},
    {0x9b,0x24},
    {0xb0,0xc8},
    {0xb1,0x97},
    {0x9c,0xc4},
    {0x9d,0x44},
    {0xb3,0xb7},
    {0xb4,0x7f},
    {0x9f,0xc7},
    {0xa0,0xc8},
    {0xb5,0x00},
    {0xb6,0x00},
    {0xa1,0x00},
    {0xa2,0x00},
    {0x86,0x60},
    {0x87,0x08},
    {0x88,0x00},
    {0x89,0x00},
    {0x8b,0xde},
    {0x8c,0x80},
    {0x8d,0x00},
    {0x8e,0x00},
    {0x94,0x55},
    {0x99,0xa6},
    {0x9e,0xaa},
    {0xa3,0x0a},
    {0x8a,0x0a},
    {0xa8,0x55},
    {0xad,0x55},
    {0xb2,0x55},
    {0xb7,0x05},
    {0x8f,0x05},
    {0xb8,0xcc},
    {0xb9,0x9a},
*/

    /////////////////////////////////////////////////
    ////////////////////  CC ////////////////////////
    /////////////////////////////////////////////////
    {0xfe,0x01},

    {0xd0,0x38},//skin red
    {0xd1,0x00},
    {0xd2,0x02},
    {0xd3,0x04},
    {0xd4,0x38},
    {0xd5,0x12},
/*
    {0xd0,0x38},//skin white
    {0xd1,0xfd},
    {0xd2,0x06},
    {0xd3,0xf0},
    {0xd4,0x40},
    {0xd5,0x08},
*/

/*
    {0xd0,0x38},
    {0xd1,0xf8},
    {0xd2,0x06},
    {0xd3,0xfd},
    {0xd4,0x40},
    {0xd5,0x00},
*/
    {0xd6,0x30},
    {0xd7,0x00},
    {0xd8,0x0a},
    {0xd9,0x16},
    {0xda,0x39},
    {0xdb,0xf8},

    /////////////////////////////////////////////////
    ////////////////////   LSC   ////////////////////
    /////////////////////////////////////////////////
    {0xfe,0x01},
    {0xc1,0x3c},
    {0xc2,0x50},
    {0xc3,0x00},
    {0xc4,0x40},
    {0xc5,0x30},
    {0xc6,0x30},
    {0xc7,0x10},
    {0xc8,0x00},
    {0xc9,0x00},
    {0xdc,0x20},
    {0xdd,0x10},
    {0xdf,0x00},
    {0xde,0x00},

    /////////////////////////////////////////////////
    ///////////////////  Histogram  /////////////////
    /////////////////////////////////////////////////
    {0x01,0x10},
    {0x0b,0x31},
    {0x0e,0x50},
    {0x0f,0x0f},
    {0x10,0x6e},
    {0x12,0xa0},
    {0x15,0x60},
    {0x16,0x60},
    {0x17,0xe0},

    /////////////////////////////////////////////////
    //////////////   Measure Window   ///////////////
    /////////////////////////////////////////////////
    {0xcc,0x0c},
    {0xcd,0x10},
    {0xce,0xa0},
    {0xcf,0xe6},

    /////////////////////////////////////////////////
    /////////////////   dark sun   //////////////////
    /////////////////////////////////////////////////
    {0x45,0xf7},
    {0x46,0xff},
    {0x47,0x15},
    {0x48,0x03},
    {0x4f,0x60},

    /////////////////////////////////////////////////
    ///////////////////  banding  ///////////////////
    /////////////////////////////////////////////////
    {0xfe,0x00},
    {0x05,0x02},
    {0x06,0xd1}, //HB
    {0x07,0x00},
    {0x08,0x22}, //VB
    {0xfe,0x01},
    {0x25,0x00}, //step
    {0x26,0x6a},
    {0x27,0x02}, //20fps
    {0x28,0x12},
    {0x29,0x03}, //12.5fps
    {0x2a,0x50},
    {0x2b,0x05}, //7.14fps
    {0x2c,0xcc},
    {0x2d,0x07}, //5.55fps
    {0x2e,0x74},
    {0x3c,0x20},
    {0xfe,0x00},
    /////////////////////////////////////////////////
    /////////////////////  DVP   ////////////////////
    /////////////////////////////////////////////////
    {0xfe,0x03},
    {0x01,0x00},
    {0x02,0x00},
    {0x10,0x00},
    {0x15,0x00},
    ///////////////////OUTPUT//////////////////////
    // {0xfe,0x00},
    // {0xf3,0xff}, // output_enable

    {GC0312_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list gc0312_chip_id_regs[] = {
    {0xfe, 0x00},
    {0xf0, 0x00},
    {0xf1, 0x00},
    {GC0312_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list gc0312_regs_stream_on[] = {
    {0xfe,0x00},
    {0xf3,0xff},
    {GC0312_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list gc0312_regs_stream_off[] = {
    {0xfe,0x00},
    {0xf3,0x00},
    {GC0312_REG_END, 0x00},    /* END MARKER */
};


static int gc0312_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
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
        printk(KERN_ERR "gc0312: failed to write reg: %x\n", (int)reg);
    }

    return ret;
}

static int gc0312_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
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
        printk(KERN_ERR "gc0312(%x): failed to read reg: %x\n", i2c->addr, (int)reg);
    }

    return ret;
}

static int gc0312_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != GC0312_REG_END) {
        if (vals->reg_num == GC0312_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc0312_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        // printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n", vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static inline int gc0312_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != GC0312_REG_END) {
        if (vals->reg_num == GC0312_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc0312_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
            vals->value = val;
        }
        printk(KERN_DEBUG "vals->reg_num:%x, vals->value:%x\n", vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int gc0312_detect(struct i2c_client *i2c)
{
    int ret;

    ret = gc0312_read_array(i2c, gc0312_chip_id_regs);
    if (ret < 0)
        return ret;

    if (gc0312_chip_id_regs[1].value != GC0312_CHIP_ID_H) {
        printk(KERN_ERR "gc0312 read chip id HIGH failed: 0x%.2x\n", gc0312_chip_id_regs[1].value);
        return -ENODEV;
    }

    if (gc0312_chip_id_regs[2].value != GC0312_CHIP_ID_L) {
        printk(KERN_ERR "gc0312 read chip id LOW failed: 0x%.2x\n", gc0312_chip_id_regs[2].value);
        return -ENODEV;
    }

    printk(KERN_DEBUG "gc0312 read chip id: 0x%.2x%.2x\n", gc0312_chip_id_regs[1].value, gc0312_chip_id_regs[2].value);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "gc0312_reset");
        if (ret) {
            printk(KERN_ERR "gc0312: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "gc0312_pwdn");
        if (ret) {
            printk(KERN_ERR "gc0312: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc0312_power");
        if (ret) {
            printk(KERN_ERR "gc0312: failed to request pwdn pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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

static void gc0312_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 1);

    camera_disable_sensor_mclk();
}

static int gc0312_power_on(void)
{
    int ret, retry;

    camera_enable_sensor_mclk(GC0312_SUPPORT_SCLK);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 0);
        m_msleep(10);
    }

    if (pwdn_gpio != -1) {
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(5);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(5);
    }

    if (reset_gpio != -1) {
        gpio_direction_output(reset_gpio, 1);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(5);
    }

    for (retry = 0; retry < 10; retry++) {
        ret = gc0312_detect(i2c_dev);
        if (!ret)
            break;
    }

    if (retry >= 10) {
        printk(KERN_ERR "gc0312: failed to detect\n");
        gc0312_power_off();
        return ret;
    }


    ret = gc0312_write_array(i2c_dev, gc0312_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "gc0312: failed to init regs\n");
        gc0312_power_off();
        return ret;
    }

    return 0;
}

static int gc0312_stream_on(void)
{
    int ret = gc0312_write_array(i2c_dev, gc0312_regs_stream_on);
    if (ret)
        printk(KERN_ERR "gc0312: failed to stream on\n");

    return ret;
}

static void gc0312_stream_off(void)
{
    int ret = gc0312_write_array(i2c_dev, gc0312_regs_stream_off);
    if (ret)
        printk(KERN_ERR "gc0312: failed to stream on\n");
}

static int gc0312_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = gc0312_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int gc0312_s_register(struct sensor_dbg_register *reg)
{
    return gc0312_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}


static struct sensor_attr gc0312_sensor_attr = {
    .device_name        = GC0312_DEVICE_NAME,
    .cbus_addr          = GC0312_DEVICE_I2C_ADDR,

    .dma_mode           = SENSOR_DATA_DMA_MODE_RAW,
    .dbus_type          = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .pclk_polarity  = POLARITY_SAMPLE_RISING,
        .hsync_polarity = POLARITY_HIGH_ACTIVE,
        .vsync_polarity = POLARITY_LOW_ACTIVE,
        .img_scan_mode  = DVP_IMG_SCAN_PROGRESS,
    },

    .sensor_info = {
        .private_init_setting   = gc0312_init_regs_640_480_dvp_30fps,
        .width                  = 640,
        .height                 = 480,
        .fmt                    = SENSOR_PIXEL_FMT_YUYV8_2X8,

        .fps                    = 30 << 16 | 1,
    },

    .ops = {
        .power_on               = gc0312_power_on,
        .power_off              = gc0312_power_off,
        .stream_on              = gc0312_stream_on,
        .stream_off             = gc0312_stream_off,
        .get_register           = gc0312_g_register,
        .set_register           = gc0312_s_register,
    },
};

static int gc0312_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        goto err_init_gpio;

    ret = dvp_init_select_gpio();
    if (ret)
        goto err_dvp_select_gpio;

    ret = camera_register_sensor(&gc0312_sensor_attr);
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

static int gc0312_remove(struct i2c_client *client)
{
    camera_unregister_sensor(&gc0312_sensor_attr);
    dvp_deinit_gpio();
    deinit_gpio();
    return 0;
}

static struct i2c_device_id gc0312_id[] = {
    { GC0312_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gc0312_id);

static struct i2c_driver gc0312_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = GC0312_DEVICE_NAME,
    },
    .probe              = gc0312_probe,
    .remove             = gc0312_remove,
    .id_table           = gc0312_id,
};

static struct i2c_board_info sensor_gc0312_info = {
    .type               = GC0312_DEVICE_NAME,
    .addr               = GC0312_DEVICE_I2C_ADDR,
};

static __init int init_gc0312(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gc0312: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    gc0312_driver.driver.name = sensor_name;
    strcpy(gc0312_id[0].name, sensor_name);
    strcpy(sensor_gc0312_info.type, sensor_name);
    gc0312_sensor_attr.device_name = sensor_name;

    if (i2c_addr != -1)
        sensor_gc0312_info.addr = i2c_addr;

    int ret = i2c_add_driver(&gc0312_driver);
    if (ret) {
        printk(KERN_ERR "gc0312: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gc0312_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gc0312: failed to register i2c device\n");
        i2c_del_driver(&gc0312_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_gc0312(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&gc0312_driver);
}

module_init(init_gc0312);
module_exit(exit_gc0312);

MODULE_DESCRIPTION("x1660 gc0312 driver");
MODULE_LICENSE("GPL");
