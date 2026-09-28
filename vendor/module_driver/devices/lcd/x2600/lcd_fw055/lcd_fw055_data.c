#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>

#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>
#include <linux/regulator/consumer.h>

int gpio_lcd_power_en = -1;
int gpio_lcd_rst = -1;
int gpio_lcd_backlight_en = -1;
static char *gpio_lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);
module_param(gpio_lcd_regulator_name, charp, 0644);

static struct regulator *fw055_regulator = NULL;

static struct dsi_cmd_packet fw055_cmd_list[] = {
    {0x39, 0x04, 0x00, (uint8_t[]){0xB9, 0xFF, 0x83, 0x99}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xD2, 0x77}},
    {0x39, 0x10, 0x00, (uint8_t[]){0xB1, 0x02, 0x04, 0x74, 0x94, 0x01, 0x32, 0x33, 0x11, 0x11, \
                                    0xAB, 0x4D, 0x56, 0x73, 0x02, 0x02}},
    {0x39, 0x10, 0x00, (uint8_t[]){0xB2, 0x42, 0x80, 0x80, 0xAE, 0x05, 0x07, 0x5A, 0x11, 0x00, \
                                    0x00, 0x10, 0x1E, 0x70, 0x03, 0xD4}},
    {0x39, 0x2D, 0x00, (uint8_t[]){0xB4, 0x00, 0xFF, 0x02, 0xC0, 0x02, 0xC0, 0x00, 0x00, 0x08, \
                                    0x00, 0x04, 0x06, 0x00, 0x32, 0x04, 0x0A, 0x08, 0x21, 0x03,\
                                    0x01, 0x00, 0x0F, 0xB8, 0x8B, 0x02, 0xC0, 0x02, 0xC0, 0x00,\
                                    0x00, 0x08, 0x00, 0x04, 0x06, 0x00, 0x32, 0x04, 0x0A, 0x08,\
                                    0x01, 0x00, 0x0F, 0xB8, 0x01}},
    {0x39, 0x22, 0x00, (uint8_t[]){0xD3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, \
                                    0x10, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
                                    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x05, 0x07, 0x00,\
                                    0x00, 0x00, 0x05, 0x40}},
    {0x39, 0x21, 0x00, (uint8_t[]){0xD5, 0x18, 0x18, 0x19, 0x19, 0x18, 0x18, 0x21, 0x20, 0x01, \
                                    0x00, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x18, 0x18, 0x18,\
                                    0x18, 0x18, 0x18, 0x2F, 0x2F, 0x30, 0x30, 0x31, 0x31, 0x18,\
                                    0x18, 0x18, 0x18}},
    {0x39, 0x21, 0x00, (uint8_t[]){0xD6, 0x18, 0x18, 0x19, 0x19, 0x40, 0x40, 0x20, 0x21, 0x02, \
                                    0x03, 0x04, 0x05, 0x06, 0x07, 0x00, 0x01, 0x40, 0x40, 0x40,\
                                    0x40, 0x40, 0x40, 0x2F, 0x2F, 0x30, 0x30, 0x31, 0x31, 0x40,\
                                    0x40, 0x40, 0x40}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xD8, 0xA2, 0xAA, 0x02, 0xA0, 0xA2, 0xA8, 0x02, 0xA0, 0xB0, \
                                    0x00, 0x00, 0x00, 0xB0, 0x00, 0x00, 0x00}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xBD, 0x01}},
    {0x39, 0x11, 0x00, (uint8_t[]){0xD8, 0xB0, 0x00, 0x00, 0x00, 0xB0, 0x00, 0x00, 0x00, 0xE2, \
                                    0xAA, 0x03, 0xF0, 0xE2, 0xAA, 0x03, 0xF0}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xBD, 0x02}},
    {0x39, 0x09, 0x00, (uint8_t[]){0xD8, 0xE2, 0xAA, 0x03, 0xF0, 0xE2, 0xAA, 0x03, 0xF0}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xBD, 0x00}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xB6, 0x66, 0x66}},//0x8D,0x8D
    {0x39, 0x37, 0x00, (uint8_t[]){0xE0, 0x00, 0x1A, 0x2A, 0x27, 0x5D, 0x69, 0x7A, 0x77, 0x7F, \
                                    0x89, 0x90, 0x96, 0x9B, 0xA4, 0xAC, 0xB1, 0xB5, 0xBF, 0xC2,\
                                    0xCB, 0xC0, 0xCE, 0xCF, 0x69, 0x64, 0x6D, 0x77, 0x00, 0x1A,\
                                    0x2A, 0x27, 0x5D, 0x69, 0x7A, 0x77, 0x7F, 0x89, 0x90, 0x96,\
                                    0x9B, 0xA4, 0xAC, 0xB1, 0xB5, 0xBF, 0xC2, 0xCB, 0xC0, 0xCE,\
                                    0xCF, 0x69, 0x64, 0x6D, 0x77}},
    {0x39, 0x03, 0x00, (uint8_t[]){0xC6, 0xFF, 0xF9}},
    {0x39, 0x02, 0x00, (uint8_t[]){0xCC, 0x08}},
};

