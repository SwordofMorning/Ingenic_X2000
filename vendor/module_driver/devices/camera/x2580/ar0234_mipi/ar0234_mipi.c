#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/regulator/consumer.h>
#include <camera-double/hal/camera_sensor.h>


#define AR0234_DEVICE_NAME              "ar0234"
#define AR0234_DEVICE_I2C_ADDR          0x10
#define SENSOR_OUTPUT_MIN_FPS           5
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_USE_FPS                  30
#define AR0234_HTS                      0x0264
#define AR0234_VTS                      0x04C4 * 2  // 0x4c4是60fps
#define AR0234_SCLK                     (AR0234_HTS * AR0234_VTS * SENSOR_USE_FPS)

#define AR0234_DEVICE_WIDTH             1920
#define AR0234_DEVICE_HEIGHT            1200

#define AR0234_CHIP_ID                  (0x0a56)
#define AR0234_REG_CHIP_ID              0x3000

#define AR0234_REG_END                  0xffff
#define AR0234_REG_DELAY                0xfffe

static int power_gpio       = -1;
static int reset_gpio       = -1;   //pa08
static int i2c_bus_num      = -1;
static int i2c_addr         = -1;
static int camera_index     = 0;
static char *sensor_name    = NULL;
static char *regulator_name = "";

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, int, 0644);
module_param(sensor_name, charp, 0644);
module_param(regulator_name, charp, 0644);


static struct i2c_client *i2c_dev;
static struct sensor_attr ar0234_attr;
static struct regulator *ar0234_regulator = NULL;

struct regval_list {
    unsigned short reg_num;
    unsigned short value;
};

struct again_lut {
    unsigned short value;
    unsigned int gain;
};

struct again_lut ar0234_again_lut[] = {
    {0x0008, 0},      //x1
    {0x0010, 65535},  //x2
    {0x0018, 103870},
    {0x0020, 131070},
    {0x0028, 152167},
    {0x0030, 169405},
    {0x0038, 183980},
    {0x0040, 196605},
    {0x0048, 207741},
    {0x0050, 217702},
    {0x0058, 226713},
    {0x0060, 234940},
    {0x0068, 242508},
    {0x0070, 249515},
    {0x0078, 256038},
    {0x007f, 262140}, //x16
};

/*
    @@1920*1200 raw8@30fps-mipi-2lanes
    Sensor revision: AR0234
    Input clock frequency: 24MHz
    Image output size 1920 x 1200
    Pixel data format:  RAW8
    Frame timing and frame rate: 30fps
    Output interface and data rate: MIPI 2Lane
    HTS reg_0x300c
    VTS reg_0x300a
*/

