#include <stdio.h>
#include <driver/clk.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <bit_field2.h>
#include <bits_opt.h>
#include <soc/clk.h>
#include <driver/systick.h>

static const int pll_no_tab[] = {
    [1] = 2,
    [2] = 4,
    [3] = 8,
    [4] = 16,
    [5] = 32,
    [6] = 64,
};

unsigned long pll_get_rate(int reg)
{
    unsigned long value = cpm_inl(reg);

    if (!(value & BIT(XPCR_PLLEN)))
        return 0;

    unsigned int fd = get_bit_field(value, XPCR_PLLFD);
    unsigned int rd = get_bit_field(value, XPCR_PLLRD);
    unsigned int od = get_bit_field(value, XPCR_PLLOD);

    unsigned int nf = fd + 1;
    unsigned int nr = rd + 1;
    unsigned int no = pll_no_tab[od];
    unsigned int rate_in = 24 * 1000 * 1000 / 1000;
    unsigned int rate = rate_in * 2 * nf / (nr * no);

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
    case CLK_ID_OTGPHY:
        rate = 48 * 1000 * 1000;
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