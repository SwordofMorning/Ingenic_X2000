#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <utils/gpio.h>
#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>
#include <linux/regulator/consumer.h>

#define I2C_ADDR            0x2D
#define LCD_DRIVER_NAME     "MG1561B01"

static int gpio_lcd_power_en         = -1;  /* PC13 */
static int power_valid_level         = -1;
static int gpio_lcd_rst              = -1;  /* PD03 */
static int i2c_bus_num               = -1;  /* I2C4 PD00 PD01 */
static char *gpio_lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param(power_valid_level, int, 0644);
module_param_named(gpio_lcd_regulator_name, gpio_lcd_regulator_name, charp, 0644);
module_param_named(mg1561b01_i2c_bus_num, i2c_bus_num, int, 0664);
static struct regulator *mg1561b01_regulator = NULL;

static struct i2c_client *i2c_client;

#if 1
#define FPS    45
#define HACT   1920
#define VACT   1080

#define HFP    88
#define HBP    148
#define HS     44

#define VFP    4
#define VBP    36
#define VS     5
#endif

#include "lt9211.c"

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "lcd_lt9211: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }

    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}

static int MG1561B01_gpio_request(void)
{
    int ret;
    if (strcmp("-1", gpio_lcd_regulator_name) && strlen(gpio_lcd_regulator_name)) {
        mg1561b01_regulator = regulator_get(NULL, gpio_lcd_regulator_name);
        if(!mg1561b01_regulator) {
            printk(KERN_ERR "MG1561B01: mg1561b01_regulator get err!\n");
            goto err_regulator;
        }
    }

    ret = m_gpio_request(gpio_lcd_power_en, "mg1561b01 power_en");
    if (ret < 0)
        goto err_power_en;

    ret = m_gpio_request(gpio_lcd_rst, "mg1561b01 lcd_reset");
    if (ret < 0)
        goto err_reset;

    return 0;

err_reset:
    m_gpio_free(gpio_lcd_power_en);
err_power_en:
    if (mg1561b01_regulator)
        regulator_put(mg1561b01_regulator);
err_regulator:

    return ret;
}

int lt9211_init;
static int MG1561B01_power_on(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en != -1) {
        gpio_direction_output(gpio_lcd_power_en, !power_valid_level);
        msleep(50);
        gpio_direction_output(gpio_lcd_power_en, !!power_valid_level);
    }

    if (mg1561b01_regulator) {
        regulator_enable(mg1561b01_regulator);
    }

    msleep(100);

    if (gpio_lcd_rst != -1) {
        gpio_direction_output(gpio_lcd_rst, 1);
        msleep(50);
        gpio_direction_output(gpio_lcd_rst, 0);
        msleep(200);
        gpio_direction_output(gpio_lcd_rst, 1);
        msleep(50);
    }

    return 0;
}

static int MG1561B01_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en != -1)
        gpio_direction_output(gpio_lcd_power_en, !power_valid_level);

    if (mg1561b01_regulator)
        regulator_disable(mg1561b01_regulator);

    msleep(100);
    if (gpio_lcd_rst != -1)
        gpio_direction_output(gpio_lcd_rst, 0);

    lt9211_init = 0;

    return 0;
}

static void MIPI2LVDS_pan_display_cb(void)
{
    if (lt9211_init)
        return;

    // usleep_range(100*1000, 100*1000);
    LT9211_MIPI2LVDS_Config(i2c_client);
    lt9211_init = 1;
}

static struct lcdc_data lcdc_data = {
    .name = "lt9211-MG1561B01",
    .refresh = FPS,
    .xres = HACT,
    .yres = VACT,
    .pixclock = 0,
    .left_margin = HBP,
    .right_margin = HFP,
    .upper_margin = VBP,
    .lower_margin = VFP,
    .hsync_len = HS,
    .vsync_len = VS,

    .fb_fmt = fb_fmt_RGB888,
    .lcd_mode = TFT_MIPI,
    .out_format = OUT_FORMAT_RGB888,

    .mipi = {
        .num_of_lanes = Lane_Num,
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

    .pan_display_cb = MIPI2LVDS_pan_display_cb,
    .power_on = MG1561B01_power_on,
    .power_off = MG1561B01_power_off,
};

static const struct i2c_device_id MG1561B01_lcd_id[] = {
    { LCD_DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, MG1561B01_lcd_id);

static struct i2c_board_info lcd_MG1561B01_info = {
    .type = LCD_DRIVER_NAME,
    .addr = I2C_ADDR,
};

static int MG1561B01_lcd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;

    ret = MG1561B01_gpio_request();
    if (ret < 0)
        return ret;

    i2c_client = client;

    jzfb_register_lcd(&lcdc_data);

    return 0;
}

static int MG1561B01_lcd_remove(struct i2c_client *client)
{
    m_gpio_free(gpio_lcd_power_en);
    m_gpio_free(gpio_lcd_rst);

    if (mg1561b01_regulator)
        regulator_put(mg1561b01_regulator);

    i2c_client = NULL;

    jzfb_unregister_lcd(&lcdc_data);

    return 0;
}

static struct i2c_driver MG1561B01_lcd_driver = {
    .driver = {
        .name  = LCD_DRIVER_NAME,
        .owner = THIS_MODULE,
    },
    .probe    = MG1561B01_lcd_probe,
    .remove   = MG1561B01_lcd_remove,
    .id_table = MG1561B01_lcd_id,
};

static int __init MG1561B01_lcd_init(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "MG1561B01_lcd: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&MG1561B01_lcd_driver);
    if (ret) {
        printk(KERN_ERR "MG1561B01_lcd: failed to register i2c driver\n");
        return ret;
    }

    i2c_client = i2c_register_device(&lcd_MG1561B01_info, i2c_bus_num);
    if (i2c_client == NULL) {
        printk(KERN_ERR "MG1561B01_lcd: failed to register i2c device\n");
        i2c_del_driver(&MG1561B01_lcd_driver);
        return -EINVAL;
    }

    return 0;
}

static void __exit MG1561B01_lcd_exit(void)
{
    i2c_unregister_device(i2c_client);

    i2c_del_driver(&MG1561B01_lcd_driver);
}

module_init(MG1561B01_lcd_init);
module_exit(MG1561B01_lcd_exit);

MODULE_DESCRIPTION("MG1561B01 Series Driver");
MODULE_LICENSE("GPL");
