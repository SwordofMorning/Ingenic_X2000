root.COMMON_FLAGS += -march=rv32im -mabi=ilp32
root.COMMON_FLAGS += -fno-pic -static -Wall -Wno-abi
root.COMMON_FLAGS += -Isoc/x2580/include/

root.CXXFLAGS += -std=c++11 -D_GNU_SOURCE
root.CXXFLAGS += -fno-exceptions -fno-unwind-tables -fno-threadsafe-statics

root.LDFLAGS += -static
root.TARGET_LDFLAGS += -T soc/x2580/x2580.lds
root.TARGET_LDFLAGS += -static -Bstatic -m elf32lriscv
CROSS_COMPILE = riscv32-unknown-elf-

LIBC_PATH = $(dir $(shell $(CC) -print-file-name=libc.a))
LIBGCC_PATH = $(dir $(shell $(CC) -print-file-name=libgcc.a))

root.TARGET_LDFLAGS += -L $(LIBC_PATH) -L $(LIBGCC_PATH)

root.TARGET_LIBS += -lc -lgcc

src-y += init/start.S
src-y += init/bzero.S
src-y += init/init.c
src-y += init/cpu.c
src-y += init/host_cpu.c
src-y += init/uncache_mem.c
src-$(APP_libmcu_x2580_irq) += init/irq.c
src-$(APP_libmcu_x2580_irq) += init/intc.c

src-$(APP_libmcu_x2580_clk) += clk/clk.c
src-$(APP_libmcu_x2580_clk) += clk/clk_pll.c
src-$(APP_libmcu_x2580_clk) += clk/clk_cpccr.c
src-$(APP_libmcu_x2580_clk) += clk/clk_div.c
src-$(APP_libmcu_x2580_clk) += clk/clk_gate.c
src-$(APP_libmcu_x2580_clk) += clk/clk_i2s.c
src-$(APP_libmcu_x2580_clk) += clk/clk_list.c

src-$(APP_libmcu_x2580_gpio) += gpio/gpio.c
src-$(APP_libmcu_x2580_uart) += uart/uart.c
src-$(APP_libmcu_x2580_systick) += systick/systick.c
src-$(APP_libmcu_x2580_i2c) += i2c/i2c.c

package_targets += soc/x2580/x2580.lds.in

soc/x2580/x2580.lds.in:
	$(Q)echo $(MSG_LDS) $@
	$(Q)echo "sram_start = $(APP_libmcu_x2580_mem_start);" > $@
	$(Q)echo "sram_size = $(APP_libmcu_x2580_mem_size);" >> $@

.PHONY: soc/x2580/x2580.lds.in
