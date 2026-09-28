#include <stdio.h>
#include <driver/systick.h>
#include <driver/irq.h>
#include <driver/console.h>
#include <driver/uart.h>
#include <driver/clk.h>
#include <driver/tcu.h>
#include <driver/gpio.h>
#include <driver/fb.h>
#include <cpu/host_cpu.h>
#include <driver/adc.h>
#include <driver/dma.h>
#include <driver/spi.h>
#include <device/lcd.h>
#include <driver/hrtimer.h>
#include <cpu/tcsm_section.h>

__tcsm_bss __attribute__((aligned(16))) char user_stack_mem[APP_libmcu_x2600_stack_size];

void show_compile_time(void)
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

#ifdef APP_libmcu_driver_tcu
    tcu_init();
#endif

#ifdef APP_libmcu_driver_hrtimer
    hrtimer_core_init();
#endif

#ifdef APP_libmcu_driver_adc
    adc_init();
#endif

#ifdef APP_libmcu_driver_dma
    dma_init();
#endif

#ifdef APP_libmcu_driver_fb
    fb_init();
#endif

#ifdef APP_libmcu_driver_spi
    spi_init();
#endif

#ifdef APP_libmcu_device_lcd
    lcd_init();
#endif

#ifdef APP_libmcu_driver_i2c
    i2c_init();
#endif

#ifdef APP_libmcu_show_compile_time
    show_compile_time();
#endif

    __libc_init_array();

    vendor_init();
}