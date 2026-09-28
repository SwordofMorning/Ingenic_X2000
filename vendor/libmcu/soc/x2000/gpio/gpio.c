#include <stdio.h>
#include <driver/irq.h>
#include <driver/gpio.h>

#include <soc/base.h>
#include <soc/gpio.h>

#include <cpu/ffs.h>
#include <assert.h>

#define GPIO_PORT_OFF    0x100

#define PXPIN      0x00   /* PIN Level Register */
#define PXINT      0x10   /* Port Interrupt Register */
#define PXINTS     0x14   /* Port Interrupt Set Register */
#define PXINTC     0x18   /* Port Interrupt Clear Register */
#define PXMSK      0x20   /* Port Interrupt Mask Reg */
#define PXMSKS     0x24   /* Port Interrupt Mask Set Reg */
#define PXMSKC     0x28   /* Port Interrupt Mask Clear Reg */
#define PXPAT1     0x30   /* Port Pattern 1 Set Reg. */
#define PXPAT1S    0x34   /* Port Pattern 1 Set Reg. */
#define PXPAT1C    0x38   /* Port Pattern 1 Clear Reg. */
#define PXPAT0     0x40   /* Port Pattern 0 Register */
#define PXPAT0S    0x44   /* Port Pattern 0 Set Register */
#define PXPAT0C    0x48   /* Port Pattern 0 Clear Register */
#define PXFLG      0x50   /* Port Flag Register */
#define PXFLGC     0x58   /* Port Flag clear Register */
#define PXPU       0x80   /* Port PULL-UP State Register */
#define PXPUS      0x84   /* Port PULL-UP State Set Register */
#define PXPUC      0x88   /* Port PULL-UP State Clear Register */
#define PXPD       0x90   /* Port PULL-DOWN State Register */
#define PXPDS      0x94   /* Port PULL-DOWN State Set Register */
#define PXPDC      0x98   /* Port PULL-DOWN State Clear Register */

#define PZGID2LD   0xF0   /* GPIOZ Group ID to load */

struct gpio_data {
    unsigned int request_map;
    unsigned int irq_map;
    unsigned int irq_both_edge;
    unsigned int is_enable;
};

struct gpio_data gpiodata[5];

static const unsigned long gpio_irqbase[] = {
    [GPIO_PORT_A] = IRQ_GPIO_START + 0 * 32,
    [GPIO_PORT_B] = IRQ_GPIO_START + 1 * 32,
    [GPIO_PORT_C] = IRQ_GPIO_START + 2 * 32,
    [GPIO_PORT_D] = IRQ_GPIO_START + 3 * 32,
    [GPIO_PORT_E] = IRQ_GPIO_START + 4 * 32,
};

#define to_irqtype(flags) ((flags) & 0x0f)

