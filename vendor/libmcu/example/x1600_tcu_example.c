#include <stdio.h>
#include <string.h>
#include <driver/tcu.h>
#include <delay.h>
#include <driver/gpio.h>

void tcu_example(void)
{
    struct tcu_config tcu_config;
    int count = 0;

    /* 以x1600为例 tcu4_gpio0为PC00 仅在tcu4_gpio0上升沿计数 */
    tcu_config_gpio0_up_count(&tcu_config, 4);
    tcu_config_pc0_as_tcu4_gpio0(&tcu_config);
    tcu_enable(&tcu_config);

    /* 测试现象: 将pc0与pd2相连 打印2次/s 每次加1直至0xFFFF后清0 */
    while(1) {
        gpio_direction_output(GPIO_PD(2), 1);
        gpio_direction_output(GPIO_PD(2), 0);
        count = tcu_get_count(&tcu_config);
        printf("count = %d\n", count);
        mdelay(500);
    }
}