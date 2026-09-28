#include <stdio.h>
#include <soc/base.h>
#include <delay.h>
#include <driver/systick.h>
#include <driver/gpio.h>
#include <errno.h>

#include <driver/fb.h>

struct gpio_pin {
    int pin;
    int level; // active level
};

struct gpio_pin gpio_lcd_power = {
#ifdef APP_libmcu_device_x2600_lcd_kx070
    .pin = APP_libmcu_device_x2600_kx070_power_pin,    //pc15
    .level = APP_libmcu_device_x2600_kx070_power_level   // 1
#endif
};

struct gpio_pin gpio_lcd_power_en = {
#ifdef APP_libmcu_device_x2600_lcd_kx070
    .pin = APP_libmcu_device_x2600_kx070_power_en_pin,    //pc16
    .level = APP_libmcu_device_x2600_kx070_power_en_level   // 0
#endif
};

struct gpio_pin gpio_lcd_rst = {
#ifdef APP_libmcu_device_x2600_lcd_kx070
    .pin = APP_libmcu_device_x2600_kx070_rst_pin,   //pc18
    .level = APP_libmcu_device_x2600_kx070_rst_level //0
#endif
};

struct gpio_pin gpio_lcd_backlight = {
#ifdef APP_libmcu_device_x2600_lcd_kx070
    .pin = APP_libmcu_device_x2600_kx070_backlight_pin,  //pc08
    .level = APP_libmcu_device_x2600_kx070_backlight_level //1
#endif
};

static inline void m_gpio_direction_output(int gpio, int value)
{
    if (gpio < 0)
        return;

    gpio_set_func(gpio, value ? GPIO_OUTPUT1 : GPIO_OUTPUT0);
}

static int kx070_power_on(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en.pin, gpio_lcd_power_en.level);
    m_gpio_direction_output(gpio_lcd_power.pin, gpio_lcd_power.level);
    m_gpio_direction_output(gpio_lcd_backlight.pin, gpio_lcd_backlight.level);
    // mdelay(50);

    // m_gpio_direction_output(gpio_lcd_rst.pin, !gpio_lcd_rst.level);
    // mdelay(30);
    // m_gpio_direction_output(gpio_lcd_rst.pin, gpio_lcd_rst.level);
    // mdelay(10);
    // m_gpio_direction_output(gpio_lcd_rst.pin, !gpio_lcd_rst.level);
    // mdelay(120);

    return 0;
}

static int kx070_power_off(struct lcdc *lcdc)
{
    m_gpio_direction_output(gpio_lcd_power_en.pin, !gpio_lcd_power_en.level);

    m_gpio_direction_output(gpio_lcd_power.pin, !gpio_lcd_power.level);

    m_gpio_direction_output(gpio_lcd_backlight.pin, !gpio_lcd_backlight.level);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "kx070",
    // .refresh = 0,
    .xres = 1024,
    .yres = 600,
    .pixclock = 51000000,
    .left_margin = 160,
    .right_margin = 160,
    .upper_margin = 23,
    .lower_margin = 12,
    .hsync_len = 19,
    .vsync_len = 5,

    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = LVDS_VESA,
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
    .height = 164,
    .width = 100,

    .power_on = kx070_power_on,
    .power_off = kx070_power_off,
};

int lcd_kx070_init(void)
{
    int ret;

    ret = fb_register_lcd(&lcdc_data);
    if (ret < 0) {
        printf("kx070: register lcd failed!\n");
        return -1;
    }

    return 0;
}

void lcd_kx070_exit(void)
{
    fb_unregister_lcd(&lcdc_data);
}
