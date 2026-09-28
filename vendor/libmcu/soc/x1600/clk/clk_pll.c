#include <stdio.h>
#include <driver/clk.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <bit_field2.h>
#include <bits_opt.h>
#include <soc/clk.h>
#include <driver/systick.h>

struct pll_rate_setting {
    unsigned long rate;
    int m,n,od,rng;
};

unsigned long pll_get_rate(int reg)
{
    unsigned long cpxpcr = cpm_inl(reg);
    unsigned long m, n, od0, od1;
    unsigned long rate;

    if (cpxpcr & 1) {
        m = ((cpxpcr >> 20) & 0xFFF);
        n = ((cpxpcr >> 14) & 0x3f);
        od1 = ((cpxpcr >> 11) & 0x7);
        od0 = ((cpxpcr >> 8) & 0x7);

        unsigned long fbdiv = m;
        unsigned long refdiv = n;
        unsigned long postdiv2 = od0;
        unsigned long postdiv1 = od1;
        unsigned long fvco = (24 * 1000 * 1000 / 1000) * fbdiv / refdiv;
        rate = fvco / postdiv2 / postdiv1;
    }else return rate = 0;

    return rate * 1000;
}

unsigned long ext_pll_get_rate(int clk_id)
{
    unsigned long rate;

    switch (clk_id) {
    case CLK_ID_EXT0:
        rate = 32768;
        break;
    case CLK_ID_EXT1:
        rate = 24 * 1000 * 1000;
        break;
    case CLK_ID_EPLL:
        rate = pll_get_rate(CPM_CPEPCR);
        break;
    case CLK_ID_APLL:
        rate = pll_get_rate(CPM_CPAPCR);
        break;
    case CLK_ID_MPLL:
        rate = pll_get_rate(CPM_CPMPCR);
        break;
    case CLK_ID_EXT:
        rate = 0;
        break;
    default:
        rate = 0;
        printf("ext_pll_get_rate error!\n");
        break;
    }

    return rate;
}