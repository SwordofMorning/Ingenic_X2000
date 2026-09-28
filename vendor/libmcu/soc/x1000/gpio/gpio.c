#include <stdio.h>
#include <driver/irq.h>
#include <driver/gpio.h>

#include <soc/base.h>
#include <soc/gpio.h>

#define GPIO_PORT_OFF    0x100
#define GPIO_SHADOW_OFF  0x700

#define PXPIN		0x00   /* PIN Level Register */
#define PXINT		0x10   /* Port Interrupt Register */
#define PXINTS		0x14   /* Port Interrupt Set Register */
#define PXINTC		0x18   /* Port Interrupt Clear Register */
#define PXMSK		0x20   /* Port Interrupt Mask Reg */
#define PXMSKS		0x24   /* Port Interrupt Mask Set Reg */
#define PXMSKC		0x28   /* Port Interrupt Mask Clear Reg */
#define PXPAT1		0x30   /* Port Pattern 1 Set Reg. */
#define PXPAT1S		0x34   /* Port Pattern 1 Set Reg. */
#define PXPAT1C		0x38   /* Port Pattern 1 Clear Reg. */
#define PXPAT0		0x40   /* Port Pattern 0 Register */
#define PXPAT0S		0x44   /* Port Pattern 0 Set Register */
#define PXPAT0C		0x48   /* Port Pattern 0 Clear Register */
#define PXFLG		0x50   /* Port Flag Register */
#define PXFLGC		0x58   /* Port Flag clear Register */
#define PXOENS		0x64   /* Port Output Disable Set Register */
#define PXOENC		0x68   /* Port Output Disable Clear Register */
#define PXPEN		0x70   /* Port Pull Disable Register */
#define PXPENS		0x74   /* Port Pull Disable Set Register */
#define PXPENC		0x78   /* Port Pull Disable Clear Register */
#define PXDSS		0x84   /* Port Drive Strength set Register */
#define PXDSC		0x88   /* Port Drive Strength clear Register */
#define PZGID2LD	0xF0   /* GPIOZ Group ID to load */

#define SHADOW 6

static const unsigned long gpiobase[] = {
    [0] = GPIO_IOBASE + 0 * GPIO_PORT_OFF,
    [1] = GPIO_IOBASE + 1 * GPIO_PORT_OFF,
    [2] = GPIO_IOBASE + 2 * GPIO_PORT_OFF,
    [3] = GPIO_IOBASE + 3 * GPIO_PORT_OFF,
    [4] = GPIO_IOBASE + 4 * GPIO_PORT_OFF,
    [5] = GPIO_IOBASE + 5 * GPIO_PORT_OFF,

    [SHADOW] = GPIO_IOBASE + GPIO_SHADOW_OFF,
};

#define GPIO_ADDR(port, reg) ((volatile unsigned long *)(gpiobase[port] + reg))

static inline void gpio_write(int port, unsigned int reg, int val)
{
    *GPIO_ADDR(port, reg) = val;
}

static inline unsigned int gpio_read(int port, unsigned int reg)
{
    return *GPIO_ADDR(port, reg);
}

static void hal_gpio_port_set_func(int port, unsigned int pins, enum gpio_function func)
{
    /* func option */
    if (func & 0x10) {
        if (func & 0x8)
            gpio_write(SHADOW, PXINTS, pins);
        else
            gpio_write(SHADOW, PXINTC, pins);

        if (func & 0x4)
            gpio_write(SHADOW, PXMSKS, pins);
        else
            gpio_write(SHADOW, PXMSKC, pins);

        if (func & 0x2)
            gpio_write(SHADOW, PXPAT1S, pins);
        else
            gpio_write(SHADOW, PXPAT1C, pins);

        if (func & 0x1)
            gpio_write(SHADOW, PXPAT0S, pins);
        else
            gpio_write(SHADOW, PXPAT0C, pins);

        /* configure PzGID2LD to specify which port group to load */
        gpio_write(SHADOW, PZGID2LD, port);
    }

    if (func & 0x80) {
        if (func & 0x20)
            gpio_write(port, PXPENC, pins);
        else
            gpio_write(port, PXPENS, pins);
    }
}

int gpio_set_func(int gpio, enum gpio_function func)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    hal_gpio_port_set_func(port, 1 << pin, func);

    return 0;
}

void gpio_direction_output(int gpio, int value)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    hal_gpio_port_set_func(port, 1 << pin, value ? GPIO_OUTPUT1 : GPIO_OUTPUT0);
}

void gpio_direction_input(int gpio)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    hal_gpio_port_set_func(port, 1 << pin, GPIO_INPUT);
}

void gpio_set_value(int gpio, int value)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    if (value)
        gpio_write(port, PXPAT0S, 1 << pin);
    else
        gpio_write(port, PXPAT0C, 1 << pin);
}

int gpio_get_value(int gpio)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    return !!(gpio_read(port, PXPIN) & (1 << pin));
}

void gpio_enable_irq(int irq)
{

}

void gpio_disable_irq(int irq)
{

}

void gpio_startup_irq(int irq, unsigned int irqflags)
{

}

void gpio_shutdown_irq(int irq)
{

}

void gpio_init_irq(void)
{

}

void gpio_init(void)
{

}