static void fw055_init(void)
{
    int i;
    struct dsi_cmd_packet fw055_sleep_out = {0x05, 0x11, 0x00};
    struct dsi_cmd_packet fw055_display_on = {0x05, 0x29, 0x00};

    for (i = 0; i < ARRAY_SIZE(fw055_cmd_list); i++) {
        dsi_write_cmd(&fw055_cmd_list[i]);
    }

    dsi_write_cmd(&fw055_sleep_out);
    msleep(200);
    dsi_write_cmd(&fw055_display_on);
    msleep(50);
}

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "fw055: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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
    if (gpio >= 0) {
        gpio_direction_output(gpio, value);
    }
}

static int fw055_power_on(struct lcdc *lcdc)
{
    if (fw055_regulator)
        regulator_enable(fw055_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 1);
    msleep(50);

    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(30);
    m_gpio_direction_output(gpio_lcd_rst, 0);
    msleep(10);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    msleep(120);

    m_gpio_direction_output(gpio_lcd_backlight_en, 1);

    return 0;
}

static int fw055_power_off(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_backlight_en, 0);
    m_gpio_direction_output(gpio_lcd_power_en, 0);

    if (fw055_regulator)
        regulator_disable(fw055_regulator);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "fw055",
    .refresh = 60,
    .xres = 1080,
    .yres = 1920,
    .pixclock = 0, // 自动计算
    .left_margin = 60,
    .right_margin = 60,
    .upper_margin = 16,
    .lower_margin = 10,
    .hsync_len = 20,
    .vsync_len = 3,

    .fb_fmt = fb_fmt_RGB888,
    .lcd_mode = TFT_MIPI,
    .out_format = OUT_FORMAT_RGB888,

    .mipi = {
        .num_of_lanes = 4,
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
    .height = 127,
    .width = 70,

    .power_on = fw055_power_on,
    .power_off = fw055_power_off,
    .lcd_init = fw055_init,
};

static int lcd_fw055_init(void)
{
    int ret;

    if (strcmp("-1", gpio_lcd_regulator_name) && strlen(gpio_lcd_regulator_name)) {
        fw055_regulator = regulator_get(NULL, gpio_lcd_regulator_name);
        if(!fw055_regulator) {
            printk(KERN_ERR "lcd_regulator get err!\n");
            return -EINVAL;
        }
    }

    ret = m_gpio_request(gpio_lcd_power_en, "gpio_lcd_power_en");
    if(ret)
        goto power_on_err;

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
    m_gpio_free(gpio_lcd_backlight_en);
backlight_err:
    m_gpio_free(gpio_lcd_rst);
rst_err:
    m_gpio_free(gpio_lcd_power_en);
power_on_err:
    if (fw055_regulator)
        regulator_put(fw055_regulator);
    return -1;
}


static void lcd_fw055_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    if(fw055_regulator)
        regulator_put(fw055_regulator);

    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
}


module_init(lcd_fw055_init);
module_exit(lcd_fw055_exit);

MODULE_DESCRIPTION("Ingenic Soc fw055_lcd driver");
MODULE_LICENSE("GPL");