root.COMMON_FLAGS += -mips32 -mabi=32 -EL
root.COMMON_FLAGS += -G 0  -fno-pic -static -nostdlib -Wall -msoft-float -mno-abicalls
root.COMMON_FLAGS += -Isoc/x2000/include/

root.LDFLAGS += -G 0 -static -n -EL -m elf32ltsmip
root.TARGET_LDFLAGS += -T soc/x2000/x2000.lds
root.TARGET_LDFLAGS += -G 0 -static -n -EL -m elf32ltsmip -Bstatic
CROSS_COMPILE = mips-linux-gnu-

root.TARGET_LIBS += soc/x2000/libgcc.a

src-y += init/start.S
src-y += init/init.c
src-y += init/host_cpu.c
src-$(APP_libmcu_x2000_irq) += init/irq.c
src-$(APP_libmcu_x2000_irq) += init/exception.S
src-$(APP_libmcu_x2000_irq) += intc/intc.c
src-$(APP_libmcu_x2000_irq) += gpio/gpio.c
src-$(APP_libmcu_x2000_gpio) += gpio/gpio.c
src-$(APP_libmcu_x2000_uart) += uart/uart.c
src-$(APP_libmcu_x2000_clk) += clk/clk.c
src-$(APP_libmcu_x2000_clk) += clk/clk_pll.c
src-$(APP_libmcu_x2000_clk) += clk/clk_cpccr.c
src-$(APP_libmcu_x2000_clk) += clk/clk_cgu.c
src-$(APP_libmcu_x2000_clk) += clk/clk_gate.c
src-$(APP_libmcu_x2000_systick) += systick/systick.c
src-$(APP_libmcu_x2000_tcu) += tcu/tcu.c
src-$(APP_libmcu_x2000_hrtimer) += hrtimer/hrtimer.c
src-$(APP_libmcu_x2000_adc) += adc/adc.c adc/hal/adc_hal.c
src-$(APP_libmcu_x2000_pwm) += pwm/pwm.c

src-$(APP_libmcu_x2000_i2c) += i2c/i2c.c
