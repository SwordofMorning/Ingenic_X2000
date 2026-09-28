#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <utils/gpio.h>
#include <utils/gpio.h>
#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>
#include <linux/regulator/consumer.h>


#define LCD_DRIVER_NAME   "gm8285c"

static int gpio_lcd_power_en         = -1;
static int power_valid_level         = -1;
static int gpio_lcd_rst              = -1;

static char *gpio_lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param(power_valid_level, int, 0644);
module_param_named(gpio_lcd_regulator_name, gpio_lcd_regulator_name, charp, 0644);
static struct regulator *gm8285c_regulator = NULL;

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
        printk(KERN_ERR "lcd_gm8285c: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }

    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}

static int gm8285c_gpio_request(void)
{
    int ret;
    if (strcmp("-1", gpio_lcd_regulator_name) && strlen(gpio_lcd_regulator_name)) {
        gm8285c_regulator = regulator_get(NULL, gpio_lcd_regulator_name);
        if(!gm8285c_regulator) {
            printk(KERN_ERR "gm8285c: gm8285c_regulator get err!\n");
            goto err_regulator;
        }
    }

    ret = m_gpio_request(gpio_lcd_power_en, "gm8285c power_en");
    if (ret < 0)
        goto err_power_en;

    ret = m_gpio_request(gpio_lcd_rst, "gm8285c lcd_reset");
    if (ret < 0)
        goto err_reset;

    return 0;

err_reset:
    m_gpio_free(gpio_lcd_power_en);
err_power_en:
    if (gm8285c_regulator)
        regulator_put(gm8285c_regulator);
err_regulator:

    return ret;
}

static int gm8285c_power_on(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en != -1) {
        gpio_direction_output(gpio_lcd_power_en, !power_valid_level);
        msleep(50);
        gpio_direction_output(gpio_lcd_power_en, !!power_valid_level);
    }

    if (gm8285c_regulator) {
        regulator_enable(gm8285c_regulator);
    }

    if (gpio_lcd_rst != -1) {
        gpio_direction_output(gpio_lcd_rst, 0);
        m_msleep(10);
        gpio_direction_output(gpio_lcd_rst, 1);
    }

    return 0;
}

static int gm8285c_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en  != -1)
        gpio_direction_output(gpio_lcd_power_en, !power_valid_level);

    if (gm8285c_regulator)
        regulator_disable(gm8285c_regulator);

    if (gpio_lcd_rst != -1)
        gpio_direction_output(gpio_lcd_rst, 0);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "gm8285c",
    .refresh = 60,
    .xres = 1024,
    .yres = 768,
    .pixclock = 0,
    .left_margin = 36,
    .right_margin = 100,
    .upper_margin = 39,
    .lower_margin = 80,
    .hsync_len = 62,
    .vsync_len = 6,

    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_24BITS,
    .out_format = OUT_FORMAT_RGB888,
    .tft = {
        .even_line_order = ORDER_RGB,
        .odd_line_order = ORDER_RGB,
        .pix_clk_polarity = AT_FALLING_EDGE,
        .de_active_level = AT_HIGH_LEVEL,
        .hsync_vsync_active_level = AT_LOW_LEVEL,
    },
    .power_on = gm8285c_power_on,
    .power_off = gm8285c_power_off,
};

static int __init gm8285c_lcd_init(void)
{
    int ret;

    ret = gm8285c_gpio_request();

    jzfb_register_lcd(&lcdc_data);
    return 0;
}

static void __exit gm8285c_lcd_exit(void)
{
    m_gpio_free(gpio_lcd_power_en);
    m_gpio_free(gpio_lcd_rst);

    if (gm8285c_regulator)
        regulator_put(gm8285c_regulator);

    jzfb_unregister_lcd(&lcdc_data);
}

module_init(gm8285c_lcd_init);
module_exit(gm8285c_lcd_exit);

MODULE_DESCRIPTION("Gm8285c Series Driver");
MODULE_LICENSE("GPL");
