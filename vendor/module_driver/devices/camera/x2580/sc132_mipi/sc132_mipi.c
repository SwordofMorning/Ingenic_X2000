#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>


#define SC132_DEVICE_NAME              "sc132"
#define SC132_DEVICE_I2C_ADDR          0x30
#define SC132_SCLK                     (108000000)
#define SENSOR_OUTPUT_MIN_FPS          5
#define SENSOR_OUTPUT_MAX_FPS          60
#define SENSOR_USE_FPS                 60
#define SC132_HTS                      0x0535
#define SC132_VTS                      0x0546

#define SC132_DEVICE_WIDTH             1056
#define SC132_DEVICE_HEIGHT            1280

#define SC132_CHIP_ID_H                (0x01)
#define SC132_CHIP_ID_L                (0x32)
#define SC132_REG_CHIP_ID_HIGH         0x3107
#define SC132_REG_CHIP_ID_LOW          0x3108

#define SC132_REG_END                  0xffff
#define SC132_REG_DELAY                0xfffe

static int power_gpio   = -1;
static int reset_gpio   = -1;
static int pwdn_gpio    = -1;
static int i2c_bus_num  = -1;
static int i2c_addr     = SC132_DEVICE_I2C_ADDR;
static int camera_index = 0;

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(i2c_bus_num, int, 0644);


static struct i2c_client *i2c_dev;
static struct sensor_attr sc132_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct dgain_lut {
    unsigned int index;
    unsigned int reg3e06;
    unsigned int reg3e07;
    unsigned int gain;
};

