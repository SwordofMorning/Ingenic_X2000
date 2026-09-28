#include <common.h>
#include <bit_field.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include "lcdc_data.h"

#ifndef GPIO_PORT_B
#define GPIO_PORT_B 1
#endif

#define TFT_d0 4
#define TFT_d1 5
#define TFT_d2 6
#define TFT_d3 7
#define TFT_d4 8
#define TFT_d5 9
#define TFT_d6 10
#define TFT_d7 11
#define TFT_d8 12
#define TFT_d9 13
#define TFT_d10 14
#define TFT_d11 15
#define TFT_d12 16
#define TFT_d13 17
#define TFT_d14 18
#define TFT_d15 19
#define TFT_d16 20
#define TFT_d17 21
#define TFT_d18 22
#define TFT_d19 23
#define TFT_d20 24
#define TFT_d21 25
#define TFT_d22 26
#define TFT_d23 27

#define TFT_pclk 28
#define TFT_vsync 29
#define TFT_hsync 30
#define TFT_de 31

static unsigned int m_pins;

static void jzfb_release_pins(void)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (m_pins & (1 << i))
            gpio_free(GPIO_PB(i));
    }

    m_pins = 0;
}

static char tft_names[][10] = {
    [TFT_d0] = "TFT_d0",
    [TFT_d1] = "TFT_d1",
    [TFT_d2] = "TFT_d2",
    [TFT_d3] = "TFT_d3",
    [TFT_d4] = "TFT_d4",
    [TFT_d5] = "TFT_d5",
    [TFT_d6] = "TFT_d6",
    [TFT_d7] = "TFT_d7",
    [TFT_d8] = "TFT_d8",
    [TFT_d9] = "TFT_d9",
    [TFT_d10] = "TFT_d10",
    [TFT_d11] = "TFT_d11",
    [TFT_d12] = "TFT_d12",
    [TFT_d13] = "TFT_d13",
    [TFT_d14] = "TFT_d14",
    [TFT_d15] = "TFT_d15",
    [TFT_d16] = "TFT_d16",
    [TFT_d17] = "TFT_d17",
    [TFT_d18] = "TFT_d18",
    [TFT_d19] = "TFT_d19",
    [TFT_d20] = "TFT_d20",
    [TFT_d21] = "TFT_d21",
    [TFT_d22] = "TFT_d22",
    [TFT_d23] = "TFT_d23",
    [TFT_pclk] = "TFT_pclk",
    [TFT_vsync] = "TFT_vsync",
    [TFT_hsync] = "TFT_hsync",
    [TFT_de] = "TFT_de",
};

static int tft_request_pins(unsigned int pins)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i)) {
            int ret = gpio_request(GPIO_PB(i), tft_names[i]);
            if (ret) {
                printk(KERN_ERR "jzfb: failed to request GPIO_PB%d\n", i);
                jzfb_release_pins();
                return ret;
            }
            m_pins |= (1 << i);
        }
    }

    return 0;
}

static int tft_init_gpio(int r, int g, int b)
{
    unsigned int pins =
        bit_field_mask(TFT_d7 - b + 1, TFT_d7) |
        bit_field_mask(TFT_d15 - g + 1, TFT_d15) |
        bit_field_mask(TFT_d23 - r + 1, TFT_d23) |
        BIT(TFT_pclk) | BIT(TFT_de) | BIT(TFT_vsync) | BIT(TFT_hsync);

    int ret = tft_request_pins(pins);
    if (ret)
        return ret;

    gpio_port_set_func(GPIO_PORT_B, pins, GPIO_FUNC_0);

    return 0;
}

static int tft_18bit_init_gpio(void)
{
    unsigned int pins =
        bit_field_mask(TFT_d0, TFT_d17) |
        BIT(TFT_pclk) | BIT(TFT_de) | BIT(TFT_vsync) | BIT(TFT_hsync);

    int ret = tft_request_pins(pins);
    if (ret)
        return ret;

    gpio_port_set_func(GPIO_PORT_B, pins, GPIO_FUNC_0);

    return 0;
}

static int tft_16bit_init_gpio(void)
{
    unsigned int pins =
        bit_field_mask(TFT_d0, TFT_d15) |
        BIT(TFT_pclk) | BIT(TFT_de) | BIT(TFT_vsync) | BIT(TFT_hsync);

    int ret = tft_request_pins(pins);
    if (ret)
        return ret;

    gpio_port_set_func(GPIO_PORT_B, pins, GPIO_FUNC_0);

    return 0;
}

