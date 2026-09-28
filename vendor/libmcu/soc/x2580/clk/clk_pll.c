#include <stdio.h>
#include <driver/clk.h>
#include <soc/base.h>
#include <cpu/io.h>
#include <bit_field2.h>
#include <bits_opt.h>

#define CPPCR   io_addr(CPM_IOBASE+0x0c)
#define CPAPCR  io_addr(CPM_IOBASE+0x10)
#define CPMPCR  io_addr(CPM_IOBASE+0x14)
#define CPVPCR  io_addr(CPM_IOBASE+0xe0)

#define APLL_BP 30
#define MPLL_BP 28
#define VPLL_BP 26

#define PLLM   20,28
#define PLLN   14,19
#define PLLOD  11,13
#define PLLOD1 7,10
#define PLLRG  4,6
#define PLL_ON 3,3
#define LOCK   2,2
#define PLLEN  0,0

static const char bypass[3] = {APLL_BP, MPLL_BP, VPLL_BP};
static volatile unsigned long *reg_addr[3] = {CPAPCR, CPMPCR, CPVPCR};

unsigned int clk_pll_get_rate(enum clk_pll_type id)
{
    int bypass_bit = bypass[id];
    if (test_bit(*CPPCR, bypass_bit))
        return CLK_EXT_RATE;

    unsigned long cpxpcr = *reg_addr[id];
    unsigned int pllm = get_bit_field(cpxpcr, PLLM);
    unsigned int plln = get_bit_field(cpxpcr, PLLN);
    unsigned int pllod = get_bit_field(cpxpcr, PLLOD);
    unsigned int pllod1 = get_bit_field(cpxpcr, PLLOD1);
    unsigned int freq = (CLK_EXT_RATE/1000);

    // static const char od[] = {1, 2, 4, 8, 16, 32, 64};
    // pllod = od[pllod];
    // 换一种写法
    pllod = 1 << pllod;

    return freq*(pllm+1)*2/((plln+1)*pllod*(pllod1+1))*1000;
}

int clk_pll_is_enable(enum clk_pll_type id)
{
    int bypass_bit = bypass[id];
    if (test_bit(*CPPCR, bypass_bit))
        return 1;

    unsigned long cpxpcr = *reg_addr[id];
    return !!get_bit_field(cpxpcr, PLL_ON);
}
