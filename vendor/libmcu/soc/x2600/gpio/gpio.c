#include <stdio.h>
#include <driver/irq.h>
#include <driver/gpio.h>
#include <cpu/irqflags.h>
#include <cpu/tcsm_section.h>

#include <soc/base.h>
#include <soc/gpio.h>

#include <assert.h>
#include <bit_field2.h>
#include <bits_opt.h>

#define GPIO_PORT_OFF    0x1000

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

#define PXDS0      0xA0  /* Drive Strength Register0 */
#define PXDS0S     0xA4  /* Drive Strength Set Register0 */
#define PXDS0C     0xA8  /* Drive Strength Clear Register0 */
#define PXDS1      0xB0  /* Drive Strength Register1 */
#define PXDS1S     0xB4  /* Drive Strength Set Register1 */
#define PXDS1C     0xB8  /* Drive Strength Clear Register1 */

#define PXSMT      0xE0   /* Port Schmitt Trigger Register */
#define PXSMTS     0xE4   /* Port Schmitt Trigger set Register */
#define PXSMTC     0xE8   /* Port Schmitt Trigger clear Register */

#define PXEDG      0x70   /* Port Dual-Edge Interrupt Register */
#define PXEDGS     0x74   /* Port Dual-Edge Interrupt Set Register */
#define PXEDGC     0x78   /* Port Dual-Edge Interrupt Clear Register */
#define PXMMSK     0x170  /* Port Interrupt Mask Register For RISC-V */
#define PXMMSKS    0x174  /* Port Interrupt Mask Set Register For RISC-V */
#define PXMMSKC    0x178  /* Port Interrupt Mask Clear Register For RISC-V */
#define PXMPEND    0x190  /* Port Interrupt Pending Register For RISC-V */
#define PXSPCFG    0x200  /* Port Single Pin Function Configuration Register */
#define PXMPSEL1   0x20C  /* Port Multi Pin Select Register 1 */
#define PXMPCFG1   0x210  /* Port Multi Pin Configuration Register 1 */

struct gpio_data {
    unsigned int request_map;
    unsigned int irq_map;
    unsigned int is_enable;
};

static __tcsm_bss struct gpio_data gpiodata[5];

static __tcsm_data const unsigned long gpio_irqbase[] = {
    [GPIO_PORT_A] = IRQ_GPIO_START + 0 * 32,
    [GPIO_PORT_B] = IRQ_GPIO_START + 1 * 32,
    [GPIO_PORT_C] = IRQ_GPIO_START + 2 * 32,
    [GPIO_PORT_D] = IRQ_GPIO_START + 3 * 32,
    [GPIO_PORT_E] = IRQ_GPIO_START + 4 * 32,
};

#define to_irqtype(flags) ((flags) & 0x0f)

static __tcsm_data const unsigned long gpiobase[] = {
    [0] = GPIO_IOBASE + 0 * GPIO_PORT_OFF,
    [1] = GPIO_IOBASE + 1 * GPIO_PORT_OFF,
    [2] = GPIO_IOBASE + 2 * GPIO_PORT_OFF,
    [3] = GPIO_IOBASE + 3 * GPIO_PORT_OFF,
    [4] = GPIO_IOBASE + 4 * GPIO_PORT_OFF,
};

#define GPIO_ADDR(port, reg) ((volatile unsigned long *)(gpiobase[port] + reg))

static inline void __tcsm_text gpio_write(int port, unsigned int reg, int val)
{
    *GPIO_ADDR(port, reg) = val;
}

static inline unsigned int __tcsm_text gpio_read(int port, unsigned int reg)
{
    return *GPIO_ADDR(port, reg);
}

static void __tcsm_text hal_gpio_port_set_func(int port, unsigned int pins, enum gpio_function func)
{
    unsigned long flags;

    local_irq_save(flags);

    /* func option */
    if (func & BIT(6)) {
        unsigned int cfg = get_bit_field(func, 0, 5);
        gpio_write(port, PXMPSEL1, pins);
        gpio_write(port, PXMPCFG1, cfg);
    }

    if (func & BIT(9)) {
        int pull = get_bit_field(func, 7, 8);
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

    local_irq_restore(flags);
}

static void hal_gpio_port_set_strength(int port, unsigned int pins, int strength)
{
    if (strength & BIT(0))
        gpio_write(port, PXDS0S, pins);
    else
        gpio_write(port, PXDS0C, pins);

    if (strength & BIT(1))
        gpio_write(port, PXDS1S, pins);
    else
        gpio_write(port, PXDS1C, pins);
}

void gpio_set_strength(int gpio, int strength)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    hal_gpio_port_set_strength(port, 1 << pin, strength);
}

static void hal_gpio_port_set_schmitt(int port, unsigned int pins, int enable)
{
    if (enable)
        gpio_write(port, PXSMTS, pins);
    else
        gpio_write(port, PXSMTC, pins);
}

void gpio_set_schmitt(int gpio, int enable)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    hal_gpio_port_set_schmitt(port, 1 << pin, enable);
}

int __tcsm_text gpio_set_func(int gpio, enum gpio_function func)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    hal_gpio_port_set_func(port, 1 << pin, func);

    return 0;
}

