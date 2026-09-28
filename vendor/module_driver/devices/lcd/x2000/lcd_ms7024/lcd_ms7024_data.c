#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <utils/gpio.h>
#include <fb/lcdc_data.h>
#include <mipi_dsi/jz_mipi_dsi.h>
#include <linux/regulator/consumer.h>

#define I2C_ADDR          0x76
#define LCD_DRIVER_NAME   "ms7024"

static int gpio_lcd_power_en         = -1;
static int power_valid_level         = -1;
static int gpio_lcd_rst              = -1;  /* PC05 */
static int i2c_bus_num               = -1;  /* I2C4 PC25 PC26 */
static char *gpio_lcd_regulator_name = "";

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param(power_valid_level, int, 0644);
module_param_named(gpio_lcd_regulator_name, gpio_lcd_regulator_name, charp, 0644);
module_param_named(ms7024_i2c_bus_num, i2c_bus_num, int, 0664);
static struct regulator *ms7024_regulator = NULL;

static struct i2c_client *i2c_client;

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
        printk(KERN_ERR "lcd_ms7024: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }

    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}

static int ms7024_i2c_write(struct i2c_client *client, unsigned char reg, unsigned char value)
{
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr   = client->addr,
        .buf    = buf,
        .len    = 2,
        .flags  = 0,
    };

    int ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}

