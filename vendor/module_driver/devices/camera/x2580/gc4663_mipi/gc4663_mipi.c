/*
 * gc4663.c
 *
 * Copyright (C) 2022 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Settings:
 * sboot        resolution      fps     interface              mode
 *   0          2560*1440       25        mipi_2lane           linear
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


#define GC4663_DEVICE_NAME                  "gc4663"
#define GC4663_DEVICE_I2C_ADDR              0x29
#define MCLK                                27000000
#define GC4663_CHIP_ID_H                    (0x46)
#define GC4663_CHIP_ID_L                    (0x53)
#define GC4663_REG_END                      0xffff
#define GC4663_REG_DELAY                    0x0000
#define GC4663_SUPPORT_30FPS_SCLK           (144 * 1000 * 1000)
#define GC2093_SUPPORT_30FPS_SCLK_HDR       (132000000)
#define SENSOR_OUTPUT_MIN_FPS               5
#define SENSOR_VERSION                      "H20220819a"

// #define SENSOR_WDR_MODE
static int power_gpio = -1;                 // -1;
static int reset_gpio = GPIO_PA(18);        // GPIO_PA(10);
static int pwdn_gpio = -1;                  // -1;
static int i2c_bus_num = 6;                 // 3;
static int cam_bus_num = 0;                 //0;
static int i2c_addr = 0x29;                 //0x3d;
static char *sensor_name = "gc4663";

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(i2c_addr, int, 0644);
module_param(cam_bus_num, int, 0644);
module_param(i2c_bus_num, int, 0644);
module_param(sensor_name, charp, 0644);

static struct sensor_attr gc4663_sensor_attr;
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
    unsigned char reg2b3;
    unsigned char reg2b4;
    unsigned char reg2b8;
    unsigned char reg2b9;
    unsigned char reg515;
    unsigned char reg519;
    unsigned char reg2d9;
    unsigned int gain;
};

struct again_lut gc4663_again_lut[] = {
    {0x00, 0x00, 0x0, 0x01, 0x00, 0x30, 0x1e, 0x5c, 0},   // 1.000000
    {0x01, 0x20, 0x0, 0x01, 0x0b, 0x30, 0x1e, 0x5c, 14995},   // 1.171875
    {0x02, 0x01, 0x0, 0x01, 0x19, 0x30, 0x1d, 0x5b, 31177},   // 1.390625
    {0x03, 0x21, 0x0, 0x01, 0x2a, 0x30, 0x1e, 0x5c, 47704},   // 1.656250
    {0x04, 0x02, 0x0, 0x02, 0x00, 0x30, 0x1e, 0x5c, 65536},   // 2.000000
    {0x05, 0x22, 0x0, 0x02, 0x17, 0x30, 0x1d, 0x5b, 81159},   // 2.359375
    {0x06, 0x03, 0x0, 0x02, 0x33, 0x20, 0x16, 0x54, 97243},   // 2.796875
    {0x07, 0x23, 0x0, 0x03, 0x14, 0x20, 0x17, 0x55, 113240},   // 3.312500
    {0x08, 0x04, 0x0, 0x04, 0x00, 0x20, 0x17, 0x55, 131072},   // 4.000000
    {0x09, 0x24, 0x0, 0x04, 0x2f, 0x20, 0x19, 0x57, 147007},   // 4.734375
    {0x0a, 0x05, 0x0, 0x05, 0x26, 0x20, 0x19, 0x57, 162779},   // 5.593750
    {0x0b, 0x25, 0x0, 0x06, 0x28, 0x20, 0x1b, 0x59, 178776},   // 6.625000
    {0x0c, 0x0c, 0x0, 0x08, 0x00, 0x20, 0x1d, 0x5b, 196608},   // 8.000000
    {0x0d, 0x2c, 0x0, 0x09, 0x1e, 0x20, 0x1f, 0x5d, 212543},   // 9.468750
    {0x0e, 0x0d, 0x0, 0x0b, 0x0c, 0x20, 0x21, 0x5f, 228315},   // 11.187500
    {0x0f, 0x2d, 0x0, 0x0d, 0x11, 0x20, 0x24, 0x62, 244423},   // 13.265625
    {0x10, 0x1c, 0x0, 0x10, 0x00, 0x20, 0x26, 0x64, 262144},   // 16.000000
    {0x11, 0x3c, 0x0, 0x12, 0x3d, 0x18, 0x2a, 0x68, 278158},   // 18.953125
    {0x12, 0x5c, 0x0, 0x16, 0x19, 0x18, 0x2c, 0x6a, 293916},   // 22.390625
    {0x13, 0x7c, 0x0, 0x1a, 0x22, 0x18, 0x2e, 0x6c, 309959},   // 26.531250
    {0x14, 0x9c, 0x0, 0x20, 0x00, 0x18, 0x32, 0x70, 327680},   // 32.000000
    {0x15, 0xbc, 0x0, 0x25, 0x3a, 0x18, 0x35, 0x73, 343694},   // 37.906250
    {0x16, 0xdc, 0x0, 0x2c, 0x33, 0x10, 0x36, 0x74, 359485},   // 44.796875
    {0x17, 0xfc, 0x0, 0x35, 0x05, 0x10, 0x38, 0x76, 375523},   // 53.078125
    {0x18, 0x1c, 0x1, 0x40, 0x00, 0x10, 0x3c, 0x7a, 393216},   // 64.000000
    {0x19, 0x3c, 0x1, 0x4b, 0x35, 0x10, 0x42, 0x80, 409249},   // 75.828125
};

struct again_lut gc4663_again_lut_60fps[] = {
    {0x00, 0x00, 0x0, 0x01, 0x00, 0x30, 0x28, 0x66, 0},   // 1.000000
    {0x01, 0x20, 0x0, 0x01, 0x0b, 0x30, 0x2a, 0x68, 14995},   // 1.171875
    {0x02, 0x01, 0x0, 0x01, 0x19, 0x30, 0x27, 0x65, 31177},   // 1.390625
    {0x03, 0x21, 0x0, 0x01, 0x2a, 0x30, 0x29, 0x67, 47704},   // 1.656250
    {0x04, 0x02, 0x0, 0x02, 0x00, 0x30, 0x27, 0x65, 65536},   // 2.000000
    {0x05, 0x22, 0x0, 0x02, 0x17, 0x30, 0x29, 0x67, 81159},   // 2.359375
    {0x06, 0x03, 0x0, 0x02, 0x33, 0x30, 0x28, 0x66, 97243},   // 2.796875
    {0x07, 0x23, 0x0, 0x03, 0x14, 0x30, 0x2a, 0x68, 113240},   // 3.312500
    {0x08, 0x04, 0x0, 0x04, 0x00, 0x30, 0x2a, 0x68, 131072},   // 4.000000
    {0x09, 0x24, 0x0, 0x04, 0x2f, 0x30, 0x2b, 0x69, 147007},   // 4.734375
    {0x0a, 0x05, 0x0, 0x05, 0x26, 0x30, 0x2c, 0x6a, 162779},   // 5.593750
    {0x0b, 0x25, 0x0, 0x06, 0x28, 0x30, 0x2e, 0x6c, 178776},   // 6.625000
    {0x0c, 0x06, 0x0, 0x08, 0x00, 0x30, 0x2f, 0x6d, 196608},   // 8.000000
    {0x0d, 0x26, 0x0, 0x09, 0x1e, 0x30, 0x31, 0x6f, 212543},   // 9.468750
    {0x0e, 0x46, 0x0, 0x0b, 0x0c, 0x30, 0x34, 0x72, 228315},   // 11.187500
    {0x0f, 0x66, 0x0, 0x0d, 0x11, 0x30, 0x37, 0x75, 244423},   // 13.265625
    {0x10, 0x0e, 0x0, 0x10, 0x00, 0x30, 0x3a, 0x78, 262144},   // 16.000000
    {0x11, 0x2e, 0x0, 0x12, 0x3d, 0x30, 0x3e, 0x7c, 278158},   // 18.953125
    {0x12, 0x4e, 0x0, 0x16, 0x19, 0x30, 0x41, 0x7f, 293916},   // 22.390625
    {0x13, 0x6e, 0x0, 0x1a, 0x22, 0x30, 0x45, 0x83, 309959},   // 26.531250
    {0x14, 0x1e, 0x0, 0x20, 0x00, 0x30, 0x49, 0x87, 327680},   // 32.000000
    {0x15, 0x3e, 0x0, 0x25, 0x3a, 0x30, 0x4d, 0x8b, 343694},   // 37.906250
    {0x16, 0x5e, 0x0, 0x2c, 0x33, 0x30, 0x53, 0x91, 359485},   // 44.796875
    {0x17, 0x7e, 0x0, 0x35, 0x05, 0x30, 0x5a, 0x98, 375523},   // 53.078125
    {0x18, 0x9e, 0x0, 0x40, 0x00, 0x30, 0x60, 0x9e, 393216},   // 64.000000
    {0x19, 0xbe, 0x0, 0x4b, 0x35, 0x30, 0x67, 0xa5, 409249},   // 75.828125
};

unsigned int gc4663_alloc_integration_time(unsigned int it, unsigned char shift, unsigned int *sensor_it)
{
    unsigned int expo = it >> shift;
    unsigned int isp_it = it;

    *sensor_it = expo;

    return isp_it;
}
unsigned int gc4663_alloc_integration_time_short(unsigned int it, unsigned char shift, unsigned int *sensor_it)
{
    unsigned int expo = it >> shift;
    unsigned int isp_it = it;

    *sensor_it = expo;

    return isp_it;
}

unsigned int gc4663_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = gc4663_again_lut;
    while(lut->gain <= gc4663_sensor_attr.sensor_info.max_again) {
        if(isp_gain == 0) {
            *sensor_again = 0;
            return lut[0].gain;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->index;
            return (lut - 1)->gain;
        } else {
            if((lut->gain == gc4663_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }

        lut++;
    }

    return 0;
}
unsigned int gc4663_alloc_again_short(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = gc4663_again_lut;
    while(lut->gain <= gc4663_sensor_attr.sensor_info.max_again) {
        if(isp_gain == 0) {
            *sensor_again = 0;
            return 0;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->gain;
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == gc4663_sensor_attr.sensor_info.max_again_short) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

unsigned int gc4663_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return 0;
}

#ifdef SENSOR_WDR_MODE
static struct regval_list gc4663_init_regs_2560_1440_30fps_mipi_dol[] = {
    {0x03fe,0xf0},
    {0x03fe,0x00},
    {0x0317,0x00},
    {0x0320,0x77},
    {0x0324,0xc4},
    {0x0326,0x3a},
    {0x0327,0x03},
    {0x0321,0x10},
    {0x0314,0x50},
    {0x0334,0x40},
    {0x0335,0xd1},
    {0x0336,0x63},
    {0x0337,0x82},
    {0x0315,0x33},
    {0x031c,0xce},
    {0x0287,0x18},
    {0x0084,0x00},
    {0x0087,0x50},
    {0x029d,0x08},
    {0x0290,0x00},
    {0x0340,0x06},//
    {0x0341,0x40},/*vts 0x640 === > 20fps 0x960*/
    {0x0345,0x06},
    {0x034b,0xb0},
    {0x0352,0x08},
    {0x0354,0x08},
    {0x02d1,0xc0},
    {0x023c,0x04},
    {0x0238,0xb4},
    {0x0223,0xfb},
    {0x0232,0xc4},
    {0x0279,0x53},
    {0x02d3,0x01},
    {0x0243,0x06},
    {0x02ce,0xbf},
    {0x02ee,0x30},
    {0x026f,0x70},
    {0x0257,0x09},
    {0x0211,0x02},
    {0x0219,0x09},
    {0x023f,0x2d},
    {0x0518,0x00},
    {0x0519,0x14},
    {0x0515,0x18},
    {0x02d9,0x50},
    {0x02da,0x02},
    {0x02db,0xe8},
    {0x02e6,0x20},
    {0x021b,0x10},
    {0x0252,0x22},
    {0x024e,0x22},
    {0x02c4,0x01},
    {0x021d,0x17},
    {0x024a,0x01},
    {0x02ca,0x02},
    {0x0262,0x10},
    {0x029a,0x20},
    {0x021c,0x0e},
    {0x0298,0x03},
    {0x029c,0x00},
    {0x027e,0x14},
    {0x02c2,0x10},
    {0x0540,0x20},
    {0x0546,0x01},
    {0x0548,0x01},
    {0x0544,0x01},
    {0x0242,0x36},
    {0x02c0,0x36},
    {0x02c3,0x4d},
    {0x02e4,0x10},
    {0x022e,0x00},
    {0x027b,0x3f},
    {0x0269,0x0f},
    {0x02d2,0x40},
    {0x027c,0x08},
    {0x023a,0x2e},
    {0x0245,0xce},
    {0x0530,0x3f},
    {0x0531,0x02},
    {0x0228,0x50},
    {0x02ab,0x00},
    {0x0250,0x00},
    {0x0221,0x50},
    {0x02ac,0x00},
    {0x02a5,0x02},
    {0x0260,0x0b},
    {0x0216,0x04},
    {0x0299,0x1C},
    {0x021a,0x98},
    {0x0266,0xd0},
    {0x0020,0x01},
    {0x0021,0x05},
    {0x0022,0xc0},
    {0x0023,0x08},

    {0x0098,0x10},
    {0x009a,0xb0},
    {0x02bb,0x0d},
    {0x02a3,0x02},
    {0x02a4,0x02},
    {0x021e,0x02},
    {0x024f,0x08},
    {0x028c,0x08},
    {0x0532,0x3f},
    {0x0533,0x02},
    {0x0277,0x70},//tx_width
    {0x0276,0xc0},
    {0x0239,0xc0},
    {0x0200,0x00},
    {0x0201,0x50},
    {0x0202,0x05},
    {0x0203,0x00},
    {0x0205,0xc0},
    {0x02b0,0x68},
    {0x000f,0x10},/*for flip*/
    {0x0006,0xe0},
    {0x0002,0xa9},
    {0x0004,0x00},/*bit0: dpc on/off*/
    {0x0060,0x40},
    {0x0218,0x12},
    {0x0342,0x05},
    {0x0343,0x5f},
    {0x03fe,0x10},
    {0x03fe,0x00},
    {0x0106,0x78},
    {0x0107,0x89},
    {0x0108,0x0c},
    {0x0114,0x01},
    {0x0115,0x12},
    {0x0180,0x4f},
    {0x0181,0x30},
    {0x0182,0x05},
    {0x0185,0x01},
    {0x03fe,0x10},
    {0x03fe,0x00},
    {0x0100,0x09},
    //otp
    {0x0129,0x0a},
    {0x0080,0x02},
    {0x0097,0x0a},
    {0x0098,0x10},
    {0x0099,0x05},
    {0x009a,0xb0},
    {0x0317,0x08},
    {0x0a67,0x80},
    {0x0a70,0x03},
    {0x0a82,0x00},
    {0x0a83,0x10},
    {0x0a80,0x2b},
    {0x05be,0x00},
    {0x05a9,0x01},
    {0x0313,0x80},
    {0x05be,0x01},
    {0x0317,0x00},
    {0x0a67,0x00},

    {GC4663_REG_END, 0x00},    /* END MARKER */
};
#else
static struct regval_list gc4663_init_regs_2560_1440_25fps_mipi[] = {
    /*SYSTEM*/
    {0x03fe, 0xf0},
    {0x03fe, 0x00},
    {0x0317, 0x00},
    {0x0320, 0x77},
    {0x0324, 0xc8},
    {0x0325, 0x06},
    {0x0326, 0x60},
    {0x0327, 0x03},
    {0x0334, 0x40},
    {0x0336, 0x60},
    {0x0337, 0x82},
    {0x0315, 0x25},
    {0x031c, 0xc6},
    {0x0287, 0x18},
    {0x0084, 0x00},
    {0x0087, 0x50},
    {0x029d, 0x08},
    {0x0290, 0x00},
    {0x0340, 0x07},//vts
    {0x0341, 0x80},
    {0x0345, 0x06},
    {0x034b, 0xb0},
    {0x0352, 0x08},
    {0x0354, 0x08},
    {0x02d1, 0xe0},
    {0x0223, 0xf2},
    {0x0238, 0xa4},
    {0x02ce, 0x7f},
    {0x0232, 0xc4},
    {0x02d3, 0x01},/*fwc*/
    {0x0243, 0x06},
    {0x02ee, 0x30},
    {0x026f, 0x70},
    {0x0257, 0x09},
    {0x0211, 0x02},
    {0x0219, 0x09},
    {0x023f, 0x2d},
    {0x0518, 0x00},
    {0x0519, 0x01},
    {0x0515, 0x08},
    {0x02d9, 0x3f},
    {0x02da, 0x02},
    {0x02db, 0xe8},
    {0x02e6, 0x20},
    {0x021b, 0x10},
    {0x0252, 0x22},
    {0x024e, 0x22},
    {0x02c4, 0x01},
    {0x021d, 0x17},
    {0x024a, 0x01},
    {0x02ca, 0x02},
    {0x0262, 0x10},
    {0x029a, 0x20},
    {0x021c, 0x0e},
    {0x0298, 0x03},
    {0x029c, 0x00},
    {0x027e, 0x14},
    {0x02c2, 0x10},
    {0x0540, 0x20},
    {0x0546, 0x01},
    {0x0548, 0x01},
    {0x0544, 0x01},
    {0x0242, 0x1b},
    {0x02c0, 0x1b},
    {0x02c3, 0x40},/*0x20 -> 0x40*/
    {0x02e4, 0x10},
    {0x022e, 0x00},
    {0x027b, 0x3f},
    {0x0269, 0x0f},
    {0x02d2, 0x40},
    {0x027c, 0x08},
    {0x023a, 0x2e},
    {0x0245, 0xce},
    {0x0530, 0x20},
    {0x0531, 0x02},
    {0x0228, 0x50},
    {0x02ab, 0x00},
    {0x0250, 0x00},
    {0x0221, 0x50},
    {0x02ac, 0x00},
    {0x02a5, 0x02},
    {0x0260, 0x0b},
    {0x0216, 0x04},
    {0x0299, 0x1C},
    {0x02bb, 0x0d},
    {0x02a3, 0x02},
    {0x02a4, 0x02},
    {0x021e, 0x02},
    {0x024f, 0x08},
    {0x028c, 0x08},
    {0x0532, 0x3f},
    {0x0533, 0x02},
    {0x0277, 0xc0},
    {0x0276, 0xc0},
    {0x0239, 0xc0},
    {0x0202, 0x05},
    {0x0203, 0xd0},     //long exp
    {0x0205, 0xc0},
    {0x02b0, 0x68},
    {0x0002, 0xa9},
    {0x0004, 0x00},/*bit0: dpc on/off*/
    {0x021a, 0x98},
    {0x0266, 0xa0},
    {0x0020, 0x01},
    {0x0021, 0x03},
    {0x0022, 0x00},
    {0x0023, 0x04},
    {0x0342, 0x05},//hts
    {0x0343, 0xdc},
    {0x03fe, 0x10},
    {0x03fe, 0x00},
    {0x0106, 0x78},
    {0x0108, 0x0c},
    {0x0114, 0x01},
    {0x0115, 0x12},
    {0x0180, 0x46},
    {0x0181, 0x30},
    {0x0182, 0x05},
    {0x0185, 0x01},
    {0x03fe, 0x10},
    {0x03fe, 0x00},
    {0x0100, 0x00},
    {0x000f, 0x10},/*for flip dpc*/
    /*** otp ***/
    {0x0080,0x02},
    {0x0097,0x0a},
    {0x0098,0x10},
    {0x0099,0x05},
    {0x009a,0xb0},
    {0x0317,0x08},
    {0x0a67,0x80},
    {0x0a70,0x03},
    {0x0a82,0x00},
    {0x0a83,0x10},
    {0x0a80,0x2b},
    {0x05be,0x00},
    {0x05a9,0x01},
    {0x0313,0x80},
    {0x05be,0x01},
    {0x0317,0x00},
    {0x0a67,0x00},

