#include <utils/gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <fb/lcdc_data.h>

static int gpio_lcd_power_en = -1;
static int gpio_lcd_backlight_en = -1;
static int gpio_lcd_display_en = -1;

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);
module_param_gpio(gpio_lcd_display_en, 0644);

static inline void m_msleep(int ms)
{
    usleep_range(ms * 1000, ms * 1000);
}

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "qm047ks01: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int st7283_power_on(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en, 1);
    m_gpio_direction_output(gpio_lcd_display_en, 1);
    // m_msleep(300); // 15 * frames

    m_gpio_direction_output(gpio_lcd_backlight_en, 1);

    return 0;
}

static int st7283_power_off(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_backlight_en, 0);
    // m_msleep(120); // 6 * frames
    m_gpio_direction_output(gpio_lcd_display_en, 0);
    m_gpio_direction_output(gpio_lcd_power_en, 0);

    return 0;
}

struct lcdc_data lcdc_data = {
    .name = "st7283",
    .refresh = 150,
    .xres = 480,
    .yres = 272,
    .pixclock = 0, // 自动计算
    .left_margin = 43,
    .right_margin = 8,
    .upper_margin = 12,
    .lower_margin = 8,
    .hsync_len = 4,
    .vsync_len = 3,
    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_8BITS_SERIAL,
    .out_format = OUT_FORMAT_RGB888,
    .tft = {
        .even_line_order = ORDER_RGB,
        .odd_line_order = ORDER_RGB,
        .pix_clk_polarity = AT_FALLING_EDGE,
        .de_active_level = AT_HIGH_LEVEL,
        .hsync_active_level = AT_LOW_LEVEL,
        .vsync_active_level = AT_LOW_LEVEL,
    },
    .power_on = st7283_power_on,
    .power_off = st7283_power_off,
};

static int __init st7283_init(void)
{
    int ret;

    ret = m_gpio_request(gpio_lcd_power_en, "gpio_lcd_power_en");
    if (ret)
        return -1;

    ret = m_gpio_request(gpio_lcd_display_en, "gpio_lcd_display_en");
    if (ret)
        goto display_err;

    ret = m_gpio_request(gpio_lcd_backlight_en, "gpio_lcd_backlight_en");
    if (ret)
        goto backlight_err;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret < 0) {
        goto register_err;
    }

    return 0;
register_err:
    gpio_free(gpio_lcd_backlight_en);
backlight_err:
    gpio_free(gpio_lcd_display_en);
display_err:
    gpio_free(gpio_lcd_power_en);

    return -1;
}

static void __exit st7283_exit(void)
{
    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_display_en);
    m_gpio_free(gpio_lcd_power_en);
}

module_init(st7283_init);
module_exit(st7283_exit);

MODULE_DESCRIPTION("st7283 lcd panel driver");
MODULE_LICENSE("GPL");