static void ms7024_i2c_init(struct i2c_client *client)
{
#if 0
    //彩条测试
    ms7024_i2c_write(client, 0x04, 0x18);
    ms7024_i2c_write(client, 0x0e, 0x16);
    ms7024_i2c_write(client, 0x0f, 0x00);
    ms7024_i2c_write(client, 0x30, 0x02);
    ms7024_i2c_write(client, 0x31, 0x4a);
    ms7024_i2c_write(client, 0x32, 0x03);
    ms7024_i2c_write(client, 0x33, 0x7a);
    ms7024_i2c_write(client, 0x34, 0x00);
    ms7024_i2c_write(client, 0x35, 0x07);
    ms7024_i2c_write(client, 0x36, 0x02);
    ms7024_i2c_write(client, 0x37, 0x27);
    ms7024_i2c_write(client, 0x38, 0x00);
    ms7024_i2c_write(client, 0x39, 0x00);
    ms7024_i2c_write(client, 0x3a, 0x00);
    ms7024_i2c_write(client, 0x3b, 0x00);
    ms7024_i2c_write(client, 0x3c, 0x00);
    ms7024_i2c_write(client, 0x90, 0x02);
    ms7024_i2c_write(client, 0x91, 0x00);
    ms7024_i2c_write(client, 0x92, 0x00);
    ms7024_i2c_write(client, 0x93, 0x00);
    ms7024_i2c_write(client, 0x94, 0x00);
    ms7024_i2c_write(client, 0x95, 0x00);
    ms7024_i2c_write(client, 0x96, 0x00);
    ms7024_i2c_write(client, 0x97, 0x00);
    ms7024_i2c_write(client, 0x98, 0x00);
    ms7024_i2c_write(client, 0x99, 0x00);
    ms7024_i2c_write(client, 0x9a, 0x00);
    ms7024_i2c_write(client, 0x9b, 0x00);
    ms7024_i2c_write(client, 0x9c, 0x00);
    ms7024_i2c_write(client, 0x9d, 0x00);
    ms7024_i2c_write(client, 0x9e, 0x00);
    ms7024_i2c_write(client, 0x9d, 0x00);
    ms7024_i2c_write(client, 0x9e, 0x00);
    ms7024_i2c_write(client, 0x9f, 0x00);
    ms7024_i2c_write(client, 0xa0, 0x00);
    ms7024_i2c_write(client, 0xa1, 0x00);
    ms7024_i2c_write(client, 0xa2, 0x00);
    ms7024_i2c_write(client, 0xa4, 0x00);
    ms7024_i2c_write(client, 0xa5, 0x00);
    ms7024_i2c_write(client, 0xa6, 0x00);
    ms7024_i2c_write(client, 0xa7, 0x00);
    ms7024_i2c_write(client, 0xa8, 0x00);
    ms7024_i2c_write(client, 0xa9, 0x07);
    ms7024_i2c_write(client, 0xaa, 0x02);
    ms7024_i2c_write(client, 0xab, 0x05);
    ms7024_i2c_write(client, 0xac, 0x15);
    ms7024_i2c_write(client, 0xad, 0x89);
    ms7024_i2c_write(client, 0x50, 0x00);
    ms7024_i2c_write(client, 0x51, 0x11);
    ms7024_i2c_write(client, 0x52, 0x0b);
    ms7024_i2c_write(client, 0x53, 0x01);
    ms7024_i2c_write(client, 0x54, 0x00);
    ms7024_i2c_write(client, 0x55, 0x00);
    ms7024_i2c_write(client, 0x56, 0x00);
    ms7024_i2c_write(client, 0x57, 0x00);
    ms7024_i2c_write(client, 0x58, 0x00);
    ms7024_i2c_write(client, 0x59, 0xc5);
    ms7024_i2c_write(client, 0x5a, 0x00);
    ms7024_i2c_write(client, 0x20, 0x59);
    ms7024_i2c_write(client, 0x21, 0x08);
    ms7024_i2c_write(client, 0x22, 0x63);
    ms7024_i2c_write(client, 0x23, 0x01);
    ms7024_i2c_write(client, 0x24, 0x00);
    ms7024_i2c_write(client, 0x25, 0x00);
    ms7024_i2c_write(client, 0x26, 0x00);
    ms7024_i2c_write(client, 0x27, 0xc1);
    ms7024_i2c_write(client, 0x28, 0xc1);
    ms7024_i2c_write(client, 0x29, 0x80);
    ms7024_i2c_write(client, 0x2a, 0x84);
    ms7024_i2c_write(client, 0x2b, 0x00);
    ms7024_i2c_write(client, 0x2c, 0x00);
    ms7024_i2c_write(client, 0x2d, 0x00);
    ms7024_i2c_write(client, 0x60, 0x03);
    ms7024_i2c_write(client, 0x61, 0x00);
    ms7024_i2c_write(client, 0x62, 0x01);
    ms7024_i2c_write(client, 0x63, 0x00);
    ms7024_i2c_write(client, 0x64, 0x20);
    ms7024_i2c_write(client, 0x66, 0x00);
    ms7024_i2c_write(client, 0x67, 0x40);
    ms7024_i2c_write(client, 0x68, 0x00);
    ms7024_i2c_write(client, 0x69, 0x20);
    ms7024_i2c_write(client, 0x6a, 0x40);
    ms7024_i2c_write(client, 0x6b, 0x60);
    ms7024_i2c_write(client, 0x6c, 0x80);
    ms7024_i2c_write(client, 0x6d, 0xa0);
    ms7024_i2c_write(client, 0x6e, 0xc0);
    ms7024_i2c_write(client, 0x6f, 0xe0);
    ms7024_i2c_write(client, 0x70, 0xff);
    ms7024_i2c_write(client, 0x71, 0x03);
    ms7024_i2c_write(client, 0x72, 0x4b);
    ms7024_i2c_write(client, 0x73, 0x40);
    ms7024_i2c_write(client, 0x74, 0x40);
    ms7024_i2c_write(client, 0x75, 0x40);
    ms7024_i2c_write(client, 0x76, 0x40);
    ms7024_i2c_write(client, 0x77, 0x5b);
    ms7024_i2c_write(client, 0x78, 0x5b);
    ms7024_i2c_write(client, 0x79, 0x5b);
    ms7024_i2c_write(client, 0x7a, 0x5b);
    ms7024_i2c_write(client, 0x7b, 0x02);
    ms7024_i2c_write(client, 0x7c, 0x8c);
    ms7024_i2c_write(client, 0x7d, 0xd4);
    ms7024_i2c_write(client, 0x7e, 0x72);
    ms7024_i2c_write(client, 0x7f, 0x00);
    ms7024_i2c_write(client, 0x80, 0x00);
    ms7024_i2c_write(client, 0x81, 0x00);
    ms7024_i2c_write(client, 0x82, 0x04);
    ms7024_i2c_write(client, 0x83, 0x00);
    ms7024_i2c_write(client, 0x84, 0xff);
    ms7024_i2c_write(client, 0x85, 0xce);
    ms7024_i2c_write(client, 0x86, 0xb2);
    ms7024_i2c_write(client, 0x87, 0x00);
    ms7024_i2c_write(client, 0x88, 0x00);
    ms7024_i2c_write(client, 0x89, 0x93);
    ms7024_i2c_write(client, 0x8a, 0x06);
    ms7024_i2c_write(client, 0x5f, 0x01);
    ms7024_i2c_write(client, 0x2e, 0x10);
    ms7024_i2c_write(client, 0x20, 0x56);
    ms7024_i2c_write(client, 0x2e, 0x11);
    ms7024_i2c_write(client, 0x20, 0x50);
    m_msleep(10);
    ms7024_i2c_write(client, 0x20, 0x59);
    m_msleep(10);
    ms7024_i2c_write(client, 0x20, 0x56);
    ms7024_i2c_write(client, 0x20, 0x50);
    m_msleep(10);
    ms7024_i2c_write(client, 0x20, 0x59);
    m_msleep(100);
    ms7024_i2c_write(client, 0x05, 0x07);
    ms7024_i2c_write(client, 0x06, 0x0f);
#else
    ms7024_i2c_write(client, 0x04, 0x18);
    ms7024_i2c_write(client, 0x0e, 0x16);
    ms7024_i2c_write(client, 0x0f, 0x00);
    ms7024_i2c_write(client, 0x30, 0x02);
    ms7024_i2c_write(client, 0x31, 0x4a);
    ms7024_i2c_write(client, 0x31, 0x4a);
    ms7024_i2c_write(client, 0x32, 0x03);//(0x32<<8)+0x31:H Blank start值增大右侧图像向右扩展,值减小右侧图像向左缩放
    ms7024_i2c_write(client, 0x33, 0x50);//H blank stop值增大整体图像左移,值减小整体图像右移
    ms7024_i2c_write(client, 0x34, 0x00);
    ms7024_i2c_write(client, 0x35, 0xf0);
    ms7024_i2c_write(client, 0x36, 0x01);//(0x36<<8)+0x35:V Blank start值增大整体图像上移,值减小整体图像下移
    ms7024_i2c_write(client, 0x37, 0x13);//V blank odd stop值增大上侧图像向下缩放,值减小上侧图像向上扩展
    ms7024_i2c_write(client, 0x38, 0x00);
    ms7024_i2c_write(client, 0x39, 0x00);
    ms7024_i2c_write(client, 0x3a, 0x00);
    ms7024_i2c_write(client, 0x3b, 0x00);
    ms7024_i2c_write(client, 0x3c, 0x00);
    ms7024_i2c_write(client, 0x90, 0x02);
    ms7024_i2c_write(client, 0x91, 0x00);
    ms7024_i2c_write(client, 0x92, 0x00);
    ms7024_i2c_write(client, 0x93, 0x00);
    ms7024_i2c_write(client, 0x94, 0x00);
    ms7024_i2c_write(client, 0x95, 0x00);
    ms7024_i2c_write(client, 0x96, 0x00);
    ms7024_i2c_write(client, 0x97, 0x00);
    ms7024_i2c_write(client, 0x98, 0x00);
    ms7024_i2c_write(client, 0x99, 0x00);
    ms7024_i2c_write(client, 0x9a, 0x00);
    ms7024_i2c_write(client, 0x9b, 0x00);
    ms7024_i2c_write(client, 0x9c, 0x00);
    ms7024_i2c_write(client, 0x9d, 0x00);
    ms7024_i2c_write(client, 0x9e, 0x00);
    ms7024_i2c_write(client, 0x9d, 0x00);
    ms7024_i2c_write(client, 0x9e, 0x00);
    ms7024_i2c_write(client, 0x9f, 0x00);
    ms7024_i2c_write(client, 0xa0, 0x00);
    ms7024_i2c_write(client, 0xa1, 0x00);
    ms7024_i2c_write(client, 0xa2, 0x00);
    ms7024_i2c_write(client, 0xa4, 0x00);
    ms7024_i2c_write(client, 0xa5, 0x00);
    ms7024_i2c_write(client, 0xa6, 0x00);
    ms7024_i2c_write(client, 0xa7, 0x00);
    ms7024_i2c_write(client, 0xa8, 0x00);
    ms7024_i2c_write(client, 0xa9, 0x07);
    ms7024_i2c_write(client, 0xaa, 0x02);
    ms7024_i2c_write(client, 0xab, 0x05);
    ms7024_i2c_write(client, 0xac, 0x15);
    ms7024_i2c_write(client, 0xad, 0x89);
    ms7024_i2c_write(client, 0x50, 0x00);
    ms7024_i2c_write(client, 0x51, 0x11);
    ms7024_i2c_write(client, 0x52, 0x0b);
    ms7024_i2c_write(client, 0x53, 0x01);
    ms7024_i2c_write(client, 0x54, 0x00);
    ms7024_i2c_write(client, 0x55, 0x00);
    ms7024_i2c_write(client, 0x56, 0x00);
    ms7024_i2c_write(client, 0x57, 0x00);
    ms7024_i2c_write(client, 0x58, 0x00);
    ms7024_i2c_write(client, 0x59, 0x00);
    ms7024_i2c_write(client, 0x5a, 0x00);
    ms7024_i2c_write(client, 0x20, 0x59);
    ms7024_i2c_write(client, 0x21, 0x08);
    ms7024_i2c_write(client, 0x22, 0x63);
    ms7024_i2c_write(client, 0x23, 0x01);
    ms7024_i2c_write(client, 0x24, 0x00);
    ms7024_i2c_write(client, 0x25, 0x00);
    ms7024_i2c_write(client, 0x26, 0x00);
    ms7024_i2c_write(client, 0x27, 0xc1);
    ms7024_i2c_write(client, 0x28, 0xc1);
    ms7024_i2c_write(client, 0x29, 0x80);
    ms7024_i2c_write(client, 0x2a, 0x84);
    ms7024_i2c_write(client, 0x2b, 0x00);
    ms7024_i2c_write(client, 0x2c, 0x00);
    ms7024_i2c_write(client, 0x2d, 0x00);
    ms7024_i2c_write(client, 0x60, 0x03);
    ms7024_i2c_write(client, 0x61, 0x00);
    ms7024_i2c_write(client, 0x62, 0x01);
    ms7024_i2c_write(client, 0x63, 0x00);
    ms7024_i2c_write(client, 0x64, 0x20);
    ms7024_i2c_write(client, 0x66, 0x00);
    ms7024_i2c_write(client, 0x67, 0x40);
    ms7024_i2c_write(client, 0x68, 0x00);
    ms7024_i2c_write(client, 0x69, 0x20);
    ms7024_i2c_write(client, 0x6a, 0x40);
    ms7024_i2c_write(client, 0x6b, 0x60);
    ms7024_i2c_write(client, 0x6c, 0x80);
    ms7024_i2c_write(client, 0x6d, 0xa0);
    ms7024_i2c_write(client, 0x6e, 0xc0);
    ms7024_i2c_write(client, 0x6f, 0xe0);
    ms7024_i2c_write(client, 0x70, 0xff);
    ms7024_i2c_write(client, 0x71, 0x03);
    ms7024_i2c_write(client, 0x72, 0x4b);
    ms7024_i2c_write(client, 0x73, 0x40);
    ms7024_i2c_write(client, 0x74, 0x40);
    ms7024_i2c_write(client, 0x75, 0x40);
    ms7024_i2c_write(client, 0x76, 0x40);
    ms7024_i2c_write(client, 0x77, 0x5b);
    ms7024_i2c_write(client, 0x78, 0x5b);
    ms7024_i2c_write(client, 0x79, 0x5b);
    ms7024_i2c_write(client, 0x7a, 0x5b);
    ms7024_i2c_write(client, 0x7b, 0x02);
    ms7024_i2c_write(client, 0x7c, 0x8c);
    ms7024_i2c_write(client, 0x7d, 0xd4);
    ms7024_i2c_write(client, 0x7e, 0x72);
    ms7024_i2c_write(client, 0x7f, 0x00);
    ms7024_i2c_write(client, 0x80, 0x00);
    ms7024_i2c_write(client, 0x81, 0x00);
    ms7024_i2c_write(client, 0x82, 0x04);
    ms7024_i2c_write(client, 0x83, 0x00);
    ms7024_i2c_write(client, 0x84, 0xff);
    ms7024_i2c_write(client, 0x85, 0xce);
    ms7024_i2c_write(client, 0x86, 0xb2);
    ms7024_i2c_write(client, 0x87, 0x00);
    ms7024_i2c_write(client, 0x88, 0x00);
    ms7024_i2c_write(client, 0x89, 0x93);
    ms7024_i2c_write(client, 0x8a, 0x06);
    ms7024_i2c_write(client, 0x5f, 0x01);
    ms7024_i2c_write(client, 0x2e, 0x10);
    ms7024_i2c_write(client, 0x20, 0x56);
    ms7024_i2c_write(client, 0x2e, 0x11);
    ms7024_i2c_write(client, 0x20, 0x50);
    m_msleep(10);
    ms7024_i2c_write(client, 0x20, 0x59);
    m_msleep(10);
    ms7024_i2c_write(client, 0x20, 0x56);
    ms7024_i2c_write(client, 0x20, 0x50);
    m_msleep(10);
    ms7024_i2c_write(client, 0x20, 0x59);
    m_msleep(100);
    ms7024_i2c_write(client, 0x05, 0x07);
    ms7024_i2c_write(client, 0x06, 0x0f);
#endif
}