void __tcsm_text gpio_direction_output(int gpio, int value)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    hal_gpio_port_set_func(port, 1 << pin, value ? GPIO_OUTPUT1 : GPIO_OUTPUT0);
}

void __tcsm_text gpio_direction_input(int gpio)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    hal_gpio_port_set_func(port, 1 << pin, GPIO_INPUT);
}

void __tcsm_text gpio_set_value(int gpio, int value)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    if (value)
        gpio_write(port, PXPAT0S, 1 << pin);
    else
        gpio_write(port, PXPAT0C, 1 << pin);
}

int __tcsm_text gpio_get_value(int gpio)
{
    int port = gpio / 32;
    int pin = gpio % 32;

    return !!(gpio_read(port, PXPIN) & (1 << pin));
}

static inline void __tcsm_text hal_gpio_clear_irqflag(int gpio)
{
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    gpio_write(port, PXFLGC, 1 << pin);/* 清除中断标志位 */
}

static inline __tcsm_text int hal_gpio_port_get_irqflag(enum gpio_port port)
{
    return gpio_read(port, PXMPEND);/* 读取中断标志位 且判断对应位是否中断使能 */
}

static inline void __tcsm_text hal_gpio_unmask_irq(int gpio)
{
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    gpio_write(port, PXFLGC, 1 << pin);/* 清除中断标志位 */
    gpio_write(port, PXMMSKC, 1 << pin);/* 使能该引脚作为中断源 or 被作为device引脚 */
}

static inline void __tcsm_text hal_gpio_mask_irq(int gpio)
{
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    gpio_write(port, PXMMSKS, 1 << pin);/* 屏蔽引脚作为中断源 or 对应引脚被作为GPIO */
}

static void __tcsm_text gpio_port_enable_irq(int port)
{
    if (!gpiodata[port].is_enable++)
        enable_irq(IRQ_GPIO0 - port);
}

static void __tcsm_text gpio_port_disable_irq(int port)
{
    if (!--gpiodata[port].is_enable)
        disable_irq(IRQ_GPIO0 - port);
}

void __tcsm_text gpio_enable_irq(int irq)
{
    int gpio = irq - IRQ_GPIO_START;
    enum gpio_port port = gpio / 32;

    gpio_port_enable_irq(port);

    hal_gpio_clear_irqflag(gpio);
    hal_gpio_unmask_irq(gpio);
}

void __tcsm_text gpio_disable_irq(int irq)
{
    int gpio = irq - IRQ_GPIO_START;
    enum gpio_port port = gpio / 32;

    hal_gpio_mask_irq(gpio);

    gpio_port_disable_irq(port);
}

void gpio_shutdown_irq(int irq)
{
    int gpio = irq - IRQ_GPIO_START;
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    gpiodata[port].irq_map &= ~(1 << pin);
    hal_gpio_port_set_func(port, 1 << pin, GPIO_INPUT);
}

void gpio_startup_irq(int irq, unsigned int irqflags)
{
    int gpio = irq - IRQ_GPIO_START;
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;
    enum gpio_function func;
    int type = to_irqtype(irqflags);

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
    case IRQ_TYPE_EDGE_BOTH:
        func = GPIO_INT_DE;/* 双边沿触发中断 */
        break;
    default:
        assert(0);
        return;
    }

    // 默认屏蔽中断,相当于hal_gpio_mask_irq(gpio);
    func |= BIT(2);

    hal_gpio_port_set_func(port, 1 << pin, func);
}

static void __tcsm_text gpio_irq_handler(int irq, void *data)
 {
    enum gpio_port port = (enum gpio_port) data;
    unsigned long flag = hal_gpio_port_get_irqflag(port);/* 获取中断标志位 */
    int pin = 31 - __builtin_clz(flag);

    if (!flag)
        hang();

    gpio_write(port, PXFLGC, 1 << pin);/* 清除中断标志位 */

    handle_irq(gpio_irqbase[port] + pin);
}

void gpio_init_irq(void)
{
    gpio_write(GPIO_PORT_A, PXMMSK, 0xffffffff);
    gpio_write(GPIO_PORT_B, PXMMSK, 0xffffffff);
    gpio_write(GPIO_PORT_C, PXMMSK, 0xffffffff);
    gpio_write(GPIO_PORT_D, PXMMSK, 0xffffffff);
    gpio_write(GPIO_PORT_E, PXMMSK, 0xffffffff);

    request_irq_disabled(IRQ_GPIO0, 0, gpio_irq_handler, "GPIO_PA", (void *)GPIO_PORT_A);
    request_irq_disabled(IRQ_GPIO1, 0, gpio_irq_handler, "GPIO_PB", (void *)GPIO_PORT_B);
    request_irq_disabled(IRQ_GPIO2, 0, gpio_irq_handler, "GPIO_PC", (void *)GPIO_PORT_C);
    request_irq_disabled(IRQ_GPIO3, 0, gpio_irq_handler, "GPIO_PD", (void *)GPIO_PORT_D);
    request_irq_disabled(IRQ_GPIO4, 0, gpio_irq_handler, "GPIO_PE", (void *)GPIO_PORT_E);
}

void gpio_init(void)
{
    return ;
}
