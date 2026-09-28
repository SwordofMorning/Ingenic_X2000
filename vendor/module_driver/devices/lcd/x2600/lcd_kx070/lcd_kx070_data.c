#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <linux/regulator/consumer.h>

#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>

int gpio_lcd_power_en = -1;     // GPIO_PC(15) 1
int gpio_lcd_rst = -1;          // GPIO_PC(18) 0
int gpio_lcd_backlight_en = -1; // GPIO_PC(08) 1
static char *lcd_regulator_name = "";// GPIO_PC(16) 0

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_backlight_en, 0644);
module_param(lcd_regulator_name, charp, 0644);

static struct regulator *kx070_regulator = NULL;

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "kx070: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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

static int kx070_power_on(struct lcdc *lcdc)
{
    if (kx070_regulator)
        regulator_enable(kx070_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 1);
    m_gpio_direction_output(gpio_lcd_backlight_en, 1);

    return 0;
}

static int kx070_power_off(struct lcdc *lcdc)
{
    if (kx070_regulator)
        regulator_disable(kx070_regulator);

    m_gpio_direction_output(gpio_lcd_power_en, 0);
    m_gpio_direction_output(gpio_lcd_backlight_en, 0);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "kx070",
    .refresh = 60,
    .xres = 1024,
    .yres = 600,
    .left_margin = 160,
    .right_margin = 160,
    .upper_margin = 23,
    .lower_margin = 12,
    .hsync_len = 19,
    .vsync_len = 5,

    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = LVDS,
    .out_format = OUT_FORMAT_RGB888,

    .lvds = {
        .mapping_mode = VESA,
        .data_width = DATA_24_BPP,
        .data_en_polarity = AT_RISING_EDGE,
        .hsync_active_level = AT_HIGH_LEVEL,
        .vsync_active_level = AT_HIGH_LEVEL,
    },
    .height = 164,
    .width = 100,

    .power_on = kx070_power_on,
    .power_off = kx070_power_off,
};

static int lcd_kx070_init(void)
{
    int ret;

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        kx070_regulator = regulator_get(NULL, lcd_regulator_name);
        if (!kx070_regulator) {
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

static void lcd_kx070_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);

    m_gpio_free(gpio_lcd_backlight_en);
    m_gpio_free(gpio_lcd_rst);
    m_gpio_free(gpio_lcd_power_en);
}

module_init(lcd_kx070_init);
module_exit(lcd_kx070_exit);

MODULE_DESCRIPTION("Ingenic Soc kx070_lcd driver");
MODULE_LICENSE("GPL");