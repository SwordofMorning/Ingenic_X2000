#include <stdio.h>
#include <asm/cacheops.h>
#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/clk.h>
#include <bit_field2.h>

enum {
    SCLK_A = 1,
    APLL,
    MPLL,
    EXCLK,
    H2CLK,

    SELECT_TYPES
};

#define NO_USE 255

struct cpccr_reg {
    unsigned char sel_src[3];
    unsigned char sel;
    unsigned char ce;
    unsigned char div;
};

#define GATE_SCLKA 23

#define index(id) ((id) - CLK_ID_SCLK_A)

static const struct cpccr_reg regs[] = {
    [index(CLK_ID_CCLK)]   = {{CLK_ID_EXT, CLK_ID_SCLK_A, CLK_ID_MPLL}, 28, 22, 0},
    [index(CLK_ID_L2CLK)]  = {{CLK_ID_EXT, CLK_ID_SCLK_A, CLK_ID_MPLL}, 28, 22, 4},
    [index(CLK_ID_H0CLK)]  = {{CLK_ID_EXT, CLK_ID_SCLK_A, CLK_ID_MPLL}, 26, 21, 8},
    [index(CLK_ID_H2CLK)]  = {{CLK_ID_EXT, CLK_ID_SCLK_A, CLK_ID_MPLL}, 24, 20, 12},
    [index(CLK_ID_PCLK)]   = {{CLK_ID_EXT, CLK_ID_SCLK_A, CLK_ID_MPLL}, 24, 20, 16},
    [index(CLK_ID_SCLK_A)] = {{CLK_ID_EXT, CLK_ID_EXT1,   CLK_ID_APLL}, 30, NO_USE, NO_USE},
};

unsigned long sclk_a_get_rate(void)
{
    unsigned long rate;
    unsigned long cpccr = cpm_inl(CPM_CPCCR);
    int n = get_bit_field(cpccr, 30, 30 + 1);

    switch(n) {
    case 0:
        rate = 0;
        break;
    case 1:
        rate = ext_pll_get_rate(CLK_ID_EXT1);
        break;
    case 2:
        rate = ext_pll_get_rate(CLK_ID_APLL);
        break;
    default:
        rate = 0;
        printf("sclk_a_get_rate error!\n");
        break;
    }

    return rate;
}


unsigned long cpccr_get_rate(int clk_id)
{
    int id = index(clk_id);
    unsigned long cpccr = cpm_inl(CPM_CPCCR);
    int sel = regs[id].sel;
    int n = get_bit_field(cpccr, sel, sel + 1);
    unsigned int rate = ext_pll_get_rate(regs[id].sel_src[n]);
    unsigned int div = regs[id].div;

    if (regs[id].sel_src[n] == CLK_ID_SCLK_A)
        rate = sclk_a_get_rate();

    if (div == NO_USE)
        return rate;

    div = get_bit_field(cpccr, div, div + 3);

    return rate / (div + 1);
}

