#include <stdio.h>
#include <string.h>
#include <bit_field2.h>
#include <driver/gpio.h>
#include <delay.h>

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

static void tft_set_pins_func(unsigned int pins)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i))
            gpio_set_func(GPIO_PB(i), GPIO_FUNC_0);
    }
}

static int tft_init_gpio(int r, int g, int b)
{
    unsigned int pins =
        bit_field_mask(TFT_d7 - b + 1, TFT_d7) |
        bit_field_mask(TFT_d15 - g + 1, TFT_d15) |
        bit_field_mask(TFT_d23 - r + 1, TFT_d23) |
        BIT(TFT_pclk) | BIT(TFT_de) | BIT(TFT_vsync) | BIT(TFT_hsync);

    tft_set_pins_func(pins);

    return 0;
}

static int tft_18bit_init_gpio(void)
{
    unsigned int pins =
        bit_field_mask(TFT_d0, TFT_d17) |
        BIT(TFT_pclk) | BIT(TFT_de) | BIT(TFT_vsync) | BIT(TFT_hsync);

    tft_set_pins_func(pins);

    return 0;
}

static int tft_16bit_init_gpio(void)
{
    unsigned int pins =
        bit_field_mask(TFT_d0, TFT_d15) |
        BIT(TFT_pclk) | BIT(TFT_de) | BIT(TFT_vsync) | BIT(TFT_hsync);

    tft_set_pins_func(pins);

    return 0;
}

static int tft_serial_init_gpio(void)
{
    unsigned int pins =
        bit_field_mask(TFT_d0, TFT_d7) |
        BIT(TFT_pclk) | BIT(TFT_de) | BIT(TFT_vsync) | BIT(TFT_hsync);

    tft_set_pins_func(pins);

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

static void slcd_set_pins_func(unsigned int pins, enum gpio_function func)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i))
            gpio_set_func(GPIO_PB(i), func);
    }
}

static int slcd_init_gpio(int pins, int use_rdy, int use_te, int use_cs)
{
    if (use_rdy) {
        printf("fb: x2600 no rdy pin\n");
        return -ENODEV;
    }

    if (use_te)
        pins |= BIT(SLCD_te);

    if (use_cs)
        pins |= BIT(SLCD_cs);

    pins |= BIT(SLCD_wr);
    pins |= BIT(SLCD_dc);

    slcd_set_pins_func(pins, GPIO_FUNC_1 | GPIO_PULL_HIZ);

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
    gpio_set_func(GPIO_PB(SLCD_te), GPIO_FUNC_1 | GPIO_PULL_HIZ);

    return 0;
}