static int ms7024_gpio_request(void)
{
    int ret;
    if (strcmp("-1", gpio_lcd_regulator_name) && strlen(gpio_lcd_regulator_name)) {
        ms7024_regulator = regulator_get(NULL, gpio_lcd_regulator_name);
        if(!ms7024_regulator) {
            printk(KERN_ERR "ms7024: ms7024_regulator get err!\n");
            goto err_regulator;
        }
    }

    ret = m_gpio_request(gpio_lcd_power_en, "ms7024 power_en");
    if (ret < 0)
        goto err_power_en;

    ret = m_gpio_request(gpio_lcd_rst, "ms7024 lcd_reset");
    if (ret < 0)
        goto err_reset;

    return 0;

err_reset:
    m_gpio_free(gpio_lcd_power_en);
err_power_en:
    if (ms7024_regulator)
        regulator_put(ms7024_regulator);
err_regulator:

    return ret;
}

static int ms7024_power_on(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en != -1) {
        gpio_direction_output(gpio_lcd_power_en, !power_valid_level);
        msleep(50);
        gpio_direction_output(gpio_lcd_power_en, !!power_valid_level);
    }

    if (ms7024_regulator) {
        regulator_enable(ms7024_regulator);
    }

    if (gpio_lcd_rst != -1) {
        gpio_direction_output(gpio_lcd_rst, 0);
        m_msleep(10);
        gpio_direction_output(gpio_lcd_rst, 1);
    }

    ms7024_i2c_init(i2c_client);

    return 0;
}

