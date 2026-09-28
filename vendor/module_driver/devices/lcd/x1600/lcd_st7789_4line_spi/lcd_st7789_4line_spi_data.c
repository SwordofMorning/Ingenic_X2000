#include <utils/gpio.h>
#include <utils/spi.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/module.h>

#include <fb/lcdc_data.h>

static int gpio_lcd_rst = -1;       // GPIO_PA(28)  低电平有效
static int gpio_lcd_power_en = -1;  //GPIO_PC(02) 高电平有效
static int gpio_spi_cs = -1;        // GPIO_PA(23) 低电平有效

module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_spi_cs, 0644);

struct smart_lcd_data_table st7789v_data_table[] = {

    {SMART_CONFIG_CMD, 0x11},
    {SMART_CONFIG_UDELAY, 120000}, //Delay 120ms
    //************* Start Initial Sequence **********//

    //------------------------display and color format setting-------------------------

    {SMART_CONFIG_CMD, 0x36}, // Memory Access Control
    {SMART_CONFIG_DATA, 0x60}, //A0

    {SMART_CONFIG_CMD, 0x3A},
    {SMART_CONFIG_DATA, 0x05},
//---------------------------ST7789S Frame rate setting-------------------------

    {SMART_CONFIG_CMD, 0xb2},
    {SMART_CONFIG_DATA, 0x0c},
    {SMART_CONFIG_DATA, 0x0c},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x33},
    {SMART_CONFIG_DATA, 0x33},

    {SMART_CONFIG_CMD, 0xb7},
    {SMART_CONFIG_DATA, 0x35},
    //------------------------------ST7789S Power setting------------------------------

    {SMART_CONFIG_CMD, 0xbb},
    {SMART_CONFIG_DATA, 0x35},  //    0x1A

    {SMART_CONFIG_CMD, 0xc0},
    {SMART_CONFIG_DATA, 0x2c},

    {SMART_CONFIG_CMD, 0xc2},
    {SMART_CONFIG_DATA, 0x01},

    {SMART_CONFIG_CMD, 0xc3},
    {SMART_CONFIG_DATA, 0x13}, // 0x0B

    {SMART_CONFIG_CMD, 0xc4},
    {SMART_CONFIG_DATA, 0x20},

    {SMART_CONFIG_CMD, 0xc6},
    {SMART_CONFIG_DATA, 0x0F},

    {SMART_CONFIG_CMD, 0xca},
    {SMART_CONFIG_DATA, 0x0f},

    {SMART_CONFIG_CMD, 0xc8},
    {SMART_CONFIG_DATA, 0x08},

    {SMART_CONFIG_CMD, 0x55},
    {SMART_CONFIG_DATA, 0x90},

    {SMART_CONFIG_CMD, 0xd0},
    {SMART_CONFIG_DATA, 0xa4},
    {SMART_CONFIG_DATA, 0xa1},
    //-----------------------------ST7789S gamma setting-------------------------

    {SMART_CONFIG_CMD, 0xe0},
    {SMART_CONFIG_DATA, 0xd0},   //00
    {SMART_CONFIG_DATA, 0x00},   //03
    {SMART_CONFIG_DATA, 0x06},   //07
    {SMART_CONFIG_DATA, 0x09},   //08
    {SMART_CONFIG_DATA, 0x0b},   //15
    {SMART_CONFIG_DATA, 0x2a},   //2A
    {SMART_CONFIG_DATA, 0x3c},   //44
    {SMART_CONFIG_DATA, 0x55},   //42
    {SMART_CONFIG_DATA, 0x4b},   //0A
    {SMART_CONFIG_DATA, 0x08},   //17
    {SMART_CONFIG_DATA, 0x16},   //18
    {SMART_CONFIG_DATA, 0x14},   //18
    {SMART_CONFIG_DATA, 0x19},   //25
    {SMART_CONFIG_DATA, 0x20},   //27

    {SMART_CONFIG_CMD, 0xe1},
    {SMART_CONFIG_DATA, 0xd0},   //00
    {SMART_CONFIG_DATA, 0x00},   //03
    {SMART_CONFIG_DATA, 0x06},   //08
    {SMART_CONFIG_DATA, 0x09},   //07
    {SMART_CONFIG_DATA, 0x0b},   //07
    {SMART_CONFIG_DATA, 0x29},   //23
    {SMART_CONFIG_DATA, 0x36},   //2A
    {SMART_CONFIG_DATA, 0x54},   //43
    {SMART_CONFIG_DATA, 0x4b},   //42
    {SMART_CONFIG_DATA, 0x0d},   //09
    {SMART_CONFIG_DATA, 0x16},   //18
    {SMART_CONFIG_DATA, 0x14},   //17
    {SMART_CONFIG_DATA, 0x21},   //25
    {SMART_CONFIG_DATA, 0x20},   //27

    // {SMART_CONFIG_CMD, 0x21},
    // {SMART_CONFIG_UDELAY, 120000}, //Delay 120ms

    {SMART_CONFIG_CMD, 0x2A},   //Add
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x01},
    {SMART_CONFIG_DATA, 0x3f},

    {SMART_CONFIG_CMD, 0x2B},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0x00},
    {SMART_CONFIG_DATA, 0xef},

    // {SMART_CONFIG_CMD, 0x21},
    // {SMART_CONFIG_UDELAY, 120000}, //Delay 120ms

    {SMART_CONFIG_CMD, 0x29},

};

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;
    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "st7789: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }
    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}

