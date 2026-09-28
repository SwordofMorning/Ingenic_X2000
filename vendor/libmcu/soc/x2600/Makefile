root.COMMON_FLAGS += -march=rv32imc -mabi=ilp32 # -long-calls
root.COMMON_FLAGS += -fno-pic -static -Wall -Wno-abi  -std=gnu99 -fsigned-char
root.COMMON_FLAGS += -Isoc/x2600/include/

root.CXXFLAGS += -std=c++11 -D_GNU_SOURCE
root.CXXFLAGS += -fno-exceptions -fno-unwind-tables -fno-threadsafe-statics

ifeq ($(APP_libmcu_enable_jtag_debug), y)
root.COMMON_FLAGS += -g -ggdb -DDEBUG -fomit-frame-pointer
endif

root.LDFLAGS += -static
ifndef APP_libmcu_x2600_run_in_tcsm
root.TARGET_LDFLAGS += -T soc/x2600/x2600.lds
else
root.TARGET_LDFLAGS += -T soc/x2600/x2600_in_tcsm.lds
endif
root.TARGET_LDFLAGS += -static -Bstatic -m elf32lriscv
CROSS_COMPILE = riscv32-ingenicv0-elf-

LIBC_PATH = $(dir $(shell $(CC) -print-file-name=libc.a))
LIBGCC_PATH = $(dir $(shell $(CC) -print-file-name=libgcc.a))

root.TARGET_LDFLAGS += -L $(LIBC_PATH) -L $(LIBGCC_PATH)

root.TARGET_LIBS +=  -lm -lc -lgcc

ifndef APP_libmcu_x2600_run_in_tcsm
src-y += init/start.S
else
src-y += init/start_in_tcsm.S
endif
src-y += init/bzero.S
src-y += init/init.c
src-y += init/cpu.c
src-y += init/host_cpu.c
src-y += init/uncache_mem.c
src-$(APP_libmcu_x2600_irq) += init/irq.c
src-$(APP_libmcu_x2600_irq) += init/intc.c
src-$(APP_libmcu_tcsm_text_section) += init/tcsm.c

src-$(APP_libmcu_x2600_clk) += clk/clk.c
src-$(APP_libmcu_x2600_clk) += clk/clk_pll.c
src-$(APP_libmcu_x2600_clk) += clk/clk_cpccr.c
src-$(APP_libmcu_x2600_clk) += clk/clk_div.c
src-$(APP_libmcu_x2600_clk) += clk/clk_gate.c
src-$(APP_libmcu_x2600_clk) += clk/clk_i2s.c
src-$(APP_libmcu_x2600_clk) += clk/clk_list.c

src-$(APP_libmcu_x2600_gpio) += gpio/gpio.c
src-$(APP_libmcu_x2600_uart) += uart/uart.c
src-$(APP_libmcu_x2600_tcu) += tcu/tcu.c
src-$(APP_libmcu_x2600_systick) += systick/systick.c

src-$(APP_libmcu_x2600_adc) += adc/adc.c

src-$(APP_libmcu_x2600_fb) += fb/fb.c
src-$(APP_libmcu_x2600_fb) += fb/dpu_hal.c
src-$(APP_libmcu_x2600_fb) += fb/fb_layer_mixer.c

CFLAGS += -Isoc/x2600/felix/ -Isoc/x2600/felix/libh264 -Isoc/x2600/felix/libh264/include
src-$(APP_libmcu_x2600_felix) +=  felix/libh264/api/jzm_h264_dec.c 	\
			felix/libh264/src/buffer.c	 	\
			felix/libh264/src/golomb.c 		\
			felix/libh264/src/h2645_parse.c 	\
			felix/libh264/src/h264_direct.c 	\
			felix/libh264/src/h264_parse.c 		\
			felix/libh264/src/h264_picture.c 	\
			felix/libh264/src/h264_ps.c 		\
			felix/libh264/src/h264_refs.c 		\
			felix/libh264/src/h264_sei.c 		\
			felix/libh264/src/h264_slice.c 		\
			felix/libh264/src/h264data.c 		\
			felix/libh264/src/h264dec.c 		\
			felix/libh264/src/log.c 		\
			felix/libh264/src/mathtables.c 		\
			felix/libh264/src/mem.c 		\
			felix/libh264/src/vpu_ops.c		\
			felix/felix_h264_decoder.c		\
			felix/felix_drv.c

src-$(APP_libmcu_x2600_dma) += dma/dma.c
src-$(APP_libmcu_x2600_pwm) += pwm/pwm.c
src-$(APP_libmcu_x2600_spi) += spi/spi.c

src-$(APP_libmcu_x2600_tpc) += tpc/tpc_hal.c
src-$(APP_libmcu_x2600_tpc) += tpc/tpc_gpio.c
src-$(APP_libmcu_x2600_tpc) += tpc/tpc.c
src-$(APP_libmcu_x2600_tpc) += tpc/tpc_dma.c

src-$(APP_libmcu_x2600_nemc) += nemc/nemc.c
src-$(APP_libmcu_x2600_nemc) += nemc/nemc_gpio.c

src-$(APP_libmcu_x2600_i2c) += i2c/i2c.c

ifndef APP_libmcu_x2600_run_in_tcsm
LDS_IN_FILE_NAME = soc/x2600/x2600.lds.in
else
LDS_IN_FILE_NAME = soc/x2600/x2600_in_tcsm.lds.in
endif

package_targets += $(LDS_IN_FILE_NAME)

$(LDS_IN_FILE_NAME):
	$(Q)echo $(MSG_LDS) $@
	$(Q)echo "sram_start = $(APP_libmcu_x2600_mem_start);" > $@
	$(Q)echo "sram_size = $(APP_libmcu_x2600_mem_size);" >> $@

.PHONY: $(LDS_IN_FILE_NAME)
