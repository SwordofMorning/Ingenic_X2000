#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <linux/regulator/consumer.h>

#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>

int gpio_lcd_power_en = -1;
int gpio_lcd_rst = -1;          // GPIO_PC(22) 0
int gpio_lcd_backlight_en = -1; // GPIO_PC(4) 1
static char *lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);
module_param(lcd_regulator_name, charp, 0644);

static struct regulator *ck450_regulator = NULL;

static struct dsi_cmd_packet fitipower_ck450_cmd_list1[] = {
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x13}},
    {0x15, 0xEF, 0x08},
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x10}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC0, 0xE9, 0x03}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC1, 0x0A, 0x08}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC2, 0x01, 0x06}},
    {0x15, 0xC7, 0x00},
    {0x15, 0xCC, 0x18},
    {0x39, 0x11, 0x00, (uint8_t[]){0xB0, 0x40, 0x0B, 0x58, 0x0C, 0x11, 0x06, 0x0B, 0x08, 0x09, 0x26, 0x06, 0x15, 0x18, 0x6A, 0x6E, 0x4F}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xB1, 0x40, 0x11, 0x57, 0x0D, 0x11, 0x07, 0x0B, 0x09, 0x08, 0x26, 0x05, 0xD3, 0x0D, 0x6B, 0x6E, 0x4F}},
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x11}},
    {0x15, 0xB0, 0x56},
    {0x15, 0xB1, 0x39},
    {0x15, 0xB2, 0x87},
    {0x15, 0xB3, 0x80},
    {0x15, 0xB5, 0x4E},
    {0x15, 0xB7, 0x85},
    {0x15, 0xB8, 0x10},
    {0x15, 0xB9, 0x10},
    {0x15, 0xBC, 0x03},
    {0x15, 0xC0, 0x89},
    {0x15, 0xC1, 0x78},
    {0x15, 0xC2, 0x78},
    {0x15, 0xD0, 0x88},
    {0x39, 0x04, 0x00, (uint8_t[]){0xE0, 0x00, 0x00, 0x02}},
    {0x39, 0x0C, 0x00, (uint8_t[]){0xE1, 0x04, 0xA0, 0x00, 0x00, 0x05, 0xA0, 0x00, 0x00, 0x00, 0x20, 0x20}},
    {0x39, 0x0E, 0x00, (uint8_t[]){0xE2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {0x39, 0x05, 0x00, (uint8_t[]){0xE3, 0x00, 0x00, 0x33, 0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE4, 0x22, 0x00}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xE5, 0x04, 0x5C, 0xA0, 0xA0, 0x06, 0x5C, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {0x39, 0x05, 0x00, (uint8_t[]){0xE6, 0x00, 0x00, 0x33, 0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE7, 0x22, 0x00}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xE8, 0x05, 0x5C, 0xA0, 0xA0, 0x07, 0x5C, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {0x39, 0x08, 0x00, (uint8_t[]){0xEB, 0x02, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xEC, 0x00, 0x00}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xED, 0xAA, 0x45, 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB0, 0x54, 0xAA}},
    {0x39, 0x07, 0x00, (uint8_t[]){0xEF, 0x08, 0x08, 0x08, 0x45, 0x3F, 0x54}},
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x13}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE8, 0x00, 0x0E}},
    {0x05, 0x11, 0x00},//delay 120ms
};

static struct dsi_cmd_packet fitipower_ck450_cmd_list2[] = {
    {0x39, 0x06, 0x00, (uint8_t[]){0xFF, 0x77, 0x01, 0x00, 0x00, 0x13}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE8, 0x00, 0x0C}},//delay 10ms
};

static struct dsi_cmd_packet fitipower_ck450_cmd_list3[] = {
    {0x39, 0x03, 0x00, (uint8_t[]){0xE8, 0x00, 0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xE6, 0x16, 0x7C}},
    {0x15, 0x36, 0x00},
};

static void ck450_init(void)
{
    int i;
    struct dsi_cmd_packet ck450_display_on = {0x05, 0x29, 0x00};

    for (i = 0; i < ARRAY_SIZE(fitipower_ck450_cmd_list1); i++)
        dsi_write_cmd(&fitipower_ck450_cmd_list1[i]);
    msleep(120);

    for (i = 0; i < ARRAY_SIZE(fitipower_ck450_cmd_list2); i++)
        dsi_write_cmd(&fitipower_ck450_cmd_list2[i]);
    msleep(10);

    for (i = 0; i < ARRAY_SIZE(fitipower_ck450_cmd_list3); i++)
        dsi_write_cmd(&fitipower_ck450_cmd_list3[i]);

    dsi_write_cmd(&ck450_display_on);
    msleep(20);
}

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "ck450: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int ck450_power_on(struct lcdc *lcdc)
{
    if (ck450_regulator)
        regulator_enable(ck450_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 0);
    msleep(5);

    m_gpio_direction_output(gpio_lcd_rst, 0);
    msleep(10);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(60);

    m_gpio_direction_output(gpio_lcd_backlight_en, 1);

    return 0;
}

static int ck450_power_off(struct lcdc *lcdc)
{
    if (ck450_regulator)
        regulator_disable(ck450_regulator);

    m_gpio_direction_output(gpio_lcd_backlight_en, 0);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    m_gpio_direction_output(gpio_lcd_power_en, 1);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "ck450",/* st7701s */
    .refresh = 45,
    .xres = 480,
    .yres = 854,
    .pixclock = 0, // 自动计算
    .left_margin = 50,
    .right_margin = 30,
    .upper_margin = 11,
    .lower_margin = 12,
    .hsync_len = 10,
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
    .height = 98,
    .width = 55,

    .power_on = ck450_power_on,
    .power_off = ck450_power_off,
    .lcd_init = ck450_init,
};

static int lcd_ck450_init(void)
{
    int ret;

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        ck450_regulator = regulator_get(NULL, lcd_regulator_name);
        if(!ck450_regulator) {
            printk(KERN_ERR "lcd_regulator get err!\n");
            return -EINVAL;
        }
    }

    ret = m_gpio_request(gpio_lcd_power_en, "gpio_lcd_power_en");
    if (ret)
        return -1;

    ret = m_gpio_request(gpio_lcd_rst, "gpio_lcd_rst");
    if (ret)
        goto rst_err;

    ret = m_gpio_request(gpio_lcd_backlight_en, "gpio_lcd_backlight_en");
    if(ret)
        goto backlight_err;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret < 0) {
        goto register_err;
    }

    return 0;

register_err:
    gpio_free(gpio_lcd_backlight_en);
backlight_err:
    gpio_free(gpio_lcd_rst);
rst_err:
    gpio_free(gpio_lcd_power_en);

    return -1;
}

static void lcd_ck450_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
}

module_init(lcd_ck450_init);
module_exit(lcd_ck450_exit);

MODULE_DESCRIPTION("Ingenic Soc ck450_lcd driver");
MODULE_LICENSE("GPL");