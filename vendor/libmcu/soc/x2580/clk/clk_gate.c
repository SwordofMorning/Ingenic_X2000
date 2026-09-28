#include <stdio.h>
#include <driver/clk.h>
#include <soc/base.h>
#include <cpu/io.h>
#include <bit_field2.h>
#include <bits_opt.h>

#define CLKGR0 io_addr(CPM_IOBASE+0x20)
#define CLKGR1 io_addr(CPM_IOBASE+0x28)

void clk_gate_enable(enum clk_gate_type id)
{
    volatile unsigned long *CLKGR = CLKGR0;

    if (id >= CLK_GATE_1) {
        CLKGR = CLKGR1;
        id -= CLK_GATE_1;
    }

    clear_bits(*CLKGR, BIT(id));
}

void clk_gate_disable(enum clk_gate_type id)
{
    volatile unsigned long *CLKGR = CLKGR0;

    if (id >= CLK_GATE_1) {
        CLKGR = CLKGR1;
        id -= CLK_GATE_1;
    }

    set_bits(*CLKGR, BIT(id));
}

int clk_gate_is_enable(enum clk_gate_type id)
{
    volatile unsigned long *CLKGR = CLKGR0;

    if (id >= CLK_GATE_1) {
        CLKGR = CLKGR1;
        id -= CLK_GATE_1;
    }

    return !test_bit(*CLKGR, id);
}
