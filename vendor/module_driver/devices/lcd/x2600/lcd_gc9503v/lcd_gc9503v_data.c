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

void GC9503v_SPI_WriteComm(unsigned char i)
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

void GC9503v_SPI_WriteData(unsigned char i)
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

void gc9503v_spi_init(void)
{
    gpio_direction_output(gpio_lcd_rst, 0);
    m_msleep(10);
    gpio_direction_output(gpio_lcd_rst, 1);
    m_msleep(10);
    gpio_direction_output(gpio_lcd_rst, 0);
    m_msleep(10);

    GC9503v_SPI_WriteComm(0xF0);
    GC9503v_SPI_WriteData(0x55);
    GC9503v_SPI_WriteData(0xAA);
    GC9503v_SPI_WriteData(0x52);
    GC9503v_SPI_WriteData(0x08);
    GC9503v_SPI_WriteData(0x00);

    GC9503v_SPI_WriteComm(0xF6);
    GC9503v_SPI_WriteData(0x5A);
    GC9503v_SPI_WriteData(0x87);

    GC9503v_SPI_WriteComm(0xC1);
    GC9503v_SPI_WriteData(0x3F);

    GC9503v_SPI_WriteComm(0xC2);
    GC9503v_SPI_WriteData(0x0F);

    GC9503v_SPI_WriteComm(0xC6);
    GC9503v_SPI_WriteData(0xF8);

    GC9503v_SPI_WriteComm(0xC9);
    GC9503v_SPI_WriteData(0x10);

    GC9503v_SPI_WriteComm(0xCD);
    GC9503v_SPI_WriteData(0x25);

    GC9503v_SPI_WriteComm(0x86);
    GC9503v_SPI_WriteData(0x99);
    GC9503v_SPI_WriteData(0xA3);
    GC9503v_SPI_WriteData(0xA3);
    GC9503v_SPI_WriteData(0x51);

    GC9503v_SPI_WriteComm(0xF8);
    GC9503v_SPI_WriteData(0x8A);

    GC9503v_SPI_WriteComm(0xAC);
    GC9503v_SPI_WriteData(0x45);

    GC9503v_SPI_WriteComm(0xA7);
    GC9503v_SPI_WriteData(0x47);

    GC9503v_SPI_WriteComm(0xA0);
    GC9503v_SPI_WriteData(0xDD);

    GC9503v_SPI_WriteComm(0x71);
    GC9503v_SPI_WriteData(0x48);

    GC9503v_SPI_WriteComm(0x72);
    GC9503v_SPI_WriteData(0x48);

    GC9503v_SPI_WriteComm(0x73);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x44);

    GC9503v_SPI_WriteComm(0x97);
    GC9503v_SPI_WriteData(0xEE);

    GC9503v_SPI_WriteComm(0x83);
    GC9503v_SPI_WriteData(0x93);

    GC9503v_SPI_WriteComm(0xA3);
    GC9503v_SPI_WriteData(0xEE);

    GC9503v_SPI_WriteComm(0xFD);
    GC9503v_SPI_WriteData(0x28);
    GC9503v_SPI_WriteData(0x3C);
    GC9503v_SPI_WriteData(0x00);


    GC9503v_SPI_WriteComm(0xFA);
    GC9503v_SPI_WriteData(0x08);
    GC9503v_SPI_WriteData(0x08);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x04);

    GC9503v_SPI_WriteComm(0x9A);
    GC9503v_SPI_WriteData(0x8B);

    GC9503v_SPI_WriteComm(0x9B);
    GC9503v_SPI_WriteData(0x63);

    GC9503v_SPI_WriteComm(0x82);
    GC9503v_SPI_WriteData(0x41);
    GC9503v_SPI_WriteData(0x41);

    /* 1 dot inversion */
    GC9503v_SPI_WriteComm(0xB1);
    GC9503v_SPI_WriteData(0x00);

    /* pclk polarity :falling edge */
    // GC9503v_SPI_WriteComm(0xB0);
    // GC9503v_SPI_WriteData(0x08);

    GC9503v_SPI_WriteComm(0x7A);
    GC9503v_SPI_WriteData(0x13);
    GC9503v_SPI_WriteData(0x1A);

    GC9503v_SPI_WriteComm(0x7B);
    GC9503v_SPI_WriteData(0x13);
    GC9503v_SPI_WriteData(0x1A);

    GC9503v_SPI_WriteComm(0x6D);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x1E);
    GC9503v_SPI_WriteData(0x1A);
    GC9503v_SPI_WriteData(0x19);
    GC9503v_SPI_WriteData(0x0F);
    GC9503v_SPI_WriteData(0x0F);
    GC9503v_SPI_WriteData(0x0D);
    GC9503v_SPI_WriteData(0x0D);
    GC9503v_SPI_WriteData(0x0B);
    GC9503v_SPI_WriteData(0x0B);
    GC9503v_SPI_WriteData(0x09);
    GC9503v_SPI_WriteData(0x09);
    GC9503v_SPI_WriteData(0x1E);
    GC9503v_SPI_WriteData(0x1E);
    GC9503v_SPI_WriteData(0x1E);
    GC9503v_SPI_WriteData(0x1E);
    GC9503v_SPI_WriteData(0x1E);
    GC9503v_SPI_WriteData(0x1E);
    GC9503v_SPI_WriteData(0x0A);
    GC9503v_SPI_WriteData(0x0A);
    GC9503v_SPI_WriteData(0x0C);
    GC9503v_SPI_WriteData(0x0C);
    GC9503v_SPI_WriteData(0x0E);
    GC9503v_SPI_WriteData(0x0E);
    GC9503v_SPI_WriteData(0x10);
    GC9503v_SPI_WriteData(0x10);
    GC9503v_SPI_WriteData(0x19);
    GC9503v_SPI_WriteData(0x1A);
    GC9503v_SPI_WriteData(0x1E);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x03);

    GC9503v_SPI_WriteComm(0x60);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x09);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x08);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);

    GC9503v_SPI_WriteComm(0x61);
    GC9503v_SPI_WriteData(0x31);
    GC9503v_SPI_WriteData(0xE6);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x0A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);

    GC9503v_SPI_WriteComm(0x64);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x04);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE1);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE2);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);

    GC9503v_SPI_WriteComm(0x65);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE3);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE4);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);

    GC9503v_SPI_WriteComm(0x66);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x08);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE6);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x07);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE7);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);

    GC9503v_SPI_WriteComm(0x67);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x06);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xDF);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x38);
    GC9503v_SPI_WriteData(0x05);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE0);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);

    GC9503v_SPI_WriteComm(0x68);
    GC9503v_SPI_WriteData(0x3E);
    GC9503v_SPI_WriteData(0x08);
    GC9503v_SPI_WriteData(0x0F);
    GC9503v_SPI_WriteData(0x08);
    GC9503v_SPI_WriteData(0x0E);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);
    GC9503v_SPI_WriteData(0x08);
    GC9503v_SPI_WriteData(0x0F);
    GC9503v_SPI_WriteData(0x08);
    GC9503v_SPI_WriteData(0x0E);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x7A);

    GC9503v_SPI_WriteComm(0x69);
    GC9503v_SPI_WriteData(0x04);
    GC9503v_SPI_WriteData(0x22);
    GC9503v_SPI_WriteData(0x14);
    GC9503v_SPI_WriteData(0x22);
    GC9503v_SPI_WriteData(0x44);
    GC9503v_SPI_WriteData(0x22);
    GC9503v_SPI_WriteData(0x08);

    GC9503v_SPI_WriteComm(0x6B);
    GC9503v_SPI_WriteData(0x07);

    GC9503v_SPI_WriteComm(0xD1);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x21);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x5A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x7D);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x9A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xCA);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xED);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x2A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x59);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xA7);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE4);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x43);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x8F);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x90);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0xD1);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x1A);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x46);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x86);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xA8);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xBB);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xCC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xDC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xE9);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xF6);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFA);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFF);

    GC9503v_SPI_WriteComm(0xD2);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x21);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x5A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x7D);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x9A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xCA);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xED);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x2A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x59);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xA7);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE4);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x43);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x8F);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x90);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0xD1);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x1A);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x46);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x86);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xA8);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xBB);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xCC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xDC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xE9);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xF6);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFA);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFF);

    GC9503v_SPI_WriteComm(0xD3);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x21);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x5A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x7D);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x9A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xCA);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xED);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x2A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x59);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xA7);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE4);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x43);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x8F);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x90);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0xD1);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x1A);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x46);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x86);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xA8);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xBB);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xCC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xDC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xE9);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xF6);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFA);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFF);

    GC9503v_SPI_WriteComm(0xD4);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x21);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x5A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x7D);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x9A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xCA);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xED);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x2A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x59);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xA7);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE4);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x43);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x8F);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x90);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0xD1);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x1A);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x46);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x86);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xA8);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xBB);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xCC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xDC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xE9);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xF6);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFA);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFF);

    GC9503v_SPI_WriteComm(0xD5);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x21);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x5A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x7D);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x9A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xCA);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xED);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x2A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x59);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xA7);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE4);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x43);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x8F);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x90);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0xD1);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x1A);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x46);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x86);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xA8);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xBB);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xCC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xDC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xE9);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xF6);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFA);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFF);

    GC9503v_SPI_WriteComm(0xD6);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x21);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x5A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x7D);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0x9A);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xCA);
    GC9503v_SPI_WriteData(0x00);
    GC9503v_SPI_WriteData(0xED);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x2A);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0x59);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xA7);
    GC9503v_SPI_WriteData(0x01);
    GC9503v_SPI_WriteData(0xE4);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x43);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x8F);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0x90);
    GC9503v_SPI_WriteData(0x02);
    GC9503v_SPI_WriteData(0xD1);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x1A);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x46);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0x86);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xA8);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xBB);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xCC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xDC);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xE9);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xF6);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFA);
    GC9503v_SPI_WriteData(0x03);
    GC9503v_SPI_WriteData(0xFF);

    /* 18bit per pixel */
    GC9503v_SPI_WriteComm(0x3a);
    GC9503v_SPI_WriteData(0x60);

    GC9503v_SPI_WriteComm(0x11);
    GC9503v_SPI_WriteData(0x00);
    m_msleep(120);

    GC9503v_SPI_WriteComm(0x29);
    GC9503v_SPI_WriteData(0x00);
    m_msleep(20);
}