static int ms7024_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en  != -1)
        gpio_direction_output(gpio_lcd_power_en, !power_valid_level);

    if (ms7024_regulator)
        regulator_disable(ms7024_regulator);

    if (gpio_lcd_rst != -1)
        gpio_direction_output(gpio_lcd_rst, 0);

    return 0;
}

static struct lcdc_data lcdc_data = {
    .name = "ms7024",
    .refresh = 60,
    .xres = 720,
    .yres = 480,
    .pixclock = 0,
    .left_margin = 16,
    .right_margin = 60,
    .upper_margin = 9,
    .lower_margin = 30,
    .hsync_len = 62,
    .vsync_len = 6,

    .fb_fmt = fb_fmt_RGB888,
    .lcd_mode = TFT_24BITS,
    .out_format = OUT_FORMAT_RGB888,
    .tft = {
        .even_line_order = ORDER_RGB,
        .odd_line_order = ORDER_RGB,
        .pix_clk_polarity = AT_FALLING_EDGE,
        .de_active_level = AT_HIGH_LEVEL,
        .hsync_vsync_active_level = AT_LOW_LEVEL,
    },
    .power_on = ms7024_power_on,
    .power_off = ms7024_power_off,
};

static const struct i2c_device_id ms7024_lcd_id[] = {
    { LCD_DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ms7024_lcd_id);

static struct i2c_board_info lcd_ms7024_info = {
    .type = LCD_DRIVER_NAME,
    .addr = I2C_ADDR,
};

static int ms7024_lcd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;

    ret = ms7024_gpio_request();
    if (ret < 0)
        return ret;

    i2c_client = client;

    jzfb_register_lcd(&lcdc_data);

    return 0;
}

