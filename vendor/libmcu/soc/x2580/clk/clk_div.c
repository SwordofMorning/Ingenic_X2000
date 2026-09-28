#include <stdio.h>
#include <driver/clk.h>
#include <soc/base.h>
#include <cpu/io.h>
#include <bit_field2.h>
#include <bits_opt.h>

// ddr
#define GATE_EN 26, 26
#define CHANGE_EN 25, 25
#define FLAG 24, 24

#define CS 30, 31
#define CE 29, 29
#define BUSY 28, 28
#define STOP 27, 27
#define CDR 0, 3

#define SFC0_ORI_SEL 8, 9

// msc0 1 共用
#define EXCLK_EN 21, 21

#define DDRCDR      0x2C
#define RSACDR      0x4C
#define SSI_SLVCDR  0x50
#define MACCDR      0x54
#define SFC0CDR     0x60
#define LPCDR       0x64
#define MSC0CDR     0x68
#define MSC1CDR     0x6C
#define SSICDR      0x74
#define SFC1CDR     0x7C
#define ISPMCDR     0x80
#define CIM0CDR     0x90
#define LDCCDR      0xA0
#define ISPSCDR     0xA8
#define ISPACDR     0xAC
#define BT0CDR      0xB8
#define ALGENCCDR   0xBC
#define PWMCDR      0xFC

static int is_4bit = 
    BIT(CLK_DIV_DDR) | BIT(CLK_DIV_RSA) | BIT(CLK_DIV_ISPM) |
    BIT(CLK_DIV_ISPS) | BIT(CLK_DIV_ISPA) | BIT(CLK_DIV_ALGENC);

unsigned char regs[] = {
    DDRCDR, RSACDR, SSI_SLVCDR, MACCDR, SFC0CDR, LPCDR,
    MSC0CDR, MSC1CDR, SSICDR, SFC1CDR, ISPMCDR, CIM0CDR,
    LDCCDR, ISPSCDR, ISPACDR, BT0CDR, ALGENCCDR, PWMCDR};

unsigned int clk_div_get_rate(enum clk_div_type id)
{
    volatile unsigned long *XCDR = io_addr(CPM_IOBASE+regs[id]);
    unsigned long cdr = *XCDR;
    unsigned int cs = get_bit_field(cdr, CS);
    unsigned int div_end = is_4bit & BIT(id) ? 3 : 7;
    unsigned int div = get_bit_field(cdr, 0, div_end) + 1;

    if (id == CLK_DIV_DDR) {
        if (cs == 0)
            return 0;
        if (cs == 1)
            return clk_cpccr_get_rate(CLK_CPCCR_SCLK_A) / div;
        return clk_pll_get_rate(CLK_PLL_MPLL) / div;
    } else {
        if (cs == 0)
            return clk_cpccr_get_rate(CLK_CPCCR_SCLK_A) / div;
        if (cs == 1)
            return clk_pll_get_rate(CLK_PLL_MPLL) / div;
        return clk_pll_get_rate(CLK_PLL_VPLL) / div;
    }
}

int clk_div_is_enable(enum clk_div_type id)
{
    volatile unsigned long *XCDR = io_addr(CPM_IOBASE+regs[id]);
    unsigned long cdr = *XCDR;
    return !!get_bit_field(cdr, STOP);
}
