#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <linux/regulator/consumer.h>

#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>

int gpio_lcd_power_en = -1;
int gpio_lcd_rst = -1;
int gpio_lcd_backlight_en = -1;
static char *lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);
module_param(lcd_regulator_name, charp, 0644);

static struct regulator *st7701s_frd450_regulator = NULL;

static struct dsi_cmd_packet st7701s_frd450_cmd_list[] =
{
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x10}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC0, 0xE9, 0x03}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC1, 0x0A, 0x02}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC2, 0x37, 0x08}},
    {0x15, 0xCC, 0x30},
    {0x15, 0xCD, 0x08}, //MDT=1  RGB666 DB0-DB17
    {0x39, 0x11, 0x00, (uint8_t[]){0xB0, 0x00, 0x0C, 0x13, 0x0D, 0x10, 0x06, 0x04, 0x08, 0x09, 0x20, 0x04, 0x10, 0x0F, 0x29, 0x33, 0x1F}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xB1, 0x00, 0x0C, 0x13, 0x0D, 0x10, 0x05, 0x04, 0x08, 0x08, 0x20, 0x04, 0x11, 0x0F, 0x2A, 0x33, 0x1F}},
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x11}},
    {0x15, 0xB0, 0x4D},
    {0x15, 0xB1, 0x55},
    {0x15, 0xB2, 0x07},
    {0x15, 0xB3, 0x80},
    {0x15, 0xB5, 0x47},
    {0x15, 0xB7, 0x85},
    {0x15, 0xB8, 0x21},
    {0x15, 0xB9, 0x10},
    {0x15, 0xC1, 0x78},
    {0x15, 0xC2, 0x78},
    {0x15, 0xD0, 0x88}, //delay 100ms
};

static struct dsi_cmd_packet st7701s_frd450_cmd_list1[] =
{
    {0x39, 0x04, 0x00, (uint8_t[]){0xE0, 0x00, 0x00, 0x02}},
    {0x39, 0x0c, 0x00, (uint8_t[]){0xE1, 0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20}},
    {0x39, 0x0e, 0x00, (uint8_t[]){0xE2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {0x39, 0x05, 0x00, (uint8_t[]){0xE3, 0x00, 0x00, 0x33, 0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE4, 0x22, 0x00}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xE5, 0x04, 0x5C, 0xA0, 0xA0, 0x06, 0x5C, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {0x39, 0x05, 0x00, (uint8_t[]){0xE6, 0x00, 0x00, 0x33, 0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE7, 0x22, 0x00}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xE8, 0x05, 0x5C, 0xA0, 0xA0, 0x07, 0x5C, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {0x39, 0x08, 0x00, (uint8_t[]){0xEB, 0x02, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xEC, 0x00, 0x00}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xED, 0xFA, 0x45, 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB0, 0x54, 0xAF}},
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x00}},
    {0x15, 0x3A, 0x66}, // RGB18BIT   RGB666
    {0x15, 0x11, 0x00}, //delay 120ms
};

static void st7701s_frd450_init(void)
{
    int i;
    struct dsi_cmd_packet st7701s_frd450_display_on = {0x05, 0x29, 0x00};

    for (i = 0; i < ARRAY_SIZE(st7701s_frd450_cmd_list); i++)
        dsi_write_cmd(&st7701s_frd450_cmd_list[i]);
    msleep(100);
    for (i = 0; i < ARRAY_SIZE(st7701s_frd450_cmd_list1); i++)
        dsi_write_cmd(&st7701s_frd450_cmd_list1[i]);

    msleep(120);
    dsi_write_cmd(&st7701s_frd450_display_on);
}

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "st7701s_frd450: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static inline void m_gpio_direction_input(int gpio)
{
    if (gpio >= 0)
        gpio_direction_input(gpio);
}

static int st7701s_frd450_power_on(struct lcdc *lcdc)
{
    if (st7701s_frd450_regulator)
        regulator_enable(st7701s_frd450_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 1);

    msleep(10);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(1);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    msleep(1);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(120);

    m_gpio_direction_output(gpio_lcd_backlight_en, 1);

    return 0;
}

static int st7701s_frd450_power_off(struct lcdc *lcdc)
{
    if (st7701s_frd450_regulator)
        regulator_disable(st7701s_frd450_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 0);

    m_gpio_direction_output(gpio_lcd_backlight_en, 0);
    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "st7701s_frd450",
    .refresh = 60,
    .xres = 480,
    .yres = 854,
    .pixclock = 0, // 自动计算
    .left_margin = 60,
    .right_margin = 100,
    .upper_margin = 18,
    .lower_margin = 16,
    .hsync_len = 50,
    .vsync_len = 8,

    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_MIPI,
    .out_format = OUT_FORMAT_RGB888,

    .mipi = {
        .num_of_lanes = 2,
        .virtual_channel = 0,
        .color_coding = COLOR_CODE_24BIT,
        .data_en_polarity = AT_RISING_EDGE,
        .byte_clock = 0,
        .max_hs_to_lp_cycles = 100,
        .max_lp_to_hs_cycles = 40,
        .max_bta_cycles = 4095,
        .color_mode_polarity = AT_RISING_EDGE,
        .shut_down_polarity = AT_RISING_EDGE,
        .video_mode = VIDEO_BURST_WITH_SYNC_PULSES,

        .hsync_active_level = AT_LOW_LEVEL,
        .vsync_active_level = AT_LOW_LEVEL,
    },
    .height = 109,
    .width = 58,

    .power_on = st7701s_frd450_power_on,
    .power_off = st7701s_frd450_power_off,
    .lcd_init = st7701s_frd450_init,
};

static int lcd_st7701s_frd450_init(void)
{
    int ret;

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        st7701s_frd450_regulator = regulator_get(NULL, lcd_regulator_name);
        if (!st7701s_frd450_regulator) {
            printk(KERN_ERR "lcd_regulator get err!\n");
            return -EINVAL;
        }
    }

    ret = m_gpio_request(gpio_lcd_power_en, "gpio_lcd_power_en");
    if(ret)
        goto power_en_err;

    ret = m_gpio_request(gpio_lcd_rst, "gpio_lcd_rst");
    if (ret)
        goto rst_err;

    ret = m_gpio_request(gpio_lcd_backlight_en, "gpio_lcd_backlight_en");
    if (ret)
        goto backlight_err;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret < 0)
        goto register_err;

    return 0;

register_err:
    m_gpio_free(gpio_lcd_backlight_en);
backlight_err:
    m_gpio_free(gpio_lcd_rst);
rst_err:
    m_gpio_free(gpio_lcd_power_en);
power_en_err:
    if (st7701s_frd450_regulator)
        regulator_put(st7701s_frd450_regulator);
    return -1;
}

static void lcd_st7701s_frd450_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
    if (st7701s_frd450_regulator)
        regulator_put(st7701s_frd450_regulator);
}

module_init(lcd_st7701s_frd450_init);
module_exit(lcd_st7701s_frd450_exit);

MODULE_DESCRIPTION("Ingenic Soc st7701s_frd450_lcd driver");
MODULE_LICENSE("GPL");
