#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <fb/lcdc_data.h>
#include <linux/regulator/consumer.h>

static int gpio_lcd_power_en = -1;
static int gpio_lcd_rst = -1; // PB20
static int gpio_lcd_cs = -1; // PB28

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_cs, 0644);
module_param_gpio(gpio_lcd_rst, 0644);

/* 4 line spi (hp module) */
struct smart_lcd_data_table st7789v_data_table[] = {
    {SMART_CONFIG_CMD,0x11},
    {SMART_CONFIG_UDELAY, 120000}, //Delay 120ms
    //-------------------------------display and color format setting-----------------------------//
    {SMART_CONFIG_CMD, 0x36}, // Memory Access
    {SMART_CONFIG_DATA, 0x00}, //Normal

    {SMART_CONFIG_CMD,0x3A},//pixel format
    {SMART_CONFIG_PRM , 0x55},  //65k,RGB565

    {SMART_CONFIG_CMD,0x21},  //inversion on

    //{SMART_CONFIG_CMD,0xe7}, //2 data lane interface
    //{SMART_CONFIG_PRM , 0x10}, //enable

    //--------------------------------ST7789S Frame rate setting----------------------------------//
    {SMART_CONFIG_CMD,0xb2},
    {SMART_CONFIG_PRM , 0x0c},
    {SMART_CONFIG_PRM , 0x0c},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x33},
    {SMART_CONFIG_PRM , 0x33},
    {SMART_CONFIG_CMD,0xb7},
    {SMART_CONFIG_PRM , 0x35},
    //---------------------------------ST7789S Power setting--------------------------------------//

    {SMART_CONFIG_CMD,0xbb},
    {SMART_CONFIG_PRM , 0x33},
    {SMART_CONFIG_CMD,0xc3},
    {SMART_CONFIG_PRM , 0x1a},
    {SMART_CONFIG_CMD,0xc4},
    {SMART_CONFIG_PRM , 0x18},
    {SMART_CONFIG_CMD,0xc6},
    {SMART_CONFIG_PRM , 0x01},
    {SMART_CONFIG_CMD,0xd0},
    {SMART_CONFIG_PRM , 0xa4},
    {SMART_CONFIG_PRM , 0xb3},

    //--------------------------------ST7789S gamma setting---------------------------------------//
    {SMART_CONFIG_CMD,0xe0},
    {SMART_CONFIG_PRM , 0xf0},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x0a},
    {SMART_CONFIG_PRM , 0x10},
    {SMART_CONFIG_PRM , 0x12},
    {SMART_CONFIG_PRM , 0x1b},
    {SMART_CONFIG_PRM , 0x39},
    {SMART_CONFIG_PRM , 0x44},
    {SMART_CONFIG_PRM , 0x47},
    {SMART_CONFIG_PRM , 0x28},
    {SMART_CONFIG_PRM , 0x12},
    {SMART_CONFIG_PRM , 0x10},
    {SMART_CONFIG_PRM , 0x16},
    {SMART_CONFIG_PRM , 0x1b},
    {SMART_CONFIG_CMD,0xe1},
    {SMART_CONFIG_PRM , 0xf0},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x0a},
    {SMART_CONFIG_PRM , 0x10},
    {SMART_CONFIG_PRM , 0x11},
    {SMART_CONFIG_PRM , 0x1a},
    {SMART_CONFIG_PRM , 0x3b},
    {SMART_CONFIG_PRM , 0x34},
    {SMART_CONFIG_PRM , 0x4e},
    {SMART_CONFIG_PRM , 0x3a},
    {SMART_CONFIG_PRM , 0x17},
    {SMART_CONFIG_PRM , 0x16},
    {SMART_CONFIG_PRM , 0x21},
    {SMART_CONFIG_PRM , 0x22},

    {SMART_CONFIG_CMD,0x2a},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0xef},

    {SMART_CONFIG_CMD,0x2b},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x00},
    {SMART_CONFIG_PRM , 0x01},
    {SMART_CONFIG_PRM , 0x3f},

    {SMART_CONFIG_CMD,0x29},
};

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "st7789v: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

    m_gpio_direction_output(gpio_lcd_cs, 1);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    usleep_range(20*1000, 20*1000);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    usleep_range(120*1000, 120*1000);
    m_gpio_direction_output(gpio_lcd_rst, 1);

    m_gpio_direction_output(gpio_lcd_cs, 0);

    return 0;
}

static int st7789v_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        m_gpio_direction_output(gpio_lcd_power_en, 0);

    m_gpio_direction_output(gpio_lcd_rst, 0);
    m_gpio_direction_output(gpio_lcd_cs, 1);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name       = "st7789v",
    .refresh    = 45,
    .xres       = 240,
    .yres       = 320,

    .pixclock   = 0, // 自动计算
    .fb_fmt     = fb_fmt_ARGB8888,
    .lcd_mode   = SLCD_SPI_4LINE,
    .out_format = OUT_FORMAT_RGB565,
    .slcd = {
        .pixclock_when_init      = 240*320*3,
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
    .slcd_data_table        = st7789v_data_table,
    .slcd_data_table_length = ARRAY_SIZE(st7789v_data_table),
    .power_on               = st7789v_power_on,
    .power_off              = st7789v_power_off,
};

static int lcd_st7789v_init(void)
{
    int ret;

    if (gpio_lcd_cs < 0) {
        printk("st7789v spi: must set gpio chip select\n");
        return -EINVAL;
    }

    if (gpio_lcd_rst < 0) {
        printk("st7789v spi: must set gpio reset\n");
        return -EINVAL;
    }

    ret = m_gpio_request(gpio_lcd_power_en, "lcd power-en");
    if (ret)
        return ret;

    ret = m_gpio_request(gpio_lcd_rst, "lcd reset");
    if (ret)
        goto err_rst;

    ret = m_gpio_request(gpio_lcd_cs, "lcd cs");
    if (ret)
        goto err_cs;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret) {
        printk(KERN_ERR "st7789v: failed to register lcd data\n");
        goto err_lcdc_data;
    }

    return 0;

err_lcdc_data:
    m_gpio_free(gpio_lcd_cs);
err_cs:
    m_gpio_free(gpio_lcd_rst);
err_rst:
    m_gpio_free(gpio_lcd_power_en);
    return -1;
}

static void lcd_st7789v_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_cs);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
}

module_init(lcd_st7789v_init);

module_exit(lcd_st7789v_exit);

MODULE_DESCRIPTION("Ingenic ST7789V 4Line SPI driver");
MODULE_LICENSE("GPL");
