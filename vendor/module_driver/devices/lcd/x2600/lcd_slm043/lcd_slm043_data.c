#include <utils/gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <fb/lcdc_data.h>

static int gpio_lcd_power_en = -1;

module_param_gpio(gpio_lcd_power_en, 0644);

static inline void m_msleep(int ms)
{
    usleep_range(ms * 1000, ms * 1000);
}

static int slm043_power_on(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0) {
        gpio_direction_output(gpio_lcd_power_en, 1);
        m_msleep(180);
    }

    return 0;
}

static int slm043_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        gpio_direction_output(gpio_lcd_power_en, 0);

    return 0;
}

struct lcdc_data lcdc_data = {
    .name = "slm043",
    .refresh = 45,
    .xres = 480,
    .yres = 272,
    .pixclock = 0, // 自动计算
    .left_margin = 43,
    .right_margin = 8,
    .upper_margin = 12,
    .lower_margin = 8,
    .hsync_len = 4,
    .vsync_len = 4,
    .fb_fmt = fb_fmt_RGB888,
    .lcd_mode = TFT_8BITS_DUMMY_SERIAL,
    .out_format = OUT_FORMAT_RGB888,
    .tft = {
        .even_line_order = ORDER_RGB,
        .odd_line_order = ORDER_RGB,
        .pix_clk_polarity = AT_FALLING_EDGE,
        .de_active_level = AT_HIGH_LEVEL,
        .hsync_active_level = AT_LOW_LEVEL,
        .vsync_active_level = AT_LOW_LEVEL,
    },
    .power_on = slm043_power_on,
    .power_off = slm043_power_off,
};

static int __init slm043_init(void)
{
    int ret;

    if (gpio_lcd_power_en >= 0) {
        ret = gpio_request(gpio_lcd_power_en, "lcd_power_en");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "slm043: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_power_en, buf));
            return ret;
        }
    }

    jzfb_register_lcd(&lcdc_data);

    return 0;
}

static void __exit slm043_exit(void)
{
    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
}

module_init(slm043_init);
module_exit(slm043_exit);

MODULE_DESCRIPTION("slm043 lcd panel driver");
MODULE_LICENSE("GPL");