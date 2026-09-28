#include <utils/gpio.h>
#include <utils/spi.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>

#include <fb/lcdc_data.h>

static int gpio_lcd_power_en    = -1; // -1
static int gpio_lcd_rst         = -1; // GPIO_PC(12)
static int gpio_lcd_scl         = -1; // GPIO_PB(00)
static int gpio_lcd_sda         = -1; // GPIO_PB(01)
static int gpio_lcd_cs          = -1; // GPIO_PC(06)

module_param_gpio(gpio_lcd_power_en, 0644);
module_param_gpio(gpio_lcd_rst, 0644);
module_param_gpio(gpio_lcd_scl, 0644);
module_param_gpio(gpio_lcd_sda, 0644);
module_param_gpio(gpio_lcd_cs, 0644);


static inline void m_msleep(int ms)
{
    usleep_range(ms * 1000, ms * 1000);
}

void SPI_SendData(unsigned char i)
{
   unsigned char n;

   for(n=0; n<8; n++)
   {
        gpio_direction_output(gpio_lcd_sda, i&0x80);
        gpio_direction_output(gpio_lcd_scl, 0);
        udelay(1);
        gpio_direction_output(gpio_lcd_scl, 1);
        udelay(1);
        i<<=1;
   }
}

void st7701s_tft_SPI_WriteComm(unsigned char i)
{
    gpio_direction_output(gpio_lcd_cs, 0);
    udelay(1);
    gpio_direction_output(gpio_lcd_sda, 0);
    gpio_direction_output(gpio_lcd_scl, 0);
    udelay(1);
    gpio_direction_output(gpio_lcd_scl, 1);
    udelay(1);
    SPI_SendData(i);
    udelay(1);
    gpio_direction_output(gpio_lcd_cs, 1);
    udelay(1);
}

void st7701s_tft_SPI_WriteData(unsigned char i)
{
    gpio_direction_output(gpio_lcd_cs, 0);
    udelay(1);
    gpio_direction_output(gpio_lcd_sda, 1);
    gpio_direction_output(gpio_lcd_scl, 0);
    udelay(1);
    gpio_direction_output(gpio_lcd_scl, 1);
    udelay(1);
    SPI_SendData(i);
    udelay(1);
    gpio_direction_output(gpio_lcd_cs, 1);
    udelay(1);
}

