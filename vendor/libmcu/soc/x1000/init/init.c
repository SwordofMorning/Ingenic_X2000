#include <stdio.h>
#include <driver/irq.h>
#include <driver/clk.h>
#include <driver/uart.h>
#include <driver/console.h>
#include <driver/gpio.h>
#include <driver/systick.h>
#include <cpu/host_cpu.h>

#include <always_compile.h>

void vendor_init(void);

static inline void show_compile_time(void)
{
    printf("mcu @ %s %s\n", __DATE__, __TIME__);
}

void c_main(void)
{
#ifdef APP_libmcu_driver_console
    console_init();
#endif

#ifdef APP_libmcu_driver_irq
    irq_init();
#endif

#ifdef APP_libmcu_driver_clk
    clk_init();
#endif

#ifdef APP_libmcu_driver_gpio
    gpio_init();
#endif

#ifdef APP_libmcu_show_compile_time
    show_compile_time();
#endif

#ifdef APP_libmcu_driver_uart
    uart_init();
#endif

    vendor_init();

    printf("%s is called\n", __func__);

    while (1);
}