static int ms7024_lcd_remove(struct i2c_client *client)
{
    m_gpio_free(gpio_lcd_power_en);
    m_gpio_free(gpio_lcd_rst);

    if (ms7024_regulator)
        regulator_put(ms7024_regulator);

    i2c_client = NULL;

    jzfb_unregister_lcd(&lcdc_data);

    return 0;
}

static struct i2c_driver ms7024_lcd_driver = {
    .driver = {
        .name  = LCD_DRIVER_NAME,
        .owner = THIS_MODULE,
    },
    .probe    = ms7024_lcd_probe,
    .remove   = ms7024_lcd_remove,
    .id_table = ms7024_lcd_id,
};

static int __init ms7024_lcd_init(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ms7024_lcd: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&ms7024_lcd_driver);
    if (ret) {
        printk(KERN_ERR "ms7024_lcd: failed to register i2c driver\n");
        return ret;
    }

    i2c_client = i2c_register_device(&lcd_ms7024_info, i2c_bus_num);
    if (i2c_client == NULL) {
        printk(KERN_ERR "ms7024_lcd: failed to register i2c device\n");
        i2c_del_driver(&ms7024_lcd_driver);
        return -EINVAL;
    }

    return 0;
}

static void __exit ms7024_lcd_exit(void)
{
    i2c_unregister_device(i2c_client);

    i2c_del_driver(&ms7024_lcd_driver);
}

module_init(ms7024_lcd_init);
module_exit(ms7024_lcd_exit);

MODULE_DESCRIPTION("Ms7024 Series Driver");
MODULE_LICENSE("GPL");