void st7701s_tft_spi_init(void)
{
    gpio_direction_output(gpio_lcd_rst, 0);
    m_msleep(50);
    gpio_direction_output(gpio_lcd_rst, 1);
    m_msleep(80);
    gpio_direction_output(gpio_lcd_rst, 0);
    m_msleep(80);

    st7701s_tft_SPI_WriteComm(0xFF); //commond 2
    st7701s_tft_SPI_WriteData(0x77);
    st7701s_tft_SPI_WriteData(0x01);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x13); //切到BK3

    st7701s_tft_SPI_WriteComm(0xEF);
    st7701s_tft_SPI_WriteData(0x08);

    st7701s_tft_SPI_WriteComm(0xFF);
    st7701s_tft_SPI_WriteData(0x77);
    st7701s_tft_SPI_WriteData(0x01);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x10); //切到BK0

    st7701s_tft_SPI_WriteComm(0xC0);
    st7701s_tft_SPI_WriteData(0x3B);
    st7701s_tft_SPI_WriteData(0x00);

    st7701s_tft_SPI_WriteComm(0xC1);
    st7701s_tft_SPI_WriteData(0x0B);
    st7701s_tft_SPI_WriteData(0x02);

    /* column inversion */
    st7701s_tft_SPI_WriteComm(0xC2);
    st7701s_tft_SPI_WriteData(0x37);
    st7701s_tft_SPI_WriteData(0x02);

    /* pclk polarity :falling edge */
    // st7701s_tft_SPI_WriteComm(0xC3);
    // st7701s_tft_SPI_WriteData(0x02);

    st7701s_tft_SPI_WriteComm(0xCC);
    st7701s_tft_SPI_WriteData(0x10);

    /* mdt = 0:normal for rgb666 */
    st7701s_tft_SPI_WriteComm(0xCD);
    st7701s_tft_SPI_WriteData(0x00);

    /* mdt = 1:collect for rgb666 */
    // st7701s_tft_SPI_WriteComm(0xCD);
    // st7701s_tft_SPI_WriteData(0x08);

    st7701s_tft_SPI_WriteComm(0xB0);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x0F);
    st7701s_tft_SPI_WriteData(0x16);
    st7701s_tft_SPI_WriteData(0x0E);
    st7701s_tft_SPI_WriteData(0x11);
    st7701s_tft_SPI_WriteData(0x07);
    st7701s_tft_SPI_WriteData(0x09);
    st7701s_tft_SPI_WriteData(0x09);
    st7701s_tft_SPI_WriteData(0x08);
    st7701s_tft_SPI_WriteData(0x23);
    st7701s_tft_SPI_WriteData(0x05);
    st7701s_tft_SPI_WriteData(0x11);
    st7701s_tft_SPI_WriteData(0x0F);
    st7701s_tft_SPI_WriteData(0x28);
    st7701s_tft_SPI_WriteData(0x2D);
    st7701s_tft_SPI_WriteData(0x18);

    st7701s_tft_SPI_WriteComm(0xB1);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x0F);
    st7701s_tft_SPI_WriteData(0x16);
    st7701s_tft_SPI_WriteData(0x0E);
    st7701s_tft_SPI_WriteData(0x11);
    st7701s_tft_SPI_WriteData(0x07);
    st7701s_tft_SPI_WriteData(0x09);
    st7701s_tft_SPI_WriteData(0x08);
    st7701s_tft_SPI_WriteData(0x09);
    st7701s_tft_SPI_WriteData(0x23);
    st7701s_tft_SPI_WriteData(0x05);
    st7701s_tft_SPI_WriteData(0x11);
    st7701s_tft_SPI_WriteData(0x0F);
    st7701s_tft_SPI_WriteData(0x28);
    st7701s_tft_SPI_WriteData(0x2D);
    st7701s_tft_SPI_WriteData(0x18);

    st7701s_tft_SPI_WriteComm(0xFF);
    st7701s_tft_SPI_WriteData(0x77);
    st7701s_tft_SPI_WriteData(0x01);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x11); //切到BK1

    st7701s_tft_SPI_WriteComm(0xB0);
    st7701s_tft_SPI_WriteData(0x4D);

    st7701s_tft_SPI_WriteComm(0xB1);
    st7701s_tft_SPI_WriteData(0x33);
    st7701s_tft_SPI_WriteComm(0xB2);
    st7701s_tft_SPI_WriteData(0x87);

    st7701s_tft_SPI_WriteComm(0xB5);
    st7701s_tft_SPI_WriteData(0x4B);

    st7701s_tft_SPI_WriteComm(0xB7);
    st7701s_tft_SPI_WriteData(0x8C);

    /* Digital Gamma Enable */
    st7701s_tft_SPI_WriteComm(0xB8);
    st7701s_tft_SPI_WriteData(0x20);

    st7701s_tft_SPI_WriteComm(0xC1);
    st7701s_tft_SPI_WriteData(0x78);

    st7701s_tft_SPI_WriteComm(0xC2);
    st7701s_tft_SPI_WriteData(0x78);

    st7701s_tft_SPI_WriteComm(0xD0);
    st7701s_tft_SPI_WriteData(0x88);

    st7701s_tft_SPI_WriteComm(0xE0);
    st7701s_tft_SPI_WriteData(0x00);

    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x02);

    st7701s_tft_SPI_WriteComm(0xE1);
    st7701s_tft_SPI_WriteData(0x02);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x03);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x44);
    st7701s_tft_SPI_WriteData(0x44);

    st7701s_tft_SPI_WriteComm(0xE2);
    st7701s_tft_SPI_WriteData(0x10);
    st7701s_tft_SPI_WriteData(0x10);
    st7701s_tft_SPI_WriteData(0x40);
    st7701s_tft_SPI_WriteData(0x40);
    st7701s_tft_SPI_WriteData(0xF2);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0xF2);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);

    st7701s_tft_SPI_WriteComm(0xE3);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x11);
    st7701s_tft_SPI_WriteData(0x11);

    st7701s_tft_SPI_WriteComm(0xE4);
    st7701s_tft_SPI_WriteData(0x44);
    st7701s_tft_SPI_WriteData(0x44);

    st7701s_tft_SPI_WriteComm(0xE5);
    st7701s_tft_SPI_WriteData(0x07);
    st7701s_tft_SPI_WriteData(0xEF);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0x09);
    st7701s_tft_SPI_WriteData(0xF1);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0x03);
    st7701s_tft_SPI_WriteData(0xF3);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0x05);
    st7701s_tft_SPI_WriteData(0xED);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0xF0);

    st7701s_tft_SPI_WriteComm(0xE6);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x11);
    st7701s_tft_SPI_WriteData(0x11);

    st7701s_tft_SPI_WriteComm(0xE7);
    st7701s_tft_SPI_WriteData(0x44);
    st7701s_tft_SPI_WriteData(0x44);

    st7701s_tft_SPI_WriteComm(0xE8);
    st7701s_tft_SPI_WriteData(0x08);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0x0A);
    st7701s_tft_SPI_WriteData(0xF2);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0x04);
    st7701s_tft_SPI_WriteData(0xF4);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0x06);
    st7701s_tft_SPI_WriteData(0xEE);
    st7701s_tft_SPI_WriteData(0xF0);
    st7701s_tft_SPI_WriteData(0xF0);

    st7701s_tft_SPI_WriteComm(0xEB);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0xE4);
    st7701s_tft_SPI_WriteData(0xE4);
    st7701s_tft_SPI_WriteData(0x44);
    st7701s_tft_SPI_WriteData(0x88);
    st7701s_tft_SPI_WriteData(0x40);

    st7701s_tft_SPI_WriteComm(0xEC);
    st7701s_tft_SPI_WriteData(0x78);
    st7701s_tft_SPI_WriteData(0x00);

    st7701s_tft_SPI_WriteComm(0xED);
    st7701s_tft_SPI_WriteData(0x20);
    st7701s_tft_SPI_WriteData(0xF9);
    st7701s_tft_SPI_WriteData(0x87);
    st7701s_tft_SPI_WriteData(0x76);
    st7701s_tft_SPI_WriteData(0x65);
    st7701s_tft_SPI_WriteData(0x54);
    st7701s_tft_SPI_WriteData(0x4F);
    st7701s_tft_SPI_WriteData(0xFF);
    st7701s_tft_SPI_WriteData(0xFF);
    st7701s_tft_SPI_WriteData(0xF4);
    st7701s_tft_SPI_WriteData(0x45);
    st7701s_tft_SPI_WriteData(0x56);
    st7701s_tft_SPI_WriteData(0x67);
    st7701s_tft_SPI_WriteData(0x78);
    st7701s_tft_SPI_WriteData(0x9F);
    st7701s_tft_SPI_WriteData(0x02);

    st7701s_tft_SPI_WriteComm(0xEF);
    st7701s_tft_SPI_WriteData(0x10);
    st7701s_tft_SPI_WriteData(0x0D);
    st7701s_tft_SPI_WriteData(0x04);
    st7701s_tft_SPI_WriteData(0x08);
    st7701s_tft_SPI_WriteData(0x3F);
    st7701s_tft_SPI_WriteData(0x1F);

    st7701s_tft_SPI_WriteComm(0xFF);
    st7701s_tft_SPI_WriteData(0x77);
    st7701s_tft_SPI_WriteData(0x01);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x00);
    st7701s_tft_SPI_WriteData(0x10);

    /* 18bit per pixel */
    st7701s_tft_SPI_WriteComm(0x3a);
    st7701s_tft_SPI_WriteData(0x60);

    st7701s_tft_SPI_WriteComm(0x11);

    m_msleep (120);

    st7701s_tft_SPI_WriteComm(0x29);
    m_msleep(20);
}


