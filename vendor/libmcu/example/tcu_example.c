#include <stdio.h>
#include <string.h>
#include <driver/tcu.h>
#include <delay.h>
#include <driver/gpio.h>

static void tcu_irq_cb(int id)
{
    printf("tcu: irq id: %d\n", id);
}

void tcu_example(void)
{
    // tcu 用做 纯 timer
    struct tcu_config tcu_timer;
    tcu_config_timer_count(&tcu_timer, 7, 30000, tcu_irq_cb);
    tcu_timer.clk_div = clk_div_1024;
    tcu_enable(&tcu_timer);
    // tcu_set_full_value(&tcu_timer, 30000);

    struct tcu_config tcu_config;
    int count = 0;

    /* 以x2000为例 tcu4_gpio0为PC08 仅在tcu4_gpio0上升沿计数 */
    tcu_config_gpio0_up_count(&tcu_config, 4);
    tcu_enable(&tcu_config);

    /* 测试现象: 将pc8与pb1相连 打印2次/s 每次加1直至0xFFFF后清0 */
    while(1) {
        gpio_direction_output(GPIO_PB(1), 1);
        gpio_direction_output(GPIO_PB(1), 0);
        count = tcu_get_count(&tcu_config);
        printf("count = %d\n", count);
        mdelay(500);
    }
}