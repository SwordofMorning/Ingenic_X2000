#include <stdio.h>
#include <string.h>
#include <driver/irq.h>
#include <delay.h>
#include <driver/gpio.h>

static void m_gpio_irq_handler(int irq, void *data)
{
    printf("irq: %d %d\n", irq, gpio_get_value(irq_to_gpio(irq)));
}

void gpio_example(void)
{
    int test_gpio1 = GPIO_PB(1);
    int test_gpio2 = GPIO_PC(8);

    gpio_set_func(test_gpio2, GPIO_OUTPUT0);
    gpio_set_value(test_gpio2, 1);

    request_irq(gpio_to_irq(test_gpio1), IRQ_TYPE_EDGE_BOTH, m_gpio_irq_handler, "gpio-pb01", NULL);

    /* 测试现象: 将pc8与pb1相连 每秒进两次中断 */
    while (1) {
        mdelay(500);
        gpio_set_value(test_gpio2, 0);
        mdelay(500);
        gpio_set_value(test_gpio2, 1);
    }
}