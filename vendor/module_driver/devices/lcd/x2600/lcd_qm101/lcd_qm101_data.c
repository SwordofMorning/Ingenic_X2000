#include <linux/delay.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>

#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>

static char *lcd_regulator_name = "";//-1

module_param(lcd_regulator_name, charp, 0644);

static struct regulator *qm101_regulator = NULL;

static int qm101_power_on(struct lcdc *lcdc)
{
    if (qm101_regulator)
        regulator_enable(qm101_regulator);

    return 0;
}

static int qm101_power_off(struct lcdc *lcdc)
{
    if (qm101_regulator)
        regulator_disable(qm101_regulator);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "qm101",
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
    .out_format = OUT_FORMAT_RGB666,

    .lvds = {
        .mapping_mode = VESA,
        .data_width = DATA_18_BPP,
        .data_en_polarity = AT_RISING_EDGE,
        .hsync_active_level = AT_HIGH_LEVEL,
        .vsync_active_level = AT_HIGH_LEVEL,
    },
    .height = 164,
    .width = 100,

    .power_on = qm101_power_on,
    .power_off = qm101_power_off,
};

static int lcd_qm101_init(void)
{
    int ret;

    if (strcmp("-1", lcd_regulator_name) && strlen(lcd_regulator_name)) {
        qm101_regulator = regulator_get(NULL, lcd_regulator_name);
        if (!qm101_regulator) {
            printk(KERN_ERR "lcd_regulator get err!\n");
            return -EINVAL;
        }
    }

    ret = jzfb_register_lcd(&lcdc_data);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

static void lcd_qm101_exit(void)
{
    jzfb_unregister_lcd(&lcdc_data);
}

module_init(lcd_qm101_init);
module_exit(lcd_qm101_exit);

MODULE_DESCRIPTION("Ingenic Soc qm101_lcd driver");
MODULE_LICENSE("GPL");