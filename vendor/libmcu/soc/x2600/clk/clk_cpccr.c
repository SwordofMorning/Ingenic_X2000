#include <stdio.h>
#include <driver/clk.h>
#include <soc/base.h>
#include <cpu/io.h>
#include <bit_field2.h>
#include <bits_opt.h>

#define CPCCR io_addr(CPM_IOBASE+0x00)

#define SEL_SRC    30, 31
#define SEL_CPLL   28. // 28, 29
#define SEL_H0PLL  26. // 26, 27
#define SEL_H2PLL  24. // 24, 25

#define CDIV    0 // 0, 3
#define L2CDIV  4 // 4, 7
#define H0DIV   8 // 8, 11
#define H2DIV  12 // 12, 15
#define PDIV   16 // 16, 19

    // CLK_SCLK_A,
    // CLK_CCLK,
    // CLK_L2CLK,
    // CLK_H0CLK,
    // CLK_H2CLK,
    // CLK_PCLK,

static unsigned int get_sclk_a_rate(unsigned long cpccr)
{
    unsigned int sel = get_bit_field(cpccr, SEL_SRC);
    if (sel == 0)
        return 0;
    if (sel == 1)
        return CLK_EXT_RATE;
    return clk_pll_get_rate(CLK_PLL_APLL);
}

static const char divs[] = {CDIV, L2CDIV, H0DIV, H2DIV, PDIV};
static const char sels[] = {SEL_CPLL, SEL_CPLL, SEL_H0PLL, SEL_H2PLL, SEL_H2PLL};

unsigned int clk_cpccr_get_rate(enum clk_cpccr_type id)
{
    unsigned long cpccr = *CPCCR;
    if (id == CLK_CPCCR_SCLK_A)
        return get_sclk_a_rate(cpccr);
    
    unsigned int sel = get_bit_field(cpccr, sels[id], sels[id]+1);
    unsigned int div = get_bit_field(cpccr, divs[id], divs[id]+3);
    if (sel == 0)
        return 0;
    if (sel == 1)
        return get_sclk_a_rate(cpccr) / (div+1);
    return clk_pll_get_rate(CLK_PLL_MPLL) / (div+1);
}