struct dgain_lut sc132_dgain_lut[] = {
    {0x00, 0x00, 0x80, 0},
    {0x01, 0x00, 0x84, 2886},
    {0x02, 0x00, 0x88, 5776},
    {0x03, 0x00, 0x8c, 8494},
    {0x04, 0x00, 0x90, 11136},
    {0x05, 0x00, 0x94, 13706},
    {0x06, 0x00, 0x98, 16287},
    {0x07, 0x00, 0x9c, 18723},
    {0x08, 0x00, 0xa0, 21097},
    {0x09, 0x00, 0xa4, 23413},
    {0x0a, 0x00, 0xa8, 25746},
    {0x0b, 0x00, 0xac, 27952},
    {0x0c, 0x00, 0xb0, 30108},
    {0x0d, 0x00, 0xb4, 32216},
    {0x0e, 0x00, 0xb8, 34344},
    {0x0f, 0x00, 0xbc, 36361},
    {0x10, 0x00, 0xc0, 38335},
    {0x11, 0x00, 0xc4, 40269},
    {0x12, 0x00, 0xc8, 42225},
    {0x13, 0x00, 0xcc, 44082},
    {0x14, 0x00, 0xd0, 45903},
    {0x15, 0x00, 0xd4, 47689},
    {0x16, 0x00, 0xd8, 49499},
    {0x17, 0x00, 0xdc, 51220},
    {0x18, 0x00, 0xe0, 52910},
    {0x19, 0x00, 0xe4, 54570},
    {0x1a, 0x00, 0xe8, 56253},
    {0x1b, 0x00, 0xec, 57856},
    {0x1c, 0x00, 0xf0, 59433},
    {0x1d, 0x00, 0xf4, 60983},
    {0x1e, 0x00, 0xf8, 62557},
    {0x1f, 0x00, 0xfc, 64058},
    {0x20, 0x01, 0x80, 65535},
    {0x21, 0x01, 0x84, 68467},
    {0x22, 0x01, 0x88, 71266},
    {0x23, 0x01, 0x8c, 74029},
    {0x24, 0x01, 0x90, 76671},
    {0x25, 0x01, 0x94, 79281},
    {0x26, 0x01, 0x98, 81782},
    {0x27, 0x01, 0x9c, 84258},
    {0x28, 0x01, 0xa0, 86632},
    {0x29, 0x01, 0xa4, 88985},
    {0x2a, 0x01, 0xa8, 91245},
    {0x2b, 0x01, 0xac, 93487},
    {0x2c, 0x01, 0xb0, 95643},
    {0x2d, 0x01, 0xb4, 97785},
    {0x2e, 0x01, 0xb8, 99846},
    {0x2f, 0x01, 0xbc, 101896},
    {0x30, 0x01, 0xc0, 103870},
    {0x31, 0x01, 0xc4, 105835},
    {0x32, 0x01, 0xc8, 107730},
    {0x33, 0x01, 0xcc, 109617},
    {0x34, 0x01, 0xd0, 111438},
    {0x35, 0x01, 0xd4, 113253},
    {0x36, 0x01, 0xd8, 115006},
    {0x37, 0x01, 0xdc, 116755},
    {0x38, 0x01, 0xe0, 118445},
    {0x39, 0x01, 0xe4, 120131},
    {0x3a, 0x01, 0xe8, 121762},
    {0x3b, 0x01, 0xec, 123391},
    {0x3c, 0x01, 0xf0, 124968},
    {0x3d, 0x01, 0xf4, 126543},
    {0x3e, 0x01, 0xf8, 128068},
    {0x3f, 0x01, 0xfc, 129593},
    {0x40, 0x03, 0x80, 131070},
    {0x41, 0x03, 0x84, 133979},
    {0x42, 0x03, 0x88, 136801},
    {0x43, 0x03, 0x8c, 139542},
    {0x44, 0x03, 0x90, 142206},
    {0x45, 0x03, 0x94, 144796},
    {0x46, 0x03, 0x98, 147317},
    {0x47, 0x03, 0x9c, 149773},
    {0x48, 0x03, 0xa0, 152167},
    {0x49, 0x03, 0xa4, 154502},
    {0x4a, 0x03, 0xa8, 156780},
    {0x4b, 0x03, 0xac, 159005},
    {0x4c, 0x03, 0xb0, 161178},
    {0x4d, 0x03, 0xb4, 163303},
    {0x4e, 0x03, 0xb8, 165381},
    {0x4f, 0x03, 0xbc, 167414},
    {0x50, 0x03, 0xc0, 169405},
    {0x51, 0x03, 0xc4, 171355},
    {0x52, 0x03, 0xc8, 173265},
    {0x53, 0x03, 0xcc, 175137},
    {0x54, 0x03, 0xd0, 176973},
    {0x55, 0x03, 0xd4, 178774},
    {0x56, 0x03, 0xd8, 180541},
    {0x57, 0x03, 0xdc, 182276},
    {0x58, 0x03, 0xe0, 183980},
    {0x59, 0x03, 0xe4, 185653},
    {0x5a, 0x03, 0xe8, 187297},
    {0x5b, 0x03, 0xec, 188914},
    {0x5c, 0x03, 0xf0, 190503},
    {0x5d, 0x03, 0xf4, 192065},
    {0x5e, 0x03, 0xf8, 193603},
    {0x5f, 0x03, 0xfc, 195116},
    {0x60, 0x07, 0x80, 196605},
    {0x61, 0x07, 0x84, 199514},
    {0x62, 0x07, 0x88, 202336},
    {0x63, 0x07, 0x8c, 205077},
    {0x64, 0x07, 0x90, 207741},
    {0x65, 0x07, 0x94, 210331},
    {0x66, 0x07, 0x98, 212852},
    {0x67, 0x07, 0x9c, 215308},
    {0x68, 0x07, 0xa0, 217702},
    {0x69, 0x07, 0xa4, 220037},
    {0x6a, 0x07, 0xa8, 222315},
    {0x6b, 0x07, 0xac, 224540},
    {0x6c, 0x07, 0xb0, 226713},
    {0x6d, 0x07, 0xb4, 228838},
    {0x6e, 0x07, 0xb8, 230916},
    {0x6f, 0x07, 0xbc, 232949},
    {0x70, 0x07, 0xc0, 234940},
    {0x71, 0x07, 0xc4, 236890},
    {0x72, 0x07, 0xc8, 238800},
    {0x73, 0x07, 0xcc, 240672},
    {0x74, 0x07, 0xd0, 242508},
    {0x75, 0x07, 0xd4, 244309},
    {0x76, 0x07, 0xd8, 246076},
    {0x77, 0x07, 0xdc, 247811},
    {0x78, 0x07, 0xe0, 249515},
    {0x79, 0x07, 0xe4, 251188},
    {0x7a, 0x07, 0xe8, 252832},
    {0x7b, 0x07, 0xec, 254449},
    {0x7c, 0x07, 0xf0, 256038},
    {0x7d, 0x07, 0xf4, 257600},
    {0x7e, 0x07, 0xf8, 259138},
    {0x7f, 0x07, 0xfc, 260651},
    {0x80, 0x0f, 0x80, 262140},
};

