#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <linux/regulator/consumer.h>

#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>

int gpio_lcd_power_en = -1;     // GPIO_PB(9)
int gpio_lcd_rst = -1;          // GPIO_PB(8)
int gpio_lcd_backlight_en = -1; // GPIO_PC(0)
int gpio_lcd_te = -1;           //GPIO_PB(27)
static char *lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);
module_param_gpio(gpio_lcd_te, 0644);
module_param(lcd_regulator_name, charp, 0644);

static struct regulator *st7703_regulator = NULL;

static struct dsi_cmd_packet fitipower_st7703_640_960_cmd_list[] =
{
    {0x39, 0x04, 0x00, (uint8_t[]){0xB9, 0xF1, 0x12, 0x83}},
    {0x39, 0x04, 0x00, (uint8_t[]){0xB2, 0x78, 0x23, 0xF0}},
    {0x39, 0x0B, 0x00, (uint8_t[]){0xB3, 0x10, 0x10, 0x28, 0x28, 0x03, 0xFF, 0x00, 0x00, 0x00,  \
                                    0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xB4, 0x80}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xB5, 0x06, 0x06}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xB6, 0xB1, 0xBB}},
    {0x39, 0x05, 0x00, (uint8_t[]){0xB8, 0x26, 0x22, 0xF0, 0x63}},
    {0x39, 0x1C, 0x00, (uint8_t[]){0xBA, 0x31, 0x81, 0x05, 0xF9, 0x0E, 0x0E, 0x20, 0x00, 0x00,  \
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x25, 0x00, 0x90, 0x0A, \
                                    0x00, 0x00, 0x01, 0x4F, 0x01, 0x00, 0x00, 0x37}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xBC, 0x4F}},
    {0x39, 0x04, 0x00, (uint8_t[]){0xBF, 0x02, 0x11, 0x00}},
    {0x39, 0x0A, 0x00, (uint8_t[]){0xC0, 0x73, 0x73, 0x50, 0x50, 0x00, 0x00, 0x08, 0x70, 0x00}},
    {0x39, 0x0D, 0x00, (uint8_t[]){0xC1, 0x25, 0xC0, 0x32, 0x32, 0x99, 0xE4, 0xFF, 0xFF, 0xCC,  \
                                    0xCC, 0x77, 0x77}},
    {0x39, 0x07, 0x00, (uint8_t[]){0xC6, 0x82, 0x00, 0x3F, 0xFF, 0x00, 0xE0}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xCC, 0x0B}},
    {0x39, 0x23, 0x00, (uint8_t[]){0xE0, 0x00, 0x15, 0x19, 0x2E, 0x3B, 0x3F, 0x46, 0x37, 0x08,  \
                                    0x0D, 0x0D, 0x11, 0x13, 0x12, 0x13, 0x0F, 0x15, 0x00, 0x15, \
                                    0x19, 0x2E, 0x3B, 0x3F, 0x46, 0x37, 0x08, 0x0D, 0x0D, 0x11, \
                                    0x13, 0x12, 0x13, 0x0F, 0x15}},
    {0x39, 0x0F, 0x00, (uint8_t[]){0xE3, 0x07, 0x07, 0x0B, 0x0B, 0x03, 0x03, 0x00, 0x00, 0x00,  \
                                    0x00, 0xFF, 0x00, 0xC0, 0x10}},
    {0x39, 0x40, 0x00, (uint8_t[]){0xE9, 0xC8, 0x10, 0x0A, 0x03, 0xC5, 0x80, 0x38, 0x12, 0x31,  \
                                    0x23, 0x4F, 0x86, 0x80, 0x38, 0x47, 0x08, 0x3C, 0x00, 0xE0, \
                                    0x0C, 0x00, 0x00, 0x3C, 0x00, 0xE0, 0x0C, 0x00, 0x00, 0x88, \
                                    0x8F, 0xF9, 0x94, 0x44, 0x66, 0x00, 0x22, 0x88, 0xAA, 0x02, \
                                    0x88, 0x8F, 0xF9, 0x94, 0x55, 0x77, 0x11, 0x33, 0x88, 0xAA, \
                                    0x13, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                    0x00, 0x00, 0x00, 0x00}},
    {0x39, 0x3E, 0x00, (uint8_t[]){0xEA, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  \
                                    0x00, 0x00, 0x00, 0x8F, 0xF8, 0x89, 0x94, 0x33, 0x11, 0x77, \
                                    0x55, 0x88, 0xAA, 0x31, 0x8F, 0xF8, 0x89, 0x94, 0x22, 0x00, \
                                    0x66, 0x44, 0x88, 0xAA, 0x20, 0x23, 0x00, 0x00, 0x01, 0xB0, \
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                    0x00, 0x03, 0xCC, 0x00, 0x00, 0x40, 0x80, 0x38, 0x40, 0x80, \
                                    0x81, 0x00}},
};

