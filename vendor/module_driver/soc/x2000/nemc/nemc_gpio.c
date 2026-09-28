#include <common.h>
#include <bit_field.h>
#include <linux/gpio.h>

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

#define NEMC_rd   13
#define NEMC_wr   14
#define NEMC_wait 15

#define NEMC_d0   16
#define NEMC_d1   17
#define NEMC_d2   18
#define NEMC_d3   19
#define NEMC_d4   20
#define NEMC_d5   21
#define NEMC_d6   22
#define NEMC_d7   23
#define NEMC_d8   24
#define NEMC_d9   25
#define NEMC_d10  26
#define NEMC_d11  27
#define NEMC_d12  28
#define NEMC_d13  29
#define NEMC_d14  30
#define NEMC_d15  31

#define NEMC_cs1 GPIO_PC(23)
#define NEMC_cs2 GPIO_PC(24)

#define NEMC_name_cs1 "NEMC_cs1"
#define NEMC_name_cs2 "NEMC_cs2"


static char nemc_gpio_name[][10] = {
    [NEMC_sa0] = "NEMC_sa0",
    [NEMC_sa1] = "NEMC_sa1",
    [NEMC_sa2] = "NEMC_sa2",
    [NEMC_sa3] = "NEMC_sa3",
    [NEMC_sa4] = "NEMC_sa4",
    [NEMC_sa5] = "NEMC_sa5",
    [NEMC_sa6] = "NEMC_sa6",
    [NEMC_sa7] = "NEMC_sa7",
    [NEMC_sa8] = "NEMC_sa8",
    [NEMC_sa9] = "NEMC_sa9",
    [NEMC_sa10] = "NEMC_sa10",
    [NEMC_sa11] = "NEMC_sa11",
    [NEMC_sa12] = "NEMC_sa12",

    [NEMC_rd] = "NEMC_rd",
    [NEMC_wr] = "NEMC_wr",
    [NEMC_wait] = "NEMC_wait",

    [NEMC_d0] = "NEMC_d0",
    [NEMC_d1] = "NEMC_d1",
    [NEMC_d2] = "NEMC_d2",
    [NEMC_d3] = "NEMC_d3",
    [NEMC_d4] = "NEMC_d4",
    [NEMC_d5] = "NEMC_d5",
    [NEMC_d6] = "NEMC_d6",
    [NEMC_d7] = "NEMC_d7",
    [NEMC_d8] = "NEMC_d8",
    [NEMC_d9] = "NEMC_d9",
    [NEMC_d10] = "NEMC_d10",
    [NEMC_d11] = "NEMC_d11",
    [NEMC_d12] = "NEMC_d12",
    [NEMC_d13] = "NEMC_d13",
    [NEMC_d14] = "NEMC_d14",
    [NEMC_d15] = "NEMC_d15",
};

static unsigned int m_pins;
static unsigned int m_cs;

static int nemc_addr[13] = {0};

module_param_named(nemc_addr0, nemc_addr[0], int, 0644);
module_param_named(nemc_addr1, nemc_addr[1], int, 0644);
module_param_named(nemc_addr2, nemc_addr[2], int, 0644);
module_param_named(nemc_addr3, nemc_addr[3], int, 0644);
module_param_named(nemc_addr4, nemc_addr[4], int, 0644);
module_param_named(nemc_addr5, nemc_addr[5], int, 0644);
module_param_named(nemc_addr6, nemc_addr[6], int, 0644);
module_param_named(nemc_addr7, nemc_addr[7], int, 0644);
module_param_named(nemc_addr8, nemc_addr[8], int, 0644);
module_param_named(nemc_addr9, nemc_addr[9], int, 0644);
module_param_named(nemc_addr10, nemc_addr[10], int, 0644);
module_param_named(nemc_addr11, nemc_addr[11], int, 0644);
module_param_named(nemc_addr12, nemc_addr[12], int, 0644);


static void nemc_release_pins(void)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (m_pins & (1 << i))
            gpio_free(GPIO_PB(i));
    }

    m_pins = 0;
}

static void nemc_request_gpio(int bus_width)
{
    int i;
    int ret;

    for (i = 0; i < NEMC_rd; i++) {
        if (!nemc_addr[i])
            continue;

        ret = gpio_request(GPIO_PB(i), nemc_gpio_name[i]);
        if (ret < 0) {
            printk(KERN_ERR "nemc: failed to request GPIO_PB%d\n", i);
            nemc_release_pins();
            return;
        }

        m_pins |= (1 << i);
        gpio_set_func(GPIO_PB(i), GPIO_FUNC_0);
    }

    for(i = NEMC_rd; i < bus_width + 16; i++) {
        ret = gpio_request(GPIO_PB(i), nemc_gpio_name[i]);
        if (ret < 0) {
            printk(KERN_ERR "nemc: failed to request GPIO_PB%d\n", i);
            nemc_release_pins();
            return;
        }

        m_pins |= (1 << i);
        gpio_set_func(GPIO_PB(i), GPIO_FUNC_0);
    }
}

static void nemc_request_cs_gpio(int cs1, int cs2)
{
    if (cs1) {
        gpio_request(NEMC_cs1, NEMC_name_cs1);
        gpio_set_func(NEMC_cs1, GPIO_FUNC_3);
        m_cs |= 1;
    }

    if (cs2) {
        gpio_request(NEMC_cs2, NEMC_name_cs2);
        gpio_set_func(NEMC_cs2, GPIO_FUNC_3);
        m_cs |= 1 << 1;
    }
}

static void nemc_release_cs_gpio(void)
{
    if (m_cs & 1)
        gpio_free(NEMC_cs1);

    if (m_cs & 1 << 1)
        gpio_free(NEMC_cs2);
}

static void nemc_init_gpio(int bus_width, int cs1, int cs2)
{
    nemc_request_gpio(bus_width);
    nemc_request_cs_gpio(cs1, cs2);
}

static void nemc_deinit_gpio(void)
{
    nemc_release_pins();
    nemc_release_cs_gpio();
}


