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

static struct regulator *st7797_regulator = NULL;

static struct dsi_cmd_packet st7797_cmd_list[] =
{
    {0x15, 0xF0, 0xC3},
    {0x15, 0xF0, 0x96},
    {0x15, 0xF0, 0xA5},
    {0x15, 0xED, 0xC3},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE4, 0x40, 0x0F}},
    {0x15, 0xE7, 0x83},
    {0x39, 0x05, 0x00,(uint8_t[]){0xC3, 0x33, 0x02, 0x25, 0x04}},
    {0x39, 0x0f, 0x00,(uint8_t[]){0xE5, 0xB2, 0xF5, 0xBD, 0x24, 0x22, 0x25, 0x10, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22}},
    {0x39, 0x08, 0x00,(uint8_t[]){0xEC, 0x00, 0x55, 0x00, 0x00, 0x00, 0x49, 0x22}},
    {0x15, 0x36, 0x0C},
    {0x15, 0xB2, 0x00},
    {0x15, 0x3A, 0x07},
    {0x15, 0xC5, 0x6B},
    {0x39, 0x0f, 0x00, (uint8_t[]){0xE0, 0x88, 0x0B, 0x10, 0x08, 0x07, 0x03, 0x2C, 0x33, 0x43, 0x08, 0x16, 0x16, 0x2A, 0x2E}},
    {0x39, 0x0f, 0x00, (uint8_t[]){0xE1, 0x88, 0x0B, 0x10, 0x08, 0x06, 0x02, 0x2B, 0x32, 0x42, 0x09, 0x16, 0x15, 0x2A, 0x2E}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xA4, 0xC0, 0x63}},
    {0x15, 0xD9, 0x02},
    {0x39, 0x03, 0x00, (uint8_t[]){0xB6, 0xC7, 0x31}},
    {0x15, 0xB3, 0x01},
    {0x39, 0x05, 0x00, (uint8_t[]){0xC1, 0x77, 0x07, 0xC2, 0x15}},
    {0x39, 0x0a, 0x00, (uint8_t[]){0xA5, 0x00, 0x00, 0x00, 0x00, 0x20, 0x16, 0x2A, 0x8A, 0x02}},
    {0x39, 0x08, 0x00, (uint8_t[]){0xBA, 0x0A, 0x5A, 0x23, 0x10, 0x25, 0x02, 0x00}},
    {0x39, 0x09, 0x00, (uint8_t[]){0xBB, 0x00, 0x27, 0x00, 0x29, 0x82, 0x87, 0x18, 0x00}},
    {0x39, 0x0a, 0x00, (uint8_t[]){0xA6, 0x00, 0x00, 0x00, 0x00, 0x20, 0x16, 0x2A, 0x8A, 0x02}},
    {0x39, 0x09, 0x00, (uint8_t[]){0xBC, 0x00, 0x27, 0x00, 0x29, 0x82, 0x87, 0x18, 0x00}},
    {0x39, 0x0c, 0x00, (uint8_t[]){0xBD, 0xA1, 0xB2, 0x2B, 0x1A, 0x56, 0x43, 0x34, 0x65, 0xFF, 0xFF, 0x0F}},
    {0x15, 0x21, 0x00},
    {0x15, 0x35, 0x00},
};

static void st7797_init(void)
{
    int i;
    struct dsi_cmd_packet st7797_sleep_out = {0x05, 0x11, 0x00};
    struct dsi_cmd_packet st7797_display_on = {0x05, 0x29, 0x00};

    for (i = 0; i < ARRAY_SIZE(st7797_cmd_list); i++)
        dsi_write_cmd(&st7797_cmd_list[i]);

    dsi_write_cmd(&st7797_sleep_out);
    msleep(120);
    dsi_write_cmd(&st7797_display_on);
    msleep(50);
}

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "st7797: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int st7797_power_on(struct lcdc *lcdc)
{
    if (st7797_regulator)
        regulator_enable(st7797_regulator);

    if (gpio_lcd_power_en != -1)
        m_gpio_direction_output(gpio_lcd_power_en, 1);

    msleep(10);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(1);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    msleep(10);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(120);

    m_gpio_direction_output(gpio_lcd_backlight_en, 1);

    return 0;
}

static int st7797_power_off(struct lcdc *lcdc)
{
    if (st7797_regulator)
        regulator_disable(st7797_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 0);

    m_gpio_direction_output(gpio_lcd_backlight_en, 0);
    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "st7797",
    .refresh = 60,
    .xres = 400,
    .yres = 400,
    .pixclock = 0, // 自动计算
    .left_margin = 20,
    .right_margin = 10,
    .upper_margin = 10,
    .lower_margin = 8,
    .hsync_len = 4,
    .vsync_len = 4,

    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_MIPI,
    .out_format = OUT_FORMAT_RGB888,

    .mipi = {
        .num_of_lanes = 1,
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
    .height = 45,
    .width = 45,

    .power_on = st7797_power_on,
    .power_off = st7797_power_off,
    .lcd_init = st7797_init,
};

static int lcd_st7797_init(void)
{
    int ret;

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        st7797_regulator = regulator_get(NULL, lcd_regulator_name);
        if (!st7797_regulator) {
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
    if (st7797_regulator)
        regulator_put(st7797_regulator);
    return -1;
}

static void lcd_st7797_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
    if (st7797_regulator)
        regulator_put(st7797_regulator);
}

module_init(lcd_st7797_init);
module_exit(lcd_st7797_exit);

MODULE_DESCRIPTION("Ingenic Soc st7797_lcd driver");
MODULE_LICENSE("GPL");