static const unsigned long gpiobase[] = {
    [0] = GPIO_IOBASE + 0 * GPIO_PORT_OFF,
    [1] = GPIO_IOBASE + 1 * GPIO_PORT_OFF,
    [2] = GPIO_IOBASE + 2 * GPIO_PORT_OFF,
    [3] = GPIO_IOBASE + 3 * GPIO_PORT_OFF,
    [4] = GPIO_IOBASE + 4 * GPIO_PORT_OFF,
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
            gpio_write(port, PXINTS, pins);
        else
            gpio_write(port, PXINTC, pins);

        if (func & 0x4)
            gpio_write(port, PXMSKS, pins);
        else
            gpio_write(port, PXMSKC, pins);

        if (func & 0x2)
            gpio_write(port, PXPAT1S, pins);
        else
            gpio_write(port, PXPAT1C, pins);

        if (func & 0x1)
            gpio_write(port, PXPAT0S, pins);
        else
            gpio_write(port, PXPAT0C, pins);
    }

    if (func & 0x80) {
        int pull = (func >> 5) & 0x3;
        if (pull == 0) { // no pull
            gpio_write(port, PXPUC, pins);
            gpio_write(port, PXPDC, pins);
        }
        if (pull == 1) { // pull up
            gpio_write(port, PXPDC, pins);
            gpio_write(port, PXPUS, pins);
        }
        if (pull == 2) { // pull down
            gpio_write(port, PXPUC, pins);
            gpio_write(port, PXPDS, pins);
        }
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

static inline void hal_gpio_clear_irqflag(int gpio)
{
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    gpio_write(port, PXFLGC, 1 << pin);/* 清除中断标志位 */
}

static inline int hal_gpio_port_get_irqflag(enum gpio_port port)
{
    return gpio_read(port, PXFLG) & ~gpio_read(port, PXMSK);/* 读取中断标志位 且判断对应位是否中断使能 */
}

static inline int hal_gpio_get_value(int gpio)
{
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    return !!(gpio_read(port, PXPIN) & (1 << pin));
}

static inline void hal_gpio_unmask_irq(int gpio)
{
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    gpio_write(port, PXFLGC, 1 << pin);/* 清除中断标志位 */
    gpio_write(port, PXMSKC, 1 << pin);/* 使能该引脚作为中断源 or 被作为device引脚 */
}

static inline void hal_gpio_mask_irq(int gpio)
{
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    gpio_write(port, PXMSKS, 1 << pin);/* 屏蔽引脚作为中断源 or 对应引脚被作为GPIO */
}

static void gpio_port_enable_irq(int port)
{
    if (!gpiodata[port].is_enable++)
        enable_irq(IRQ_GPIO0 - port);
}

static void gpio_port_disable_irq(int port)
{
    if (!--gpiodata[port].is_enable)
        disable_irq(IRQ_GPIO0 - port);
}

void soc_gpio_enable_irq(int irq)
{
    int gpio = irq - IRQ_GPIO_START;
    enum gpio_port port = gpio / 32;

    gpio_port_enable_irq(port);

    hal_gpio_clear_irqflag(gpio);
    hal_gpio_unmask_irq(gpio);
}

void soc_gpio_disable_irq(int irq)
{
    int gpio = irq - IRQ_GPIO_START;
    enum gpio_port port = gpio / 32;

    hal_gpio_mask_irq(gpio);

    gpio_port_disable_irq(port);
}

void soc_gpio_shutdown_irq(int irq)
{
    int gpio = irq - IRQ_GPIO_START;
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    gpiodata[port].irq_map &= ~(1 << pin);
    hal_gpio_port_set_func(port, 1 << pin, GPIO_INPUT);
}

void soc_gpio_startup_irq(int irq, unsigned int irqflags)
{
    int gpio = irq - IRQ_GPIO_START;
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;
    enum gpio_function func;
    int type = to_irqtype(irqflags);

    gpiodata[port].irq_both_edge &= ~(1 << pin);
    gpiodata[port].irq_map |= 1 << pin;

    switch (type)
    {
    case IRQ_TYPE_LEVEL_LOW:
        func = GPIO_INT_LO;/* 低电平触发中断 */
        break;
    case IRQ_TYPE_LEVEL_HIGH:
        func = GPIO_INT_HI;/* 高电平触发中断 */
        break;
    case IRQ_TYPE_EDGE_FALLING:
        func = GPIO_INT_FE;/* 下降沿触发中断 */
        break;
    case IRQ_TYPE_EDGE_RISING:
        func = GPIO_INT_RE;/* 上升沿触发中断 */
        break;
    case IRQ_TYPE_EDGE_BOTH:/* 双边沿触发中断 */
        func = hal_gpio_get_value(gpio) ? GPIO_INT_FE : GPIO_INT_RE;
        gpiodata[port].irq_both_edge |= 1 << pin;
        break;
    default:
        assert(0);
        return;
    }

    // 默认屏蔽中断,相当于hal_gpio_mask_irq(gpio);
    func |= 4;

    hal_gpio_port_set_func(port, 1 << pin, func);
}

static void gpio_irq_handler(int irq, void *data)
 {
    enum gpio_port port = (enum gpio_port) data;
    unsigned long flag = hal_gpio_port_get_irqflag(port);/* 获取中断标志位 */
    int pin = __ffs(flag);

    if (!flag)
        hang();

    gpio_write(port, PXFLGC, 1 << pin);/* 清除中断标志位 */

    if (gpiodata[port].irq_both_edge & (1 << pin)) {
        if (gpio_read(port, PXPIN) & (1 << pin))/* 端口有电平输入 */
            gpio_write(port, PXPAT0C, (1 << pin));/* 低电平或者下降沿触发 */
        else
            gpio_write(port, PXPAT0S, (1 << pin));/* 高电平或者上升沿触发 */
    }

    handle_irq(gpio_irqbase[port] + pin);
}

void soc_gpio_irq_init(void)
{
    request_irq_disabled(IRQ_GPIO0, 0, gpio_irq_handler, "GPIO_PA", (void *)GPIO_PORT_A);
    request_irq_disabled(IRQ_GPIO1, 0, gpio_irq_handler, "GPIO_PB", (void *)GPIO_PORT_B);
    request_irq_disabled(IRQ_GPIO2, 0, gpio_irq_handler, "GPIO_PC", (void *)GPIO_PORT_C);
    request_irq_disabled(IRQ_GPIO3, 0, gpio_irq_handler, "GPIO_PD", (void *)GPIO_PORT_D);
    request_irq_disabled(IRQ_GPIO4, 0, gpio_irq_handler, "GPIO_PE", (void *)GPIO_PORT_E);
}

void gpio_enable_irq(int irq)
{
    soc_gpio_enable_irq(irq);
}

void gpio_disable_irq(int irq)
{
    soc_gpio_disable_irq(irq);
}

void gpio_startup_irq(int irq, unsigned int irqflags)
{
    soc_gpio_startup_irq(irq, irqflags);
}

void gpio_shutdown_irq(int irq)
{
    soc_gpio_shutdown_irq(irq);
}

void gpio_init_irq(void)
{
    soc_gpio_irq_init();
}

void gpio_init(void)
{
    return ;
 }