struct again_lut {
    unsigned int index;
    unsigned char reg3e08;
    unsigned char reg3e09;
    unsigned int gain;
};

struct again_lut sc132_again_lut[] = {
    {0x00, 0x03, 0x20, 0},      //x1
    {0x01, 0x03, 0x21, 2886},
    {0x02, 0x03, 0x22, 5776},
    {0x03, 0x03, 0x23, 8494},
    {0x04, 0x03, 0x24, 11136},
    {0x05, 0x03, 0x25, 13706},
    {0x06, 0x03, 0x26, 16287},
    {0x07, 0x03, 0x27, 18723},
    {0x08, 0x03, 0x28, 21097},
    {0x09, 0x03, 0x29, 23413},
    {0x0a, 0x03, 0x2a, 25746},
    {0x0b, 0x03, 0x2b, 27952},
    {0x0c, 0x03, 0x2c, 30108},
    {0x0d, 0x03, 0x2d, 32216},
    {0x0e, 0x03, 0x2e, 34344},
    {0x0f, 0x03, 0x2f, 36361},
    {0x10, 0x03, 0x30, 38335},
    {0x11, 0x03, 0x31, 40269},
    {0x12, 0x03, 0x32, 42225},
    {0x13, 0x03, 0x33, 44082},
    {0x14, 0x03, 0x34, 45903},
    {0x15, 0x03, 0x35, 47689},
    {0x16, 0x03, 0x36, 49499},
    {0x17, 0x03, 0x37, 51220},
    {0x18, 0x03, 0x38, 52910},
    {0x19, 0x03, 0x39, 54570},
    {0x1a, 0x23, 0x20, 56253},
    {0x1b, 0x23, 0x21, 59130},
    {0x1c, 0x23, 0x22, 61970},
    {0x1d, 0x23, 0x23, 64680},
    {0x1e, 0x23, 0x24, 67360},  //x2
    {0x1f, 0x23, 0x25, 69967},
    {0x20, 0x23, 0x26, 72460},
    {0x21, 0x23, 0x27, 74932},
    {0x22, 0x23, 0x28, 77340},
    {0x23, 0x23, 0x29, 79649},
    {0x24, 0x23, 0x2a, 81942},
    {0x25, 0x23, 0x2b, 84180},
    {0x26, 0x23, 0x2c, 86329},
    {0x27, 0x23, 0x2d, 88467},
    {0x28, 0x23, 0x2e, 90522},
    {0x29, 0x23, 0x2f, 92568},
    {0x2a, 0x23, 0x30, 94571},
    {0x2b, 0x23, 0x31, 96499},
    {0x2c, 0x23, 0x32, 98421},
    {0x2d, 0x23, 0x33, 100305},
    {0x2e, 0x23, 0x34, 102121},
    {0x2f, 0x23, 0x35, 103933},
    {0x30, 0x23, 0x36, 105711},
    {0x31, 0x23, 0x37, 107427},
    {0x32, 0x23, 0x38, 109141},
    {0x33, 0x23, 0x39, 110825},
    {0x34, 0x23, 0x3a, 112451},
    {0x35, 0x23, 0x3b, 114077},
    {0x36, 0x23, 0x3c, 115648},
    {0x37, 0x23, 0x3d, 117221},
    {0x38, 0x23, 0x3e, 118768},
    {0x39, 0x23, 0x3f, 120264},
    {0x3a, 0x27, 0x20, 121762},
    {0x3b, 0x27, 0x21, 124665},
    {0x3c, 0x27, 0x22, 127505},
    {0x3d, 0x27, 0x23, 130239},
    {0x3e, 0x27, 0x24, 132895}, //x4
    {0x3f, 0x27, 0x25, 135480},
    {0x40, 0x27, 0x26, 138017},
    {0x41, 0x27, 0x27, 140467},
    {0x42, 0x27, 0x28, 142855},
    {0x43, 0x27, 0x29, 145204},
    {0x44, 0x27, 0x2a, 147477},
    {0x45, 0x27, 0x2b, 149696},
    {0x46, 0x27, 0x2c, 151864},
    {0x47, 0x27, 0x2d, 154002},
    {0x48, 0x27, 0x2e, 156075},
    {0x49, 0x27, 0x2f, 158103},
    {0x4a, 0x27, 0x30, 160106},
    {0x4b, 0x27, 0x31, 162051},
    {0x4c, 0x27, 0x32, 163956},
    {0x4d, 0x27, 0x33, 165824},
    {0x4e, 0x27, 0x34, 167672},
    {0x4f, 0x27, 0x35, 169468},
    {0x50, 0x27, 0x36, 171231},
    {0x51, 0x27, 0x37, 172962},
    {0x52, 0x27, 0x38, 174676},
    {0x53, 0x27, 0x39, 176345},
    {0x54, 0x27, 0x3a, 177986},
    {0x55, 0x27, 0x3b, 179612},
    {0x56, 0x27, 0x3c, 181197},
    {0x57, 0x27, 0x3d, 182756},
    {0x58, 0x27, 0x3e, 184290},
    {0x59, 0x27, 0x3f, 185812},
    {0x5a, 0x2f, 0x20, 187297},
    {0x5b, 0x2f, 0x21, 190212},
    {0x5c, 0x2f, 0x22, 193028},
    {0x5d, 0x2f, 0x23, 195774},
    {0x5e, 0x2f, 0x24, 198430}, //x8
    {0x5f, 0x2f, 0x25, 201026},
    {0x60, 0x2f, 0x26, 203541},
    {0x61, 0x2f, 0x27, 206002},
    {0x62, 0x2f, 0x28, 208400},
    {0x63, 0x2f, 0x29, 210729},
    {0x64, 0x2f, 0x2a, 213012},
    {0x65, 0x2f, 0x2b, 215231},
    {0x66, 0x2f, 0x2c, 217409},
    {0x67, 0x2f, 0x2d, 219528},
    {0x68, 0x2f, 0x2e, 221610},
    {0x69, 0x2f, 0x2f, 223638},
    {0x6a, 0x2f, 0x30, 225633},
    {0x6b, 0x2f, 0x31, 227586},
    {0x6c, 0x2f, 0x32, 229491},
    {0x6d, 0x2f, 0x33, 231367},
    {0x6e, 0x2f, 0x34, 233199},
    {0x6f, 0x2f, 0x35, 235003},
    {0x70, 0x2f, 0x36, 236766},
    {0x71, 0x2f, 0x37, 238504},
    {0x72, 0x2f, 0x38, 240211},
    {0x73, 0x2f, 0x39, 241880},
    {0x74, 0x2f, 0x3a, 243528},
    {0x75, 0x2f, 0x3b, 245140},
    {0x76, 0x2f, 0x3c, 246732},
    {0x77, 0x2f, 0x3d, 248291},
    {0x78, 0x2f, 0x3e, 249831},
    {0x79, 0x2f, 0x3f, 251340},
    {0x7a, 0x3f, 0x20, 252832},
    {0x7b, 0x3f, 0x21, 255741},
    {0x7c, 0x3f, 0x22, 258563},
    {0x7d, 0x3f, 0x23, 261303}, //15.859

/*     {0x7e, 0x3f, 0x24, 198430},
    {0x7f, 0x3f, 0x25, 201026},
    {0x80, 0x3f, 0x26, 203541},
    {0x81, 0x3f, 0x27, 206002},
    {0x82, 0x3f, 0x28, 208400},
    {0x83, 0x3f, 0x29, 210729},
    {0x84, 0x3f, 0x2a, 213012},
    {0x85, 0x3f, 0x2b, 215231},
    {0x86, 0x3f, 0x2c, 217409}, //19.938
    {0x87, 0x3f, 0x2d, 219528},
    {0x88, 0x3f, 0x2e, 221610},
    {0x89, 0x3f, 0x2f, 223638},
    {0x8a, 0x3f, 0x30, 225633},
    {0x8b, 0x3f, 0x31, 227586},
    {0x8c, 0x3f, 0x32, 229491},
    {0x8d, 0x3f, 0x33, 231367},
    {0x8e, 0x3f, 0x34, 233199},
    {0x8f, 0x3f, 0x35, 235003},
    {0x90, 0x3f, 0x36, 236766},
    {0x91, 0x3f, 0x37, 238504},
    {0x92, 0x3f, 0x38, 240211},
    {0x93, 0x3f, 0x39, 241880},
    {0x94, 0x3f, 0x3a, 243528},
    {0x95, 0x3f, 0x3b, 245140},
    {0x96, 0x3f, 0x3c, 246732},
    {0x97, 0x3f, 0x3d, 248291},
    {0x98, 0x3f, 0x3e, 249831},
    {0x99, 0x3f, 0x3f, 251340}, */  //29.11
};