static int st7701s_tft_power_on(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0) {
        gpio_direction_output(gpio_lcd_power_en, 0);
        m_msleep(20);
    }

    if (gpio_lcd_rst >= 0) {
        gpio_direction_output(gpio_lcd_rst, 0);
        m_msleep(2);
        gpio_direction_output(gpio_lcd_rst, 1);
        m_msleep(2);
        gpio_direction_output(gpio_lcd_rst, 0);
        m_msleep(2);
        gpio_direction_output(gpio_lcd_rst, 0);
        m_msleep(20);
    }

    //black_light
    // gpio_direction_output(GPIO_PC(11), 1);

    gpio_direction_output(gpio_lcd_scl, 1);
    gpio_direction_output(gpio_lcd_cs, 1);
    gpio_direction_output(gpio_lcd_sda, 0);
    st7701s_tft_spi_init( );
    gpio_direction_output(gpio_lcd_cs, 1);

    return 0;
}

static int st7701s_tft_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        gpio_direction_output(gpio_lcd_power_en, 1);

    return 0;
}

struct lcdc_data lcdc_data = {
    .name = "st7701s_tft",
    .refresh = 60,
    .xres = 480,
    .yres = 480,
    .pixclock = 0, // 自动计算

    .left_margin = 30,
    .right_margin = 50,
    .upper_margin = 10,
    .lower_margin = 20,
    .hsync_len = 2,
    .vsync_len = 2,