static int gc9503v_power_on(struct lcdc *lcdc)
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
    gc9503v_spi_init( );
    gpio_direction_output(gpio_lcd_cs, 1);

    return 0;
}

static int gc9503v_power_off(struct lcdc *lcdc)
{
    if (gpio_lcd_power_en >= 0)
        gpio_direction_output(gpio_lcd_power_en, 1);

    return 0;
}

struct lcdc_data lcdc_data = {
    .name = "gc9503v",
    .refresh = 60,
    .xres = 480,
    .yres = 480,
    .pixclock = 0, // 自动计算

    .left_margin = 30,
    .right_margin = 30,
    .upper_margin = 20,
    .lower_margin = 20,
    .hsync_len = 150,
    .vsync_len = 240,

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
    .power_on = gc9503v_power_on,
    .power_off = gc9503v_power_off,
};

static int __init gc9503v_init(void)
{
    int ret;

    if (gpio_lcd_power_en >= 0) {
        ret = gpio_request(gpio_lcd_power_en, "lcd_power_en");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "gc9503v: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_power_en, buf));
            return ret;
        }
    }

    if (gpio_lcd_rst >= 0) {
        ret = gpio_request(gpio_lcd_rst, "lcd_rst");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "gc9503v: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_rst;
        }
    }

    if (gpio_lcd_scl >= 0) {
        ret = gpio_request(gpio_lcd_scl, "lcd_scl");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "gc9503v: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_scl;
        }
    }

    if (gpio_lcd_sda >= 0) {
        ret = gpio_request(gpio_lcd_sda, "lcd_sda");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "gc9503v: failed to request: %s\n",
                 gpio_to_str(gpio_lcd_rst, buf));
            goto err_lcd_sda;
        }
    }

    if (gpio_lcd_cs >= 0) {
        ret = gpio_request(gpio_lcd_cs, "lcd_cs");
        if (ret) {
            char buf[10];
            printk(KERN_ERR "gc9503v: failed to request: %s\n",
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


static void __exit gc9503v_exit(void)
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

module_init(gc9503v_init);
module_exit(gc9503v_exit);

MODULE_DESCRIPTION("gc9503v lcd panel driver");
MODULE_LICENSE("GPL");
