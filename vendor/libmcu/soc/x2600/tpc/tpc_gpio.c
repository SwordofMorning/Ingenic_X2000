#include <stdio.h>
#include <common.h>
#include <driver/gpio.h>
#include <assert.h>
#include "tpc_hal.h"
#include "tpc_regs.h"

/*时钟*/
#define TPC_clk       14
/*锁存*/
#define TPC_lat       23

/*电机输出io*/
#define MOTOR_d0         4
#define MOTOR_d1         5
#define MOTOR_d2         6
#define MOTOR_d3         7
#define MOTOR_d4         8
#define MOTOR_d5         9
#define MOTOR_d6         10
#define MOTOR_d7         11

/*打印头输出io*/
#define PRINT_d0        15
#define PRINT_d1        16
#define PRINT_d2        17
#define PRINT_d3        18
#define PRINT_d4        19
#define PRINT_d5        20
#define PRINT_d6        21
#define PRINT_d7        22

/*热信号输出io*/
#define HEAT_d0         24
#define HEAT_d1         25
#define HEAT_d2         26
#define HEAT_d3         27
#define HEAT_d4         28
#define HEAT_d5         29
#define HEAT_d6         30
#define HEAT_d7         31


static int shift_channels = 0;
static int motor0_channels = 0;
static int motor1_channels = 0;
static int heat_channels = 0;


void tpc_lat_gpio_init(void)
{
    gpio_set_func(GPIO_PB(TPC_lat) ,GPIO_FUNC_3);
    tpc_set_bit(PIOC, PLAT, 1);
}

void tpc_lat_gpio_deinit(void)
{
    tpc_set_bit(PIOC, PLAT, 0);
}

void tpc_shift_gpio_init(int channels)
{
    if (channels > 8 || channels < 1)
        panic("tpc print not support channels=%d\n",channels);


    int i;
    for(i = PRINT_d0; i < channels + PRINT_d0; i++) {
        gpio_set_func(GPIO_PB(i) ,GPIO_FUNC_3);
        tpc_set_bit(PIOC, i - PRINT_d0 + 1, i - PRINT_d0 + 1, 1);
    }


    gpio_set_func(GPIO_PB(TPC_clk), GPIO_FUNC_3);

    tpc_set_bit(PIOC, PSFTCLK, 1);

    shift_channels = channels;
}

void tpc_shift_gpio_deinit(void)
{
    tpc_set_bit(PIOC, PSFTCLK, 0);

    int i;
    for(i = PRINT_d0; i < shift_channels + PRINT_d0; i++) {
        tpc_set_bit(PIOC, i - PRINT_d0 + 1, i - PRINT_d0 + 1, 0);
    }
}

void tpc_heat_gpio_init(int channels)
{
    if (channels > 8 || channels < 1)
        panic("tpc heat not support channels=%d\n",channels);


    int i;
    for(i = HEAT_d0; i < channels + HEAT_d0; i++) {
        gpio_set_func(GPIO_PB(i) ,GPIO_FUNC_3);

        tpc_set_bit(PIOC, i - HEAT_d0 + 10, i - HEAT_d0 + 10, 1);
    }

    heat_channels = channels;
}

void tpc_heat_gpio_deinit(void)
{
    int i;
    for(i = HEAT_d0; i < heat_channels + HEAT_d0; i++) {
        tpc_set_bit(PIOC, i - HEAT_d0 + 10, i - HEAT_d0 + 10, 0);
    }
}

void tpc_motor_gpio_init(int id, int channels)
{

    if (channels > 8)
        panic("motor %d not support channels=%d\n", id, channels);

    if (id && channels > 4)
        panic("motor %d not support channels=%d\n", id, channels);

    int index = MOTOR_d0;
    if (id)
        index = MOTOR_d4;

    int i;
    for(i = index ;i < channels + index; i++) {

        gpio_set_func(GPIO_PB(i) ,GPIO_FUNC_3);

        tpc_set_bit(PIOC, i - MOTOR_d0 + 18, i - MOTOR_d0 + 18, 1);
    }

    if (id)
        motor1_channels = channels;
    else
        motor0_channels = channels;
}

void tpc_motor_gpio_deinit(int id)
{
    int index = MOTOR_d0;
    int channels = motor0_channels;
    if (id) {
        index = MOTOR_d4;
        channels = motor1_channels;
    }

    int i;
    for(i = index ;i < channels + index; i++)
        tpc_set_bit(PIOC, i - MOTOR_d0 + 18, i - MOTOR_d0 + 18, 0);
}