/*
    @@1080*1280 raw8@60fps-mipi-2lanes
    Sensor revision: SC132
    Input clock frequency: 24MHz
    Image output size 1080 x 1280
    Pixel data format:  RAW8
    Frame timing and frame rate: 60fps
    Output interface and data rate: MIPI 2Lane
    HTS reg_0x320c/d = 0x535
    VTS reg_0x320e/f = 0x546
*/

static struct regval_list sc132_1080_1280_60fps_mipi_init_regs[] = {
    {0x0103, 0x01},
    {0x0100, 0x00},

    // PLL bypass
    {0x36e9, 0x80},
    {0x36f9, 0x80},

    {0x3018, 0x32}, //2lane
    {0x3019, 0x0c},
    {0x301a, 0xb4},
    {0x3031, 0x08}, //raw8
    {0x3032, 0x60},
    {0x3038, 0x44},
    {0x3207, 0x17},
    {0x3208, 0x04}, //output_w, 0x438(1080)
    {0x3209, 0x20}, //old,0x38
    {0x320c, 0xff & (SC132_HTS >> 8)},  // hts-h8
    {0x320d, 0xff & SC132_HTS},         // hts-l8
    {0x320e, 0xff & (SC132_VTS >> 8)},  // vts-h8
    {0x320f, 0xff & SC132_VTS},         // vts-l8
    {0x3250, 0xcc},
    {0x3251, 0x02},
    {0x3252, 0x09},
    {0x3253, 0x5b},
    {0x3254, 0x05},
    {0x3255, 0x3b},
    {0x3306, 0x78},
    {0x330a, 0x00},
    {0x330b, 0xc8},
    {0x330f, 0x24},
    {0x3314, 0x80},
    {0x3315, 0x40},
    {0x3317, 0xf0},
    {0x331f, 0x12},
    {0x3364, 0x00},
    {0x3385, 0x41},
    {0x3387, 0x41},
    {0x3389, 0x09},
    {0x33ab, 0x00},
    {0x33ac, 0x00},
    {0x33b1, 0x03},
    {0x33b2, 0x12},
    {0x33f8, 0x02},
    {0x33fa, 0x01},
    {0x3409, 0x08},
    {0x34f0, 0xc0},
    {0x34f1, 0x20},
    {0x34f2, 0x03},
    {0x3622, 0xf5},
    {0x3630, 0x5c},
    {0x3631, 0x80},
    {0x3632, 0xc8},
    {0x3633, 0x32},
    {0x3638, 0x2a},
    {0x3639, 0x07},
    {0x363b, 0x48},
    {0x363c, 0x83},
    {0x363d, 0x10},
    {0x36ea, 0x38},
    {0x36fa, 0x25},
    {0x36fb, 0x05},
    {0x36fd, 0x04},
    {0x3900, 0x11},
    {0x3901, 0x05},
    {0x3902, 0xc5},
    {0x3904, 0x04},
    {0x3908, 0x91},
    {0x391e, 0x00},
    {0x3e01, 0x53}, // expo_h8
    {0x3e02, 0xe0}, // expo_l8
    {0x3e03, 0x0b}, // again mode
    {0x3e08, 0x03}, // again_h8
    {0x3e09, 0x20}, // again_l8
    {0x3e0e, 0xd2},
    {0x3e14, 0xb0},
    {0x3e1e, 0x7c},
    {0x3e26, 0x20},
    {0x4418, 0x38},
    {0x4503, 0x10},
    {0x4837, 0x21},
    // {0x4800, 0x20},// mipi clk is'not series
    {0x5000, 0x0e},
    {0x540c, 0x51},
    {0x550f, 0x38},
    {0x5780, 0x67},
    {0x5784, 0x10},
    {0x5785, 0x06},
    {0x5787, 0x02},
    {0x5788, 0x00},
    {0x5789, 0x00},
    {0x578a, 0x02},
    {0x578b, 0x00},
    {0x578c, 0x00},
    {0x5790, 0x00},
    {0x5791, 0x00},
    {0x5792, 0x00},
    {0x5793, 0x00},
    {0x5794, 0x00},
    {0x5795, 0x00},
    {0x5799, 0x04},

