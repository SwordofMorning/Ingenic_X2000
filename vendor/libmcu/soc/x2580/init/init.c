#include <stdio.h>
#include <driver/systick.h>
#include <driver/irq.h>
#include <driver/console.h>
#include <driver/uart.h>
#include <driver/clk.h>
#include <driver/gpio.h>
#include <cpu/host_cpu.h>
#include <driver/hrtimer.h>

__attribute__((aligned(16))) char user_stack_mem[APP_libmcu_x2580_stack_size];

static inline void show_compile_time(void)
{
    printf("risc-v mcu @ %s %s\n", __DATE__, __TIME__);
}

void __libc_init_array(void);
void vendor_init(void);

void c_main(void)
{
#ifdef APP_libmcu_driver_systick
    systick_init();
#endif

#ifdef APP_libmcu_driver_uart
    uart_init();
#endif

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

#ifdef APP_libmcu_driver_irq
    host_cpu_irq_init();
#endif

#ifdef APP_libmcu_show_compile_time
    show_compile_time();
#endif

#ifdef APP_libmcu_driver_hrtimer
    hrtimer_core_init();
#endif

#ifdef APP_libmcu_driver_i2c
    i2c_init();
#endif

    __libc_init_array();

    vendor_init();

}