static int tft_serial_init_gpio(void)
{
    unsigned int pins =
        bit_field_mask(TFT_d0, TFT_d7) |
        BIT(TFT_pclk) | BIT(TFT_de) | BIT(TFT_vsync) | BIT(TFT_hsync);

    int ret = tft_request_pins(pins);
    if (ret)
        return ret;

    gpio_port_set_func(GPIO_PORT_B, pins, GPIO_FUNC_0);

    return 0;
}

#define SLCD_d0 4
#define SLCD_d1 5
#define SLCD_d2 6
#define SLCD_d3 7
#define SLCD_d4 8
#define SLCD_d5 9
#define SLCD_d6 10
#define SLCD_d7 11
#define SLCD_d8 12
#define SLCD_d9 13
#define SLCD_d10 14
#define SLCD_d11 15
#define SLCD_d12 16
#define SLCD_d13 17
#define SLCD_d14 18
#define SLCD_d15 19

#define SLCD_cs 28
#define SLCD_dc 29
#define SLCD_wr 30
#define SLCD_te 31

static char slcd_names[][9] = {
    [SLCD_d0] = "SLCD_d0",
    [SLCD_d1] = "SLCD_d1",
    [SLCD_d2] = "SLCD_d2",
    [SLCD_d3] = "SLCD_d3",
    [SLCD_d4] = "SLCD_d4",
    [SLCD_d5] = "SLCD_d5",
    [SLCD_d6] = "SLCD_d6",
    [SLCD_d7] = "SLCD_d7",
    [SLCD_d8] = "SLCD_d8",
    [SLCD_d9] = "SLCD_d9",
    [SLCD_d10] = "SLCD_d10",
    [SLCD_d11] = "SLCD_d11",
    [SLCD_d12] = "SLCD_d12",
    [SLCD_d13] = "SLCD_d13",
    [SLCD_d14] = "SLCD_d14",
    [SLCD_d15] = "SLCD_d15",
    [SLCD_cs] = "SLCD_cs",
    [SLCD_dc] = "SLCD_dc",
    [SLCD_wr] = "SLCD_wr",
    [SLCD_te] = "SLCD_te",
};

static int slcd_request_pins(unsigned int pins)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i)) {
            int ret = gpio_request(GPIO_PB(i), slcd_names[i]);
            if (ret) {
                printk(KERN_ERR "jzfb: failed to request GPIO_PB%d\n", i);
                jzfb_release_pins();
                return ret;
            }
            m_pins |= (1 << i);
        }
    }

    return 0;
}

static int slcd_init_gpio_status(int is_cmd, struct lcdc_data *pdata)
{
    unsigned int gpio_dc = bit_field_mask(SLCD_dc, SLCD_dc);
    unsigned int gpio_wr = bit_field_mask(SLCD_wr, SLCD_wr);

    if (pdata->slcd.dc_pin == CMD_HIGH_DATA_LOW)
        is_cmd = !is_cmd;

    if (!is_cmd)
        gpio_port_set_func(GPIO_PORT_B, gpio_dc, GPIO_OUTPUT1);
    else
        gpio_port_set_func(GPIO_PORT_B, gpio_dc, GPIO_OUTPUT0);

    if (pdata->slcd.wr_data_sample_edge == AT_RISING_EDGE)
        gpio_port_set_func(GPIO_PORT_B, gpio_wr, GPIO_OUTPUT1);
    else
        gpio_port_set_func(GPIO_PORT_B, gpio_wr, GPIO_OUTPUT0);

    if (pdata->slcd.rd_gpio <= 0) {
        printk(KERN_ERR "please set rd gpio\n");
        return -1;
    }

    if (pdata->slcd.rd_data_sample_edge == AT_RISING_EDGE)
        gpio_set_func(pdata->slcd.rd_gpio, GPIO_OUTPUT1);
    else
        gpio_set_func(pdata->slcd.rd_gpio, GPIO_OUTPUT0);
    udelay(1);

    return 0;
}

static void slcd_reset_gpio_status(void)
{
    unsigned int pins = bit_field_mask(SLCD_d0, SLCD_d7);
    unsigned int gpio_dc = bit_field_mask(SLCD_dc, SLCD_dc);
    unsigned int gpio_wr = bit_field_mask(SLCD_wr, SLCD_wr);
    pins |= gpio_dc;
    pins |= gpio_wr;
    gpio_port_set_func(GPIO_PORT_B, pins, GPIO_FUNC_1 | GPIO_PULL_HIZ);
}

