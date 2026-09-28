root.COMMON_FLAGS += -mips32 -mabi=32 -EL
root.COMMON_FLAGS += -G 0  -fno-pic -static -nostdlib -Wall -msoft-float -mno-abicalls
root.COMMON_FLAGS += -Isoc/x1000/include/

root.LDFLAGS += -G 0 -static -n -EL -m elf32ltsmip
root.TARGET_LDFLAGS += -T soc/x1000/x1000.lds
root.TARGET_LDFLAGS += -G 0 -static -n -EL -m elf32ltsmip -Bstatic
CROSS_COMPILE = mips-linux-gnu-

root.TARGET_LIBS += soc/x1000/libgcc.a

src-y += init/start.S
src-y += init/init.c
src-y += init/host_cpu.c
src-y += init/irq.c
src-y += init/exception.S
src-y += intc/intc.c
src-y += gpio/gpio.c
src-y += gpio/gpio.c
src-y += uart/uart.c
src-y += clk/clk.c
src-y += clk/clk_gate.c