    // PLL set
    {0x36e9, 0x20},
    {0x36f9, 0x24},

    //    [gain<2]
    {0x33fa, 0x01},
    {0x3317, 0xf0},

    //    [gain>=2]
    {0x33fa, 0x02},
    {0x3317, 0x0a},
    {0x0100, 0x00},     // stream off
    //{0x4501, 0x08},   // test
    {SC132_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list sc132_regs_stream_on[] = {
    {0x0100, 0x01},
    {SC132_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list sc132_regs_stream_off[] = {
    {0x0100, 0x00},
    {SC132_REG_END, 0x0}, /* END MARKER */
};

static int sc132_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
{
    unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
    struct i2c_msg msg = {
        .addr = i2c->addr,
        .flags  = 0,
        .len    = 3,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "sc132: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int sc132_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
{
    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr = i2c->addr,
            .flags  = 0,
            .len    = 2,
            .buf    = buf,
        },
        [1] = {
            .addr = i2c->addr,
            .flags  = I2C_M_RD,
            .len    = 1,
            .buf    = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "sc132(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int sc132_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != SC132_REG_END) {
        if (vals->reg_num == SC132_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = sc132_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        // printk(KERN_DEBUG "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static inline int sc132_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != SC132_REG_END) {
        if (vals->reg_num == SC132_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = sc132_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_DEBUG "reg = 0x%x, val = 0x%02x\n", vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int sc132_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;

    ret = sc132_read(i2c, SC132_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != SC132_CHIP_ID_H) {
        printk(KERN_ERR "sc132 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = sc132_read(i2c, SC132_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;

    if (l != SC132_CHIP_ID_L) {
        printk(KERN_ERR "sc132 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }

    printk(KERN_DEBUG "sc132 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "sc132_reset");
        if (ret) {
            printk(KERN_ERR "sc132: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "sc132_pwdn");
        if (ret) {
            printk(KERN_ERR "sc132: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "sc132_power");
        if (ret) {
            printk(KERN_ERR "sc132: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
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

static void sc132_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 0);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(camera_index);
}

static int sc132_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(camera_index, 24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 0);
        m_msleep(10);
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (pwdn_gpio != -1) {
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(10);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
    }

    if (reset_gpio != -1) {
        gpio_direction_output(reset_gpio, 1);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(30);
    }

    ret = sc132_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "sc132: failed to detect\n");
        sc132_power_off();
        return ret;
    }

    ret = sc132_write_array(i2c_dev, sc132_sensor_attr.sensor_info.private_init_setting);
    // ret += sc132_read_array(i2c_dev, sc132_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "sc132: failed to init regs\n");
        sc132_power_off();
        return ret;
    }

    return 0;
}

static int sc132_stream_on(void)
{
    int ret = sc132_write_array(i2c_dev, sc132_regs_stream_on);
    if (ret)
        printk(KERN_ERR "sc132: failed to stream on\n");

    return ret;
}

static void sc132_stream_off(void)
{
    int ret = sc132_write_array(i2c_dev, sc132_regs_stream_off);
    if (ret)
        printk(KERN_ERR "sc132: failed to stream on\n");
}

static int sc132_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = sc132_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int sc132_s_register(struct sensor_dbg_register *reg)
{
    return sc132_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xff);
}

inline static int sc132_set_expo(int value)
{

    int ret = 0;
    int it = value & 0xffff;
    int again = (value & 0xffff0000) >> 16;
    unsigned char tmp;
    unsigned short vts;
    unsigned int max_expo;

    /*set integration time*/
    it = it << 4;   // step of intgration time is 1/16

    ret = sc132_read(i2c_dev, 0x320e, &tmp);
    vts = tmp << 8;
    ret += sc132_read(i2c_dev, 0x320f, &tmp);
    if (ret < 0)
        return ret;
    vts |= tmp;
    max_expo = (vts - 8) * 16;
    if (it > max_expo)
        it = max_expo;

    ret = sc132_write(i2c_dev, 0x3e00, (unsigned char)((it >> 16) & 0x0f));
    ret += sc132_write(i2c_dev, 0x3e01, (unsigned char)((it >> 8) & 0xff));
    ret += sc132_write(i2c_dev, 0x3e02, (unsigned char)(it & 0xff));

    /*set analog gain*/
    struct again_lut *val_lut = sc132_again_lut;
    ret += sc132_write(i2c_dev, 0x3e09, val_lut[again].reg3e09);
    ret += sc132_write(i2c_dev, 0x3e08, val_lut[again].reg3e08);
    if (ret < 0) {
        printk(KERN_ERR "sc132_set_expo %d ERROR\n", value);
        return ret;
    }

    return ret;
}

inline static int sc132_set_expo_short(int value)
{
    int ret = 0;
    printk(KERN_DEBUG "sc132_set_expo_short %d\n", value);
    return ret;
}

static int sc132_set_integration_time(int value)
{
    // int ret = 0;
    // unsigned char tmp;
    // unsigned short vts;
    // unsigned int max_expo;

    // value = value << 4;

    // ret = sc132_read(i2c_dev, 0x320e, &tmp);
    // vts = tmp << 8;
    // ret += sc132_read(i2c_dev, 0x320f, &tmp);
    // if (ret < 0)
    //     return ret;
    // vts |= tmp;
    // max_expo = (vts - 8) * 16;
    // if (value > max_expo)
    //     value = max_expo;

    // ret += sc132_write(i2c_dev, 0x3e00, (unsigned char)((value >> 16) & 0x0f));
    // ret += sc132_write(i2c_dev, 0x3e01, (unsigned char)((value >> 8) & 0xff));
    // ret += sc132_write(i2c_dev, 0x3e02, (unsigned char)(value & 0xff));
    // if (ret < 0) {
    //     printk(KERN_ERR "%s error \n", __func__);
    //     return ret;
    // }

    return 0;
}

unsigned int sc132_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

    struct again_lut *lut = sc132_again_lut;

    while (lut->gain <= sc132_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = 0;
            return lut[0].gain;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut -1)->index;
            return (lut-1)->gain;
        } else {
            if ((lut->gain == sc132_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

static int sc132_set_analog_gain(int value)
{
    // int ret = 0;
    // struct again_lut *val_lut = sc132_again_lut;

    // ret = sc132_write(i2c_dev, 0x3e09, val_lut[value].reg3e09);
    // ret += sc132_write(i2c_dev, 0x3e08, val_lut[value].reg3e08);
    // if (ret < 0) {
    //     printk("sc132 set analog gain %d error\n", value);
    //     return ret;
    // }

    return 0;
}

unsigned int sc132_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    // struct dgain_lut *lut = sc132_dgain_lut;

    // while (lut->gain <= sc132_sensor_attr.sensor_info.max_dgain) {
    //     if (isp_gain <= 0) {
    //         *sensor_dgain = 0;
    //         return lut[0].gain;
    //     } else if (isp_gain < lut->gain) {
    //         *sensor_dgain = (lut -1)->index;
    //         return (lut-1)->gain;
    //     } else {
    //         if ((lut->gain == sc132_sensor_attr.sensor_info.max_dgain) && (isp_gain >= lut->gain)) {
    //             *sensor_dgain = lut->index;
    //             return lut->gain;
    //         }
    //     }
    //     lut++;
    // }

    return isp_gain;
}

static int sc132_set_digital_gain(int value)
{
    // int ret = 0;
    // struct dgain_lut *val_lut = sc132_dgain_lut;
    // /*set digital gain*/
    // ret = sc132_write(i2c_dev, 0x3e07, val_lut[value].reg3e07);
    // ret += sc132_write(i2c_dev, 0x3e06, val_lut[value].reg3e06);
    // if (ret < 0) {
    //     printk(KERN_ERR "sc132 set digital gain error\n");
    //     return ret;
    // }
    return 0;
}

static int sc132_set_fps(int fps)
{
    struct sensor_info *sensor_info = &sc132_sensor_attr.sensor_info;
    unsigned int sclk = 0;
    unsigned int hts = 0;
    unsigned int vts = 0;
    unsigned char tmp = 0;
    unsigned int newformat = 0;
    int ret = 0;

    sclk = SC132_SCLK;

     /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if (newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "sc132 fps(%d) no in range\n", fps);
        return -1;
    }

    /* hts */
    ret = sc132_read(i2c_dev, 0x320c, &tmp);
    hts = tmp;
    ret += sc132_read(i2c_dev, 0x320d, &tmp);
    if (ret < 0)
        return -1;

    hts = (hts << 8) + tmp;

    /* vts */
    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = sc132_write(i2c_dev, 0x320e, (unsigned char)((vts >> 8) & 0xff));
    ret += sc132_write(i2c_dev, 0x320f, (unsigned char)(vts & 0xff));
    if (ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr sc132_sensor_attr = {
    .device_name                = SC132_DEVICE_NAME,
    .cbus_addr                  = SC132_DEVICE_I2C_ADDR,

    .dbus_type                  = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .mipi_crop = {
            .enable = 0,
            .sensor_ctrl = {
                .hcrop_diff_en = 0,
                .mipi_vcomp_en = 0,
                .mipi_hcomp_en = 0,
                .line_sync_mode = 0,
                .work_start_flag = 0,
                .data_type_en = 0,
                .data_type_value = MIPI_CTRL_RAW8,
                .del_start = 0,
            },
            .hcrop_start = 0,
            .vcrop_start = 0,
            .output_width = SC132_DEVICE_WIDTH,
            .output_height = SC132_DEVICE_HEIGHT,
        },
        .data_fmt               = MIPI_RAW8,
        .lanes                  = 2,
        .clk                    = 400,  /* Mbps/lane */
    },

    .isp_clk_rate               = 300 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = sc132_1080_1280_60fps_mipi_init_regs,

        .width                  = SC132_DEVICE_WIDTH,
        .height                 = SC132_DEVICE_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SBGGR8_1X8,

        .fps                    = SENSOR_USE_FPS << 16 | 1,  /* 60/1 */
        .max_fps                = SENSOR_OUTPUT_MAX_FPS << 16 | 1,
        .min_fps                = SENSOR_OUTPUT_MIN_FPS << 16 | 1,
        .total_width            = SC132_HTS,
        .total_height           = SC132_VTS,

        .min_integration_time   = 2,
        .max_integration_time   = SC132_VTS - 4,
        .one_line_expr_in_us    = 15,
        .max_again              = 261303,
        .max_dgain              = 262140,
    },

    .ops = {
        .power_on               = sc132_power_on,
        .power_off              = sc132_power_off,
        .stream_on              = sc132_stream_on,
        .stream_off             = sc132_stream_off,
        .get_register           = sc132_g_register,
        .set_register           = sc132_s_register,

        .set_integration_time   = sc132_set_integration_time,
        .alloc_again            = sc132_alloc_again,
        .set_analog_gain        = sc132_set_analog_gain,
        .alloc_dgain            = sc132_alloc_dgain,
        .set_digital_gain       = sc132_set_digital_gain,
        .set_fps                = sc132_set_fps,
        .set_expo               = sc132_set_expo,
        .set_expo_short         = sc132_set_expo_short
    },
};

static int sc132_probe(struct i2c_client *client,
                       const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(camera_index, &sc132_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sc132_remove(struct i2c_client *client)
{
    camera_unregister_sensor(camera_index, &sc132_sensor_attr);
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id sc132_id[] = {
    {SC132_DEVICE_NAME, 0},
    { }
};
MODULE_DEVICE_TABLE(i2c, sc132_id);

static struct i2c_driver sc132_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = SC132_DEVICE_NAME,
    },
    .probe          = sc132_probe,
    .remove         = sc132_remove,
    .id_table       = sc132_id,
};

static struct i2c_board_info sensor_sc132_info = {
    .type           = SC132_DEVICE_NAME,
    .addr           = SC132_DEVICE_I2C_ADDR,
};

static __init int init_sc132(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "sc132: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    if (i2c_addr != -1)
        sensor_sc132_info.addr = i2c_addr;

    int ret = i2c_add_driver(&sc132_driver);
    if (ret) {
        printk(KERN_ERR "sc132: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_sc132_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "sc132: failed to register i2c device\n");
        i2c_del_driver(&sc132_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_sc132(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&sc132_driver);
}


module_init(init_sc132);
module_exit(exit_sc132);

MODULE_DESCRIPTION("X2580 SC132 driver");
MODULE_LICENSE("GPL");