static void slcd_gpio_send(int reg, struct lcdc_data *pdata)
{
    int i;
    unsigned int data_pins = bit_field_mask(SLCD_d0, SLCD_d7);
    unsigned int gpio_wr = bit_field_mask(SLCD_wr, SLCD_wr);
    gpio_port_set_func(GPIO_PORT_B, data_pins, GPIO_OUTPUT0);

    if (pdata->slcd.wr_data_sample_edge == AT_RISING_EDGE)
        gpio_port_set_func(GPIO_PORT_B, gpio_wr, GPIO_OUTPUT0);
    else
        gpio_port_set_func(GPIO_PORT_B, gpio_wr, GPIO_OUTPUT1);
    udelay(1);

    for (i = 0; i < 8; i++)
        gpio_set_value(GPIO_PB(i+4), (reg >> i) & 0x01);

    if (pdata->slcd.wr_data_sample_edge == AT_RISING_EDGE)
        gpio_port_set_func(GPIO_PORT_B, gpio_wr, GPIO_OUTPUT1);
    else
        gpio_port_set_func(GPIO_PORT_B, gpio_wr, GPIO_OUTPUT0);
    udelay(1);
}

static int slcd_gpio_receive(struct lcdc_data *pdata)
{
    int i, value = 0;
    unsigned int data_pins = bit_field_mask(SLCD_d0, SLCD_d7);
    gpio_port_set_func(GPIO_PORT_B, data_pins, GPIO_INPUT);

    if (pdata->slcd.rd_data_sample_edge == AT_RISING_EDGE)
        gpio_set_func(pdata->slcd.rd_gpio, GPIO_OUTPUT0);
    else
        gpio_set_func(pdata->slcd.rd_gpio, GPIO_OUTPUT1);
    udelay(1);

    for (i = 0; i < 8; i++)
        value = gpio_get_value(GPIO_PB(i+4)) << i | value;

    if (pdata->slcd.rd_data_sample_edge == AT_RISING_EDGE)
        gpio_set_func(pdata->slcd.rd_gpio, GPIO_OUTPUT1);
    else
        gpio_set_func(pdata->slcd.rd_gpio, GPIO_OUTPUT0);
    udelay(1);

    return value;
}

static int slcd_init_gpio(int pins, int use_rdy, int use_te, int use_cs)
{
    if (use_rdy) {
        printk(KERN_ERR "jzfb: x2600 no rdy pin\n");
        return -ENODEV;
    }

    if (use_te)
        pins |= BIT(SLCD_te);

    if (use_cs)
        pins |= BIT(SLCD_cs);

    pins |= BIT(SLCD_wr);
    pins |= BIT(SLCD_dc);

    int ret = slcd_request_pins(pins);
    if (ret)
        return ret;

    gpio_port_set_func(GPIO_PORT_B, pins, GPIO_FUNC_1 | GPIO_PULL_HIZ);

    return 0;
}

static int slcd_init_gpio_data0(int use_rdy, int use_te, int use_cs)
{
    unsigned int pins = bit_field_mask(SLCD_d0, SLCD_d0);

    return slcd_init_gpio(pins, use_rdy, use_te, use_cs);
}

static int slcd_init_gpio_data8(int use_rdy, int use_te, int use_cs)
{
    unsigned int pins = bit_field_mask(SLCD_d0, SLCD_d7);

    return slcd_init_gpio(pins, use_rdy, use_te, use_cs);
}

static int slcd_init_gpio_data9(int use_rdy, int use_te, int use_cs)
{
    unsigned int pins = bit_field_mask(SLCD_d0, SLCD_d8);

    return slcd_init_gpio(pins, use_rdy, use_te, use_cs);
}

static int slcd_init_gpio_data16(int use_rdy, int use_te, int use_cs)
{
    unsigned int pins = bit_field_mask(SLCD_d0, SLCD_d15);

    return slcd_init_gpio(pins, use_rdy, use_te, use_cs);
}

static int mipi_slcd_init_te(void)
{
    unsigned int pins = 0;
    int ret;

    pins |= BIT(SLCD_te);

    ret = slcd_request_pins(pins);
    if (ret == 0)
        gpio_port_set_func(GPIO_PORT_B, pins, GPIO_FUNC_1 | GPIO_PULL_HIZ);

    return ret;
}