static inline void m_gpio_direction_output(int gpio, int value)
{
    if (gpio >= 0)
        gpio_direction_output(gpio, value);
}

static int st7789v_power_on(struct lcdc *lcdc)
{

    if (gpio_lcd_power_en >= 0) {
        m_gpio_direction_output(gpio_lcd_power_en, 1);
        usleep_range(20*1000, 20*1000);
    }

    m_gpio_direction_output(gpio_spi_cs, 1);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    usleep_range(20*1000, 20*1000);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    usleep_range(20*1000, 20*1000);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    usleep_range(120*1000, 120*1000);

    m_gpio_direction_output(gpio_spi_cs, 0);

    return 0;
}

static int st7789v_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        m_gpio_direction_output(gpio_lcd_power_en, 0);

    m_gpio_direction_output(gpio_lcd_rst, 0);
    m_gpio_direction_output(gpio_spi_cs, 1);

    return 0;
}


struct lcdc_data lcdc_data = {
    .name = "st7789v",
    .refresh = 60,
    .xres = 240,
    .yres = 320,

    .pixclock = 0,
    .fb_fmt = fb_fmt_ARGB8888, /* fb_fmt_RGB565 // fb_fmt_RGB888 */
    .lcd_mode = SLCD_SPI_4LINE,
    .out_format = OUT_FORMAT_RGB565, /* OUT_FORMAT_RGB565 // OUT_FORMAT_RGB888 */
    .slcd = {
        .pixclock_when_init      = 240 * 320 * 3,
        .wr_data_sample_edge     = AT_RISING_EDGE,
        .dc_pin                  = CMD_LOW_DATA_HIGH,
        .te_data_transfered_edge = AT_RISING_EDGE,
#ifdef USE_TE
        .te_pin_mode        = TE_LCDC_TRIGGER,
#else
        .te_pin_mode        = TE_NOT_EANBLE,
#endif
        .enable_rdy_pin     = 0,
        .cmd_of_start_frame = 0x2c,
    },
    .slcd_data_table = st7789v_data_table,
    .slcd_data_table_length = ARRAY_SIZE(st7789v_data_table),
    .power_on = st7789v_power_on,
    .power_off = st7789v_power_off,
};

static int lcd_st7789_init(void)
{
    int ret;

    if (gpio_lcd_power_en < 0) {
        printk("st7789: must set lcd power_en\n");
        return -EINVAL;
    }

    if (gpio_spi_cs < 0) {
        printk("st7789: must set gpio_cs\n");
        return -EINVAL;
    }

    ret = m_gpio_request(gpio_lcd_power_en, "lcd power-en");
    if (ret)
        return ret;

    ret = m_gpio_request(gpio_lcd_rst, "lcd reset");
    if (ret)
        goto err_rst;

    ret = m_gpio_request(gpio_spi_cs, "lcd cs");
    if (ret)
        goto err_cs;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret) {
        printk(KERN_ERR "st7789v: failed to register lcd data\n");
        goto err_lcdc_data;
    }

    return 0;
err_lcdc_data:
    m_gpio_free(gpio_spi_cs);
err_cs:
    m_gpio_free(gpio_lcd_rst);
err_rst:
    m_gpio_free(gpio_lcd_power_en);
    return -1;
}


static void lcd_st7789_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_spi_cs);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
}

module_init(lcd_st7789_init);

module_exit(lcd_st7789_exit);

MODULE_DESCRIPTION("Ingenic ST7789 driver");
MODULE_LICENSE("GPL");