static void st7703_init(void)
{
    int i;
    struct dsi_cmd_packet st7703_sleep_out = {0x05, 0x11, 0x00};
    struct dsi_cmd_packet st7703_display_on = {0x05, 0x29, 0x00};

    for (i = 0; i < ARRAY_SIZE(fitipower_st7703_640_960_cmd_list); i++)
        dsi_write_cmd(&fitipower_st7703_640_960_cmd_list[i]);

    msleep(5);
    dsi_write_cmd(&st7703_sleep_out);
    msleep(120);
    dsi_write_cmd(&st7703_display_on);
    msleep(10);
}

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "st7703: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int st7703_power_on(struct lcdc *lcdc)
{
    if (st7703_regulator)
        regulator_enable(st7703_regulator);

    if (gpio_lcd_power_en != -1) {
        m_gpio_direction_output(gpio_lcd_power_en, 0);
        msleep(10);
        m_gpio_direction_output(gpio_lcd_power_en, 1);
    }

    m_gpio_direction_input(gpio_lcd_te);
    msleep(10);

    m_gpio_direction_output(gpio_lcd_rst, 0);
    msleep(10);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(10);

    m_gpio_direction_output(gpio_lcd_backlight_en, 1);

    return 0;
}

static int st7703_power_off(struct lcdc *lcdc)
{
    if (st7703_regulator)
        regulator_disable(st7703_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 0);

    m_gpio_direction_output(gpio_lcd_backlight_en, 0);
    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "st7703",
    .refresh = 60,
    .xres = 640,
    .yres = 960,
    .pixclock = 0, // 自动计算
    .left_margin = 35,
    .right_margin = 45,
    .upper_margin = 21,
    .lower_margin = 16,
    .hsync_len = 45,
    .vsync_len = 4,

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

        .hsync_active_level = AT_HIGH_LEVEL,
        .vsync_active_level = AT_HIGH_LEVEL,
    },
    .height = 81,
    .width = 52,

    .power_on = st7703_power_on,
    .power_off = st7703_power_off,
    .lcd_init = st7703_init,
};

static int lcd_st7703_init(void)
{
    int ret;

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        st7703_regulator = regulator_get(NULL, lcd_regulator_name);
        if (!st7703_regulator) {
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

    ret = m_gpio_request(gpio_lcd_te, "gpio_lcd_te");
    if (ret)
        goto te_err;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret < 0)
        goto register_err;

    return 0;

register_err:
    m_gpio_free(gpio_lcd_te);
te_err:
    m_gpio_free(gpio_lcd_backlight_en);
backlight_err:
    m_gpio_free(gpio_lcd_rst);
rst_err:
    m_gpio_free(gpio_lcd_power_en);
power_en_err:
    if (st7703_regulator)
        regulator_put(st7703_regulator);
    return -1;
}

static void lcd_st7703_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_te);
    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
    if (st7703_regulator)
        regulator_put(st7703_regulator);
}

module_init(lcd_st7703_init);
module_exit(lcd_st7703_exit);

MODULE_DESCRIPTION("Ingenic Soc st7703_lcd driver");
MODULE_LICENSE("GPL");