#include <stdio.h>
#include <soc/gpio.h>
#include <driver/gpio.h>
#include <driver/nemc.h>

#define NEMC_sa0  0
#define NEMC_sa1  1
#define NEMC_sa2  2
#define NEMC_sa3  3
#define NEMC_sa4  4
#define NEMC_sa5  5
#define NEMC_sa6  6
#define NEMC_sa7  7
#define NEMC_sa8  8
#define NEMC_sa9  9
#define NEMC_sa10 10
#define NEMC_sa11 11
#define NEMC_sa12 12
#define NEMC_sa13 13
#define NEMC_sa14 14
#define NEMC_sa15 15

#define NEMC_avd  16
#define NEMC_wait 17
#define NEMC_re   18
#define NEMC_we   19

#define NEMC_ad0   20
#define NEMC_ad1   21
#define NEMC_ad2   22
#define NEMC_ad3   23
#define NEMC_ad4   24
#define NEMC_ad5   25
#define NEMC_ad6   26
#define NEMC_ad7   27
#define NEMC_ad8   28
#define NEMC_ad9   29
#define NEMC_ad10  30
#define NEMC_ad11  31
#define NEMC_ad12  32
#define NEMC_ad13  33
#define NEMC_ad14  34
#define NEMC_ad15  35

#define NEMC_cs1 36
#define NEMC_cs2 37

static int nemc_gpios[] = {
    [NEMC_sa0]  = GPIO_PB(4),
    [NEMC_sa1]  = GPIO_PB(5),
    [NEMC_sa2]  = GPIO_PB(6),
    [NEMC_sa3]  = GPIO_PB(7),
    [NEMC_sa4]  = GPIO_PB(8),
    [NEMC_sa5]  = GPIO_PB(9),
    [NEMC_sa6]  = GPIO_PB(10),
    [NEMC_sa7]  = GPIO_PB(11),
    [NEMC_sa8]  = GPIO_PC(19),
    [NEMC_sa9]  = GPIO_PC(20),
    [NEMC_sa10] = GPIO_PC(21),
    [NEMC_sa11] = GPIO_PC(22),
    [NEMC_sa12] = GPIO_PC(23),
    [NEMC_sa13] = GPIO_PC(24),
    [NEMC_sa14] = GPIO_PD(8),
    [NEMC_sa15] = GPIO_PD(9),

    [NEMC_avd]  = GPIO_PB(30),
    [NEMC_wait] = GPIO_PD(11),
    [NEMC_re]   = GPIO_PB(28),
    [NEMC_we]   = GPIO_PB(29),

    [NEMC_ad0]  = GPIO_PB(12),
    [NEMC_ad1]  = GPIO_PB(13),
    [NEMC_ad2]  = GPIO_PB(14),
    [NEMC_ad3]  = GPIO_PB(15),
    [NEMC_ad4]  = GPIO_PB(16),
    [NEMC_ad5]  = GPIO_PB(17),
    [NEMC_ad6]  = GPIO_PB(18),
    [NEMC_ad7]  = GPIO_PB(19),
    [NEMC_ad8]  = GPIO_PB(20),
    [NEMC_ad9]  = GPIO_PB(21),
    [NEMC_ad10] = GPIO_PB(22),
    [NEMC_ad11] = GPIO_PB(23),
    [NEMC_ad12] = GPIO_PB(24),
    [NEMC_ad13] = GPIO_PB(25),
    [NEMC_ad14] = GPIO_PB(26),
    [NEMC_ad15] = GPIO_PB(27),

    [NEMC_cs1] = GPIO_PB(31),
    [NEMC_cs2] = GPIO_PD(10),
};

void nemc_init_gpio(struct nemc_config config)
{
    int i;
    int ad_width = config.data_width;

    if (config.mode == MUXED_MODE) {
        gpio_set_func(nemc_gpios[NEMC_avd], GPIO_FUNC_3);

        if (config.data_width < config.addr_width)
            ad_width += (config.addr_width - config.data_width);
    } else {
        for (i = NEMC_sa0; i <= NEMC_sa15 && i < config.addr_width; i++) {
            gpio_set_func(nemc_gpios[i], GPIO_FUNC_3);
        }
    }

    for(i = NEMC_ad0; i < ad_width + NEMC_ad0; i++) {
        gpio_set_func(nemc_gpios[i], GPIO_FUNC_3);
    }

    gpio_set_func(nemc_gpios[NEMC_re], GPIO_FUNC_3);
    gpio_set_func(nemc_gpios[NEMC_we], GPIO_FUNC_3);

    if (config.use_wait_pin)
        gpio_set_func(nemc_gpios[NEMC_wait], GPIO_FUNC_3);

    if (config.id == 0)
        gpio_set_func(nemc_gpios[NEMC_cs1], GPIO_FUNC_3);
    else
        gpio_set_func(nemc_gpios[NEMC_cs2], GPIO_FUNC_3);
}