static struct regval_list ar0234_1920_1200_30fps_mipi_init_regs[] = {
    {AR0234_REG_DELAY, 0x0258}, // The delay 600 ms is required!!!
    {0x301A, 0x2058}, // RESET_REGISTER
    {0x302A, 0x0008},   //VT_PIX_CLK_DIV = 8
    {0x302C, 0x0001},   //VT_SYS_CLK_DIV = 1
    {0x302E, 0x0001},   //PRE_PLL_CLK_DIV = 1
    {0x3030, 0x001E},   //PLL_MULTIPLIER = 30
    {0x3036, 0x0008},   //OP_PIX_CLK_DIV = 8
    {0x3038, 0x0002},   //OP_SYS_CLK_DIV = 2
    {0x30B0, 0x0028},   //BITFIELD = 0x30B0, 0x4000, 0x0000	//DIGITAL_TEST, bits 0x4000 = 0
    {0x31B0, 0x0080},   //FRAME_PREAMBLE = 128
    {0x31B2, 0x005C},   //LINE_PREAMBLE = 92
    {0x31B4, 0x5248},
    {0x31B6, 0x4258},
    {0x31B8, 0x904C},
    {0x31BA, 0x028B},
    {0x31BC, 0x8D89},
    {0x3354, 0x002A},   //MIPI 8 bits
    {0x31AE, 0x0202},   //mipi 2 lane
    {0x3002, 0x0008},   //Y_ADDR_START = 8
    {0x3004, 0x0008},   //X_ADDR_START = 8
    {0x3006, 0x04B7},   //Y_ADDR_END = 1207
    {0x3008, 0x0787},   //X_ADDR_END = 1927
    {0x300A, AR0234_VTS},
    {0x300C, AR0234_HTS},
    {0x3012, 0x016F},   //integration time(unit: line)
    {0x31AC, 0x0808},   // 8 bits
    {0x306E, 0x9010},           //DATAPATH_SELECT = 36880
    {0x30A2, 0x0001},           //X_ODD_INC = 1
    {0x30A6, 0x0001},           //Y_ODD_INC = 1
    {0x3082, 0x0003},           //OPERATION_MODE_CTRL = 3
    {0x3040, 0x0000},   //READ_MODE = 0（flip/mirror/binning  ctrl）
    {0x31D0, 0x0000},           //COMPANDING = 0
    {0x3088, 0x8050},           // SEQ_CTRL_PORT
    {0x3086, 0x9237},           // SEQ_DATA_PORT
    {0x3096, 0x0280},           // ROW_NOISE_ADJUST_TOP
    {0x31E0, 0x0003},           // bad PIX_DEF_EN
    {0x30B0, 0x0028},           // DIGITAL_TEST
    {0x3F4C, 0x121F},           // PIX_DEF_1D_DDC_LO_DEF
    {0x3F4E, 0x121F},           // PIX_DEF_1D_DDC_HI_DEF
    {0x3F50, 0x0B81},           // PIX_DEF_1D_DDC_EDGE
    {0x3780, 0x0000}, // LSC_ENABLE
    {0x3600, 0x00D0},
    {0x3602, 0xEE6A},
    {0x3604, 0x5D71},
    {0x3606, 0x584B},
    {0x3608, 0x30EF},
    {0x360A, 0x00B0},
    {0x360C, 0xF00A},
    {0x360E, 0x5DB1},
    {0x3610, 0x24AB},
    {0x3612, 0x284F},
    {0x3614, 0x00B0},
    {0x3616, 0x91AA},
    {0x3618, 0x6611},
    {0x361A, 0xA76D},
    {0x361C, 0x730F},
    {0x361E, 0x00D0},
    {0x3620, 0xC16A},
    {0x3622, 0x65B1},
    {0x3624, 0xAB2D},
    {0x3626, 0x7EEF},
    {0x3640, 0x4669},
    {0x3642, 0x11EC},
    {0x3644, 0xA48F},
    {0x3646, 0x192D},
    {0x3648, 0x7910},
    {0x364A, 0x4CE9},
    {0x364C, 0x07EC},
    {0x364E, 0x9B2F},
    {0x3650, 0x490D},
    {0x3652, 0x6290},
    {0x3654, 0x1B48},
    {0x3656, 0x88AD},
    {0x3658, 0x9D8F},
    {0x365A, 0x4B6B},
    {0x365C, 0x0331},
    {0x365E, 0x7328},
    {0x3660, 0x894D},
    {0x3662, 0xB2AF},
    {0x3664, 0x27EB},
    {0x3666, 0x18F1},
    {0x3680, 0x3CF1},
    {0x3682, 0xC790},
    {0x3684, 0x22F1},
    {0x3686, 0x226F},
    {0x3688, 0xC213},
    {0x368A, 0x3DF1},
    {0x368C, 0xC170},
    {0x368E, 0x0E71},
    {0x3690, 0x7BCE},
    {0x3692, 0xAD33},
    {0x3694, 0x3DF1},
    {0x3696, 0xDC2F},
    {0x3698, 0x29B1},
    {0x369A, 0xA66F},
    {0x369C, 0xCFB3},
    {0x369E, 0x3DB1},
    {0x36A0, 0xD40F},
    {0x36A2, 0x3531},
    {0x36A4, 0xF2AF},
    {0x36A6, 0xE393},
    {0x36C0, 0xCACD},
    {0x36C2, 0xED8E},
    {0x36C4, 0x7392},
    {0x36C6, 0x5D30},
    {0x36C8, 0xF994},
    {0x36CA, 0xD16D},
    {0x36CC, 0xABEE},
    {0x36CE, 0x6A72},
    {0x36D0, 0x240F},
    {0x36D2, 0xF234},
    {0x36D4, 0x886E},
    {0x36D6, 0xADCE},
    {0x36D8, 0x25D2},
    {0x36DA, 0x7010},
    {0x36DC, 0xBA14},
    {0x36DE, 0xACEE},
    {0x36E0, 0xDBEE},
    {0x36E2, 0x6A12},
    {0x36E4, 0x3AD1},
    {0x36E6, 0x8095},
    {0x3700, 0x0271},
    {0x3702, 0x786E},
    {0x3704, 0x2053},
    {0x3706, 0x0D54},
    {0x3708, 0xF3B7},
    {0x370A, 0x6810},
    {0x370C, 0xBA2E},
    {0x370E, 0x5913},
    {0x3710, 0x21D4},
    {0x3712, 0x8198},
    {0x3714, 0x7DD0},
    {0x3716, 0x8BD1},
    {0x3718, 0x4093},
    {0x371A, 0x64B4},
    {0x371C, 0xFB17},
    {0x371E, 0x7C10},
    {0x3720, 0xA5D1},
    {0x3722, 0x4433},
    {0x3724, 0x7A54},
    {0x3726, 0xFDF7},
    {0x3782, 0x03D8},
    {0x3784, 0x0250},
    {0x37C0, 0x1686},
    {0x37C2, 0x2588},
    {0x37C4, 0x7408},
    {0x37C6, 0x41C8},
    {0x3780, 0x0000}, // LSC_ENABLE
    {0x3ED2, 0xFA96},       // DAC_LD_6_7
    {0x3180, 0x824F},       // DELTA_DK_CONTROL
    {0x3ECC, 0x0D42},       // DAC_LD_0_1
    {0x3ECC, 0x0D42},       // DAC_LD_0_1
    {0x30F0, 0x2283},       // ADC_COMMAND1_HS
    {0x3102, 0x5000}, // AE_LUMA_TARGET_REG
    {0x3060, 0x000D}, // ANALOG_GAIN
    {0x30BA, 0x7622}, // DIGITAL_CTRL
    // {0x301A, 0x205C}, // RESET_REGISTER
    {AR0234_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list ar0234_regs_stream_on[] = {
    {0x301A, 0x205C}, // RESET_REGISTER
    {AR0234_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list ar0234_regs_stream_off[] = {
    {0x301A, 0x2058}, // RESET_REGISTER
    {AR0234_REG_END, 0x0}, /* END MARKER */
};

static int ar0234_write(struct i2c_client *i2c, unsigned short reg, unsigned short value)
{
    unsigned char buf[4] = {reg >> 8, reg & 0xff, value >> 8, value & 0xff};
    struct i2c_msg msg = {
        .addr = i2c->addr,
        .flags  = 0,
        .len    = 4,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "ar0234: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int ar0234_read(struct i2c_client *i2c, unsigned short reg, unsigned short *value)
{
    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    unsigned char buf2[2] = {0};

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
            .len    = 2,
            .buf    = buf2,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "ar0234(%x): failed to read reg: %x\n", i2c->addr, (int)reg);
    else
        *value = (buf2[0] << 8) | buf2[1];

    return ret;
}

static int ar0234_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != AR0234_REG_END) {
        if (vals->reg_num == AR0234_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = ar0234_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        // printk(KERN_DEBUG "vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static inline int ar0234_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned short val;
    while (vals->reg_num != AR0234_REG_END) {
        if (vals->reg_num == AR0234_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = ar0234_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_DEBUG "reg = 0x%04x, val = 0x%04x\n", vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int ar0234_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned short id = 1;

    ret = ar0234_read(i2c, AR0234_REG_CHIP_ID, &id);
    if (ret < 0)
        return ret;
    if (id != AR0234_CHIP_ID) {
        printk(KERN_ERR "ar0234 read chip id 0x%x != CHIP_ID 0x%x\n", id, AR0234_CHIP_ID);
        return -ENODEV;
    }

    printk(KERN_DEBUG "ar0234 get chip id = %04x\n", id);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "ar0234_reset");
        if (ret) {
            printk(KERN_ERR "ar0234: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ar0234_power");
        if (ret) {
            printk(KERN_ERR "ar0234: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        ar0234_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(ar0234_regulator)) {
            ret = -1;
            printk(KERN_ERR "regulator_get error!\n");
            goto err_regulator;
        }
    }

    return 0;

err_regulator:
    if (power_gpio != -1)
        gpio_free(power_gpio);
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

    if (ar0234_regulator)
        regulator_put(ar0234_regulator);
}

static void ar0234_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (ar0234_regulator)
        regulator_disable(ar0234_regulator);

    camera_disable_sensor_mclk(camera_index);

    // printk("%s, %x\n", __func__, ar0234_attr.cbus_addr);
}

static int ar0234_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(camera_index, 24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(10);
    }

    if (ar0234_regulator) {
        regulator_enable(ar0234_regulator);
        m_msleep(10);
    }

    if (reset_gpio != -1) {
        gpio_direction_output(reset_gpio, 1);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    ret = ar0234_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "ar0234: failed to detect\n");
        ar0234_power_off();
        return ret;
    }

    ret = ar0234_write_array(i2c_dev, ar0234_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "ar0234: failed to init regs\n");
        ar0234_power_off();
        return ret;
    }

    // printk("%s, %x\n", __func__, ar0234_attr.cbus_addr);

    return 0;
}

static int ar0234_stream_on(void)
{
    int ret = ar0234_write_array(i2c_dev, ar0234_regs_stream_on);
    if (ret)
        printk(KERN_ERR "ar0234: failed to stream on\n");

    // printk("%s, %x\n", __func__, ar0234_attr.cbus_addr);

    return ret;
}

static void ar0234_stream_off(void)
{
    int ret = ar0234_write_array(i2c_dev, ar0234_regs_stream_off);
    if (ret)
        printk(KERN_ERR "ar0234: failed to stream off\n");

    // printk("%s, %x\n", __func__, ar0234_attr.cbus_addr);

}

static int ar0234_g_register(struct sensor_dbg_register *reg)
{
    unsigned short val;
    int ret;

    ret = ar0234_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int ar0234_s_register(struct sensor_dbg_register *reg)
{
    return ar0234_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int ar0234_hvflip(tisp_hv_flip_t *hvflip)
{
    int ret = 0;

    ar0234_write_array(i2c_dev, ar0234_regs_stream_off);    // 0x3040 only can be set when disable streaming
    switch (hvflip->sensor_mode) {
    case IMPISP_FLIP_ISP_H_MODE: /* mirror */
        ret = ar0234_write(i2c_dev, 0x3040, 0x8000);
        break;
    case IMPISP_FLIP_ISP_V_MODE: /* flip */
        ret = ar0234_write(i2c_dev, 0x3040, 0x4000);
        break;
    case IMPISP_FLIP_ISP_HV_MODE: /* mirror & flip */
        ret = ar0234_write(i2c_dev, 0x3040, 0xC000);
        break;
    default:
        break;
    }
    ar0234_write_array(i2c_dev, ar0234_regs_stream_on);

    return ret;
}

inline static int ar0234_set_expo(int value)
{

    int ret = 0;
    int it = value & 0xffff;
    int again = (value & 0xffff0000) >> 16;

    /*set integration time*/
    ret = ar0234_write(i2c_dev, 0x3012, it);
    if (ret < 0) {
        printk(KERN_ERR "%s error, intergration time %d \n", __func__, it);
        return ret;
    }

    /*set analog gain*/
    ret = ar0234_write(i2c_dev, 0x3060, again);
    if (ret < 0) {
        printk("%s error, again %d\n", __func__, again);
        return ret;
    }

    return ret;
}

inline static int ar0234_set_expo_short(int value)
{
    int ret = 0;
    printk(KERN_DEBUG "ar0234_set_expo_short %d\n", value);
    return ret;
}

static int ar0234_set_integration_time(int value)
{
    // int ret = 0;

    // ret = ar0234_write(i2c_dev, 0x3012, value & 0xffff);
    // if (ret < 0) {
    //     printk(KERN_ERR "%s error, intergration time %d \n", __func__, value);
    //     return ret;
    // }

    return 0;
}

unsigned int ar0234_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

    struct again_lut *lut = ar0234_again_lut;

    while (lut->gain <= ar0234_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = 0;
            return lut[0].gain;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut-1)->value;
            return (lut-1)->gain;
        } else {
            if ((lut->gain == ar0234_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

static int ar0234_set_analog_gain(int value)
{
    // int ret = 0;

    // ret = ar0234_write(i2c_dev, 0x3060, value & 0xffff);
    // if (ret < 0) {
    //     printk("%s error, again %d\n", __func__, value);
    //     return ret;
    // }

    return 0;
}

unsigned int ar0234_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int ar0234_set_digital_gain(int value)
{
    return 0;
}

static int ar0234_set_fps(int fps)
{
    struct sensor_info *sensor_info = &ar0234_attr.sensor_info;
    unsigned int sclk = 0;
    unsigned short hts = 0;
    unsigned short vts = 0;
    unsigned int newformat = 0;
    int ret = 0;

    sclk = AR0234_SCLK;

     /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if (newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk(KERN_ERR "ar0234 fps(%d) not in range\n", fps);
        return -1;
    }

    /* hts */
    ret += ar0234_read(i2c_dev, 0x300C, &hts);
    if (ret < 0)
        return -1;

    /* vts */
    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret += ar0234_write(i2c_dev, 0x300A, vts);
    if (ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr ar0234_attr = {
    .device_name                = AR0234_DEVICE_NAME,
    .cbus_addr                  = AR0234_DEVICE_I2C_ADDR,

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
            .output_width = AR0234_DEVICE_WIDTH,
            .output_height = AR0234_DEVICE_HEIGHT,
        },
        .data_fmt               = MIPI_RAW8,
        .lanes                  = 2,
        .clk                    = 350,  /* Mbps/lane */
    },

    .isp_clk_rate               = 300 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = ar0234_1920_1200_30fps_mipi_init_regs,

        .width                  = AR0234_DEVICE_WIDTH,
        .height                 = AR0234_DEVICE_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SGRBG8_1X8,

        .fps                    = SENSOR_USE_FPS << 16 | 1,  /* 30/1 */
        .max_fps                = SENSOR_OUTPUT_MAX_FPS << 16 | 1,
        .min_fps                = SENSOR_OUTPUT_MIN_FPS << 16 | 1,
        .total_width            = AR0234_HTS,
        .total_height           = AR0234_VTS,

        .min_integration_time   = 2,
        .max_integration_time   = AR0234_VTS - 4,
        .one_line_expr_in_us    = 15,
        .max_again              = 262140,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = ar0234_power_on,
        .power_off              = ar0234_power_off,
        .stream_on              = ar0234_stream_on,
        .stream_off             = ar0234_stream_off,
        .get_register           = ar0234_g_register,
        .set_register           = ar0234_s_register,
        .set_hvflip             = ar0234_hvflip,

        .set_integration_time   = ar0234_set_integration_time,
        .alloc_again            = ar0234_alloc_again,
        .set_analog_gain        = ar0234_set_analog_gain,
        .alloc_dgain            = ar0234_alloc_dgain,
        .set_digital_gain       = ar0234_set_digital_gain,
        .set_fps                = ar0234_set_fps,
        .set_expo               = ar0234_set_expo,
        .set_expo_short         = ar0234_set_expo_short,
    },
};

static int ar0234_probe(struct i2c_client *client,
                       const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ar0234_attr.cbus_addr = i2c_addr;
    ret = camera_register_sensor(camera_index, &ar0234_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int ar0234_remove(struct i2c_client *client)
{
    camera_unregister_sensor(camera_index, &ar0234_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id ar0234_id[] = {
    {AR0234_DEVICE_NAME, 0},
    { }
};
MODULE_DEVICE_TABLE(i2c, ar0234_id);

static struct i2c_driver ar0234_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = AR0234_DEVICE_NAME,
    },
    .probe          = ar0234_probe,
    .remove         = ar0234_remove,
    .id_table       = ar0234_id,
};

static struct i2c_board_info sensor_ar0234_info = {
    .type           = AR0234_DEVICE_NAME,
    .addr           = AR0234_DEVICE_I2C_ADDR,
};

static __init int init_ar0234(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ar0234: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    ar0234_driver.driver.name = sensor_name;
    strcpy(sensor_ar0234_info.type, sensor_name);
    strcpy(ar0234_id[0].name, sensor_name);
    ar0234_attr.device_name = sensor_name;

    if (i2c_addr != -1)
        sensor_ar0234_info.addr = i2c_addr;

    int ret = i2c_add_driver(&ar0234_driver);
    if (ret) {
        printk(KERN_ERR "ar0234: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_ar0234_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "ar0234: failed to register i2c device\n");
        i2c_del_driver(&ar0234_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_ar0234(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&ar0234_driver);
}


module_init(init_ar0234);
module_exit(exit_ar0234);

MODULE_DESCRIPTION("X2580 AR0234 driver");
MODULE_LICENSE("GPL");
