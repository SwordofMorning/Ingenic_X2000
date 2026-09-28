#include <stdio.h>
#include <driver/clk.h>
#include <soc/base.h>
#include <cpu/io.h>
#include <bit_field2.h>
#include <bits_opt.h>

#define I2STCDR  io_addr(CPM_IOBASE+0x70)
#define I2STCDR1 io_addr(CPM_IOBASE+0x78)
#define I2SRCDR  io_addr(CPM_IOBASE+0x84)
#define I2SRCDR1 io_addr(CPM_IOBASE+0x88)

#define CS 30, 31
#define CE 29, 29
#define DIV_M 20, 28
#define DIV_N 0, 19

volatile unsigned long *reg_addr[] = {
    I2STCDR, I2SRCDR};

unsigned int clk_i2s_get_rate(enum clk_i2s_type id)
{
    unsigned long cdr = *reg_addr[id];
    unsigned int cs = get_bit_field(cdr, CS);
    unsigned int div_m = get_bit_field(cdr, DIV_M);
    unsigned int div_n = get_bit_field(cdr, DIV_N);
    unsigned int parent_rate = 0;

    if (cs == 0)
        parent_rate = clk_cpccr_get_rate(CLK_CPCCR_SCLK_A);
    if (cs == 1)
        parent_rate = clk_pll_get_rate(CLK_PLL_MPLL);
    if (cs == 2)
        parent_rate = clk_pll_get_rate(CLK_PLL_VPLL);

    return ((unsigned long long)parent_rate * div_m / div_n);
}

int clk_i2s_is_enable(enum clk_i2s_type id)
{
    unsigned long cdr = *reg_addr[id];
    return !!get_bit_field(cdr, CE);
}