    .fb_fmt = fb_fmt_ARGB8888,
    .lcd_mode = TFT_24BITS,
    .out_format = OUT_FORMAT_RGB666,
    .tft = {
        .even_line_order = ORDER_RGB,
        .odd_line_order = ORDER_RGB,
        .pix_clk_polarity = AT_RISING_EDGE,
        .de_active_level = AT_HIGH_LEVEL,
        .hsync_active_level = AT_HIGH_LEVEL,
        .vsync_active_level = AT_HIGH_LEVEL,
        .pix_clk_inv = INVERT_ENABLE,
    },
    .power_on = st7701s_tft_power_on,
    .power_off = st7701s_tft_power_off,
};

static int __init st7701s_tft_init(void)
{
    int ret;

    if (gpio_lcd_power_en >= 0) {
        ret = gpio_request(gpio_lcd_power_en, "lcd_power_en");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "st7701s_tft: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_power_en, buf));
            return ret;
        }
    }

    if (gpio_lcd_rst >= 0) {
        ret = gpio_request(gpio_lcd_rst, "lcd_rst");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "st7701s_tft: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_rst;
        }
    }

    if (gpio_lcd_scl >= 0) {
        ret = gpio_request(gpio_lcd_scl, "lcd_scl");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "st7701s_tft: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_scl;
        }
    }

    if (gpio_lcd_sda >= 0) {
        ret = gpio_request(gpio_lcd_sda, "lcd_sda");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "st7701s_tft: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_sda;
        }
    }

    if (gpio_lcd_cs >= 0) {
        ret = gpio_request(gpio_lcd_cs, "lcd_cs");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "st7701s_tft: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_cs;
        }
    }

    jzfb_register_lcd(&lcdc_data);

    return 0;

err_lcd_cs:
    if (gpio_lcd_sda >= 0)
        gpio_free(gpio_lcd_sda);
err_lcd_sda:
    if (gpio_lcd_scl >= 0)
        gpio_free(gpio_lcd_scl);
err_lcd_scl:
    if (gpio_lcd_rst >= 0)
        gpio_free(gpio_lcd_rst);
err_lcd_rst:
    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);
    return ret;
}


static void __exit st7701s_tft_exit(void)
{
    if (gpio_lcd_cs >= 0)
        gpio_free(gpio_lcd_cs);
    if (gpio_lcd_sda >= 0)
        gpio_free(gpio_lcd_sda);
    if (gpio_lcd_scl >= 0)
        gpio_free(gpio_lcd_scl);
    if (gpio_lcd_rst >= 0)
        gpio_free(gpio_lcd_rst);
    if (gpio_lcd_power_en >= 0)
        gpio_free(gpio_lcd_power_en);

    jzfb_unregister_lcd(&lcdc_data);
}

module_init(st7701s_tft_init);
module_exit(st7701s_tft_exit);

MODULE_DESCRIPTION("st7701s_tft lcd panel driver");
MODULE_LICENSE("GPL");
