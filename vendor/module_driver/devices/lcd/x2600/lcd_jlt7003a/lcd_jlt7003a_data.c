#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <utils/gpio.h>
#include <fb/lcdc_data.h>
#include <linux/regulator/consumer.h>

static int gpio_lcd_power_en         = -1;
static int gpio_lcd_rst              = -1;/* PB03 0*/
static int gpio_lcd_backlight_en     = -1;/* PC11 1, pwm, not here*/
static char *lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);
module_param(lcd_regulator_name, charp, 0644);
static struct regulator *jlt7003a_regulator = NULL;

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
        printk(KERN_ERR "lcd_jlt7003a: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int jlt7003a_power_on(struct lcdc *lcdc)
{
    if (jlt7003a_regulator)
        regulator_enable(jlt7003a_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 0);
    msleep(50);
    m_gpio_direction_output(gpio_lcd_power_en, 1);

    m_gpio_direction_output(gpio_lcd_rst, 0);
    m_msleep(10);
    m_gpio_direction_output(gpio_lcd_rst, 1);
    m_msleep(10);

    m_gpio_direction_output(gpio_lcd_backlight_en, 1);

    return 0;
}

static int jlt7003a_power_off(struct lcdc *lcdc)
{
    if (jlt7003a_regulator)
        regulator_disable(jlt7003a_regulator);

    m_gpio_direction_output(gpio_lcd_backlight_en, 0);
    m_gpio_direction_output(gpio_lcd_power_en, 0);
    m_gpio_direction_output(gpio_lcd_rst, 0);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "jlt7003a",
    .refresh = 60,
    .xres = 1024,
    .yres = 600,
    .pixclock = 0,
    .left_margin = 100,
    .right_margin = 100,
    .upper_margin = 60,
    .lower_margin = 60,
    .hsync_len = 0,
    .vsync_len = 0,

    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_24BITS,
    .out_format = OUT_FORMAT_RGB888,
    .tft = {
        .even_line_order = ORDER_GRB,
        .odd_line_order = ORDER_GRB,
        .pix_clk_polarity = AT_FALLING_EDGE,
        .de_active_level = AT_HIGH_LEVEL,
        .hsync_active_level = AT_LOW_LEVEL,
        .vsync_active_level = AT_LOW_LEVEL,
    },
    .power_on = jlt7003a_power_on,
    .power_off = jlt7003a_power_off,
};

static int __init jlt7003a_lcd_init(void)
{
    int ret;

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        jlt7003a_regulator = regulator_get(NULL, lcd_regulator_name);
        if(!jlt7003a_regulator) {
            printk(KERN_ERR "jlt7003a: jlt7003a_regulator get err!\n");
            return -EINVAL;
        }
    }

    ret = m_gpio_request(gpio_lcd_power_en, "jlt7003a power_en");
    if (ret < 0)
        goto err_power_en;

    ret = m_gpio_request(gpio_lcd_rst, "jlt7003a lcd_reset");
    if (ret < 0)
        goto err_reset;

    ret = m_gpio_request(gpio_lcd_backlight_en, "gpio_lcd_backlight_en");
    if(ret)
        goto err_backlight;

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret < 0)
        goto err_register;

    return 0;

err_register:
    m_gpio_free(gpio_lcd_backlight_en);
err_backlight:
    m_gpio_free(gpio_lcd_rst);
err_reset:
    m_gpio_free(gpio_lcd_power_en);
err_power_en:
    if (jlt7003a_regulator)
        regulator_put(jlt7003a_regulator);

    return ret;
}

static void __exit jlt7003a_lcd_exit(void)
{
    m_gpio_free(gpio_lcd_power_en);
    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);

    if (jlt7003a_regulator)
        regulator_put(jlt7003a_regulator);

    jzfb_unregister_lcd(&lcdc_data);
}

module_init(jlt7003a_lcd_init);
module_exit(jlt7003a_lcd_exit);

MODULE_DESCRIPTION("jlt7003a Series Driver");
MODULE_LICENSE("GPL");