    {GC4663_REG_END, 0x00},    /* END MARKER */
};

#endif

static int gc4663_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
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
        printk(KERN_ERR "gc4663: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int gc4663_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
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
        printk(KERN_ERR "gc4663(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int gc4663_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != GC4663_REG_END) {
        if (vals->reg_num == GC4663_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc4663_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int gc4663_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != GC4663_REG_END) {
        if (vals->reg_num == GC4663_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc4663_read(i2c, vals->reg_num, &val);
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

static int gc4663_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = gc4663_read(i2c, 0x03f0, &h);
    if (ret < 0)
        return ret;
    if (h != GC4663_CHIP_ID_H) {
        printk(KERN_ERR "gc4663 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = gc4663_read(i2c, 0x03f1, &l);
    if (ret < 0)
        return ret;

    if (l != GC4663_CHIP_ID_L) {
        printk(KERN_ERR "gc4663 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "gc4663 get chip id = %02x%02x\n", h, l);

    return 0;
}

inline static int gc4663_set_expo( int value)
{
    int ret = 0;
    int it = value & 0xffff;
    int again = (value & 0xffff0000) >> 16;
#ifndef SENSOR_WDR_MODE
    struct again_lut *val_lut = gc4663_again_lut;
#else
    struct again_lut *val_lut = gc4663_again_lut_60fps;
#endif
    /*set integration time*/
    ret = gc4663_write(i2c_dev, 0x0203, it & 0xff);
    ret += gc4663_write(i2c_dev, 0x0202, it >> 8);

    /*set analog gain*/
    ret += gc4663_write(i2c_dev, 0x02b3, val_lut[again].reg2b3);
    ret += gc4663_write(i2c_dev, 0x02b4, val_lut[again].reg2b4);
    ret += gc4663_write(i2c_dev, 0x02b8, val_lut[again].reg2b8);
    ret += gc4663_write(i2c_dev, 0x02b9, val_lut[again].reg2b9);
    ret += gc4663_write(i2c_dev, 0x0515, val_lut[again].reg515);
    ret += gc4663_write(i2c_dev, 0x0519, val_lut[again].reg519);
    ret += gc4663_write(i2c_dev, 0x02d9, val_lut[again].reg2d9);

    /*set dpc*/
    if (again > 0x14)
        ret += gc4663_write(i2c_dev, 0x0004, 0x01);
    if (again < 0x11)
        ret += gc4663_write(i2c_dev, 0x0004, 0x00);

    if (ret < 0)
        printk("gc4663_write error  %d\n" ,__LINE__ );

    return ret;
}

inline static int gc4663_set_expo_short( int value)
{
    int ret = 0;

    return ret;
}

inline static int gc4663_set_integration_time_short( int value)
{
    int ret =0 ;
    ret = gc4663_write(i2c_dev, 0x0200, (unsigned char)((value >> 8) & 0x3f)); // long exp
    ret += gc4663_write(i2c_dev, 0x0201, (unsigned char)(value & 0xff));
    if (ret < 0)
        return ret;

    return 0;
}

inline static int gc4663_set_digital_gain( int value)
{
    return 0;
}

inline static int gc4663_get_black_pedestal( int value)
{
    return 0;
}

inline static int gc4663_set_fps( int fps)
{
    return 0;
}

static int gc4663_set_hvflip(tisp_hv_flip_t *hvflip)
{
    return 0;
}

inline static int gc4663_set_mode( int value)
{
    int ret = ISP_SUCCESS;

    return ret;
}

inline static int gc4663_set_wdr_stop( int wdr_en)
{
    int ret = 0;

    return ret;
}

inline static int gc4663_set_wdr( int wdr_en)
{
    return 0;
}

static int gc4663_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = gc4663_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;

    return ret;
}

static int gc4663_s_register(struct sensor_dbg_register *reg)
{
    return gc4663_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static void gc4663_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(cam_bus_num);
}


static int gc4663_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 27 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(10);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    ret = gc4663_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "gc4663: failed to detect\n");
        goto fail;
    }

    ret = gc4663_write_array(i2c_dev, gc4663_sensor_attr.sensor_info.private_init_setting );
    if (ret) {
        printk(KERN_ERR "gc4663: failed to init regs\n");
        goto fail;
    }

    return 0;

fail:
    gc4663_power_off();
    return ret;
}

static struct regval_list gc4663_regs_stream_on[] = {

    {0x0100,0x09},
    {GC4663_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list gc4663_regs_stream_off[] = {

    {0x0100,0x00},
    {GC4663_REG_END, 0x00},    /* END MARKER */
};


static int gc4663_stream_on(void)
{
    int ret;

    ret = gc4663_write_array(i2c_dev, gc4663_regs_stream_on);
    if (ret)
        printk(KERN_ERR "gc4663: failed to stream on\n");

    return ret;
}

static void gc4663_stream_off(void)
{
    int ret;

    ret = gc4663_write_array(i2c_dev, gc4663_regs_stream_off);
    if (ret)
        printk(KERN_ERR "gc4663: failed to stream on\n");
}

static int gc4663_set_integration_time(int value)
{
    // int ret = 0;

    // // ret += gc4663_write(i2c_dev, 0x0200, (unsigned char)((value & 0x0f) << 4)); // short exp
    // // ret = gc4663_write(i2c_dev, 0x0201, (unsigned char)((value >> 4) & 0xff));

    // ret = gc4663_write(i2c_dev, 0x0202, (unsigned char)((value >> 8) & 0x3f)); // long exp
    // ret += gc4663_write(i2c_dev, 0x0203, (unsigned char)(value & 0xff));
    // if (ret < 0)
    //     return ret;

    return 0;
}

static int gc4663_set_analog_gain(int value)
{
    // int ret = 0;
    // struct again_lut *val_lut = gc4663_again_lut_60fps;
    // /*set analog gain*/

    // ret += gc4663_write(i2c_dev, 0x02b3, val_lut[value].reg2b3);
    // ret += gc4663_write(i2c_dev, 0x02b4, val_lut[value].reg2b4);
    // ret += gc4663_write(i2c_dev, 0x02b8, val_lut[value].reg2b8);
    // ret += gc4663_write(i2c_dev, 0x02b9, val_lut[value].reg2b9);
    // ret += gc4663_write(i2c_dev, 0x0515, val_lut[value].reg515);
    // ret += gc4663_write(i2c_dev, 0x0519, val_lut[value].reg519);
    // ret += gc4663_write(i2c_dev, 0x02d9, val_lut[value].reg2d9);
    // if (ret < 0)
    //     return ret;

    return 0;
}

static struct sensor_attr gc4663_sensor_attr = {
    .device_name                = GC4663_DEVICE_NAME,
    .cbus_addr                  = GC4663_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,

    .mipi = {
        .mipi_crop = {
            .enable              = 0,
            .sensor_ctrl = {
                .hcrop_diff_en   = 0,
                .mipi_vcomp_en   = 0,
                .mipi_hcomp_en   = 0,
                .line_sync_mode  = 0,
                .work_start_flag = 0,
                .data_type_en    = 0,
                .data_type_value = MIPI_CTRL_RAW10,
                .del_start       = 0,
            },
            .hcrop_start        = 0,
            .vcrop_start        = 0,
            .output_width       = 0x7F8,
            .output_height      = 0x794,
        },
        .data_fmt               = MIPI_RAW10,
        .lanes                  = 2,
        .clk                    = 400,  /* Mbps/lane */
    },

#ifdef SENSOR_WDR_MODE
    .vc_mode                    = SENSOR_VC_MODE,
    .isp_clk_rate               = 280 * 1000 * 1000,
    .sensor_info = {
        .private_init_setting   = gc4663_init_regs_2560_1440_30fps_mipi_dol,
        .width                  = 2560,
        .height                 = 1440,
        .fmt                    = SENSOR_PIXEL_FMT_SGRBG10_1X10,
        .data_type              = SENSOR_DATA_TYPE_WDR_DOL,
        .frame_mode             = SENSOR_WDR_2_FRAME_MODE,
        .wdr_en                 = 1,
        .wdr_cache              = 2 * 3000 * 188,
        .fps                    = 30 << 16 | 1,
        .total_width            = 2750,
        .total_height           = 1600,
        .expo_fs                = 1,
        .one_line_expr_in_us    = 27,
        .min_integration_time   = 2,
        .min_integration_time_native = 2,
        .max_integration_time_native = 1504,
        .integration_time_limit = 1504,
        .min_integration_time_short = 2,
        .max_integration_time_short = 94,
        .max_integration_time   = 1504,
        .max_again              = 409249,
        .max_dgain              = 0,
        .min_fps                = 5 << 16 | 1,
        .max_fps                = 30 << 16 | 1,
    },
#else
    .vc_mode                    = SENSOR_DEFAULT_MODE,
    .isp_clk_rate               = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = gc4663_init_regs_2560_1440_25fps_mipi,
        .width                  = 2560,
        .height                 = 1440,
        .fmt                    = SENSOR_PIXEL_FMT_SGRBG10_1X10,
        .data_type              = SENSOR_DATA_TYPE_LINEAR,
        .frame_mode             = SENSOR_DEFAULT_FRAME_MODE,
        .wdr_en                 = 0,
        .fps                    = 30 << 16 | 1,
        .total_width            = 0xbb8,
        .total_height           = 0x780,

        .one_line_expr_in_us    = 30,
        .min_integration_time   = 6,
        .min_integration_time_native = 2,
        .max_integration_time_native = 1920 - 4,
        .integration_time_limit = 1920 - 4,
        .max_integration_time   = 1920 - 4,
        .max_again              = 409249,
        .max_dgain              = 0,
        .min_fps                = 5 << 16 | 1,
        .max_fps                = 30 << 16 | 1,
    },
#endif

    .ops = {
        .power_on                       = gc4663_power_on,
        .power_off                      = gc4663_power_off,
        .stream_on                      = gc4663_stream_on,
        .stream_off                     = gc4663_stream_off,
        .get_register                   = gc4663_g_register,
        .set_register                   = gc4663_s_register,
        .set_integration_time           = gc4663_set_integration_time,
        .alloc_again                    = gc4663_alloc_again,
        .set_analog_gain                = gc4663_set_analog_gain,
        .alloc_again_short              = gc4663_alloc_again_short,
        .alloc_dgain                    = gc4663_alloc_dgain,
        .set_digital_gain               = gc4663_set_digital_gain,
        .alloc_integration_time_short   = gc4663_alloc_integration_time_short,
        .set_integration_time_short     = gc4663_set_integration_time_short,
        .set_fps                        = gc4663_set_fps,
        .set_hvflip                     = gc4663_set_hvflip,
        .set_expo                       = gc4663_set_expo,
        .set_expo_short                 = gc4663_set_expo_short,
    },
};

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "gc4663_reset");
        if (ret) {
            printk(KERN_ERR "gc4663: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            return ret;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "gc4663_pwdn");
        if (ret) {
            printk(KERN_ERR "gc4663: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            if (reset_gpio != -1)
                gpio_free(reset_gpio);
            return ret;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc4663_power");
        if (ret) {
            printk(KERN_ERR "gc4663: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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

    if (reset_gpio != -1)
        gpio_free(reset_gpio);

}

static int gc4663_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    gc4663_sensor_attr.cbus_addr = i2c_addr;
    ret = camera_register_sensor(cam_bus_num, &gc4663_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }
    pr_debug("probe ok ------->gc4663\n");

    return 0;
}

static int gc4663_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &gc4663_sensor_attr);
    deinit_gpio();

    return 0;
}

static const struct i2c_device_id gc4663_id[] = {
    { "gc4663", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gc4663_id);

static struct i2c_board_info sensor_gc4663_info = {
    .type = GC4663_DEVICE_NAME,
    .addr = GC4663_DEVICE_I2C_ADDR,
};

static struct i2c_driver gc4663_driver = {
    .driver = {
        .owner    = THIS_MODULE,
        .name    = "gc4663",
    },
    .probe        = gc4663_probe,
    .remove        = gc4663_remove,
    .id_table    = gc4663_id,
};

static __init int init_gc4663(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gc4663: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    gc4663_driver.driver.name = sensor_name;
    strcpy(sensor_gc4663_info.type, sensor_name);

    if (i2c_addr != -1){
        sensor_gc4663_info.addr = i2c_addr;
    }

    int ret = i2c_add_driver(&gc4663_driver);
    if (ret) {
        printk(KERN_ERR "gc4663: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gc4663_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gc4663: failed to register i2c device\n");
        i2c_del_driver(&gc4663_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_gc4663(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&gc4663_driver);
}

module_init(init_gc4663);
module_exit(exit_gc4663);

MODULE_DESCRIPTION("A low-level driver for gc4663 sensors");
MODULE_LICENSE("GPL");