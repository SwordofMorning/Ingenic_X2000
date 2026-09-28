#include <stdio.h>
#include <driver/clk.h>
#include <soc/base.h>
#include <cpu/io.h>
#include <bit_field2.h>
#include <bits_opt.h>
#include <errno.h>
#include <common.h>

#define CPPCR   io_addr(CPM_IOBASE+0x0c)
#define CPAPCR  io_addr(CPM_IOBASE+0x10)
#define CPMPCR  io_addr(CPM_IOBASE+0x14)
#define CPEPCR  io_addr(CPM_IOBASE+0x18)

#define APLL_BP 30
#define MPLL_BP 28
#define EPLL_BP 26

#define PLLM      20,31
#define PLLN      14,19
#define PLLOD1    11,13
#define PLLOD     8,10
#define DSMPD     7, 7
#define PHASEPD   6, 6
#define POSTDIVPD 5, 5
#define VCOPD     4, 4
#define PLL_ON    3,3
#define LOCK      2,2
#define PLLEN     0,0

static const char bypass[3] = {APLL_BP, MPLL_BP, EPLL_BP};
static volatile unsigned long *reg_addr[3] = {CPAPCR, CPMPCR, CPEPCR};

#define PLL_RATE(_rate, _m, _n, _od1, _od0) \
{ \
    .rate = _rate, \
    .m = _m, \
    .n = _n, \
    .od1 = _od1, \
    .od0 = _od0, \
}

struct ingenic_pll_rate_table {
    unsigned int rate;
    unsigned short m;
    unsigned char n;
    unsigned char od1;
    unsigned char od0;
};

#define PLL_DESC(_regoff, _m, _m_w, _n, _n_w, _od1, _od1_w, _od0, _od0_w, _on, _en) \
{ \
    .regoff = (unsigned long)_regoff, \
    .m_sft = _m, \
    .m_width = _m_w, \
    .n_sft = _n, \
    .n_width = _n_w, \
    .od1_sft = _od1, \
    .od1_width = _od1_w, \
    .od0_sft = _od0, \
    .od0_width = _od0_w, \
    .on_bit = _on, \
    .en_bit =  _en, \
}

struct ingenic_pll_hwdesc {
    unsigned long regoff;
    unsigned char m_sft;
    unsigned char m_width;
    unsigned char n_sft;
    unsigned char n_width;
    unsigned char od1_sft;
    unsigned char od1_width;
    unsigned char od0_sft;
    unsigned char od0_width;
    unsigned char on_bit;
    unsigned char en_bit;
};

#define PLL(_id, _name, _parent_name, _hwdesc, _rtable) \
{ \
    .id = _id, \
    .dev_name = _name, \
    .name = _name, \
    .parent_name = _parent_name, \
    .hwdesc = _hwdesc, \
    .rate_table = _rtable, \
}

static struct ingenic_pll_rate_table x2600_pll_rate_table[] = {
    PLL_RATE(1800000000, 75, 1, 1, 1),
    PLL_RATE(1600000000, 200, 3, 1, 1),
    PLL_RATE(1500000000, 125, 2, 1, 1),
    PLL_RATE(1404000000, 117, 2, 1, 1),
    PLL_RATE(1400000000, 175, 3, 1, 1),
    PLL_RATE(1392000000, 58, 1, 1, 1),
    PLL_RATE(1296000000, 54, 1, 1, 1),
    PLL_RATE(1200000000, 50, 1, 1, 1),
    PLL_RATE(1176000000, 49, 1, 1, 1),
    PLL_RATE(1148000000, 287, 3, 1, 2),
    PLL_RATE(1134000000, 189, 2, 1, 2),
    PLL_RATE(1120000000, 140, 3, 1, 1),
    PLL_RATE(1092000000, 91, 1, 1, 2),
    PLL_RATE(1064000000, 133, 3, 1, 1),
    PLL_RATE(1050000000, 175, 2, 1, 2),
    PLL_RATE(1036000000, 259, 3, 1, 2),
    PLL_RATE(1008000000, 42, 1, 1, 1),
    PLL_RATE(1000000000, 125, 3, 1, 1),
    PLL_RATE(980000000, 245, 3, 1, 2),
    PLL_RATE(966000000, 161, 2, 1, 2),
    PLL_RATE(952000000, 119, 3, 1, 1),
    PLL_RATE(924000000, 77, 1, 1, 2),
    PLL_RATE(900000000, 75, 1, 1, 2),
    PLL_RATE(896000000, 112, 3, 1, 1),
    PLL_RATE(882000000, 147, 2, 1, 2),
    PLL_RATE(868000000, 217, 3, 1, 2),
    PLL_RATE(840000000, 35, 1, 1, 1),
    PLL_RATE(812000000, 203, 3, 1, 2),
    PLL_RATE(800000000, 100, 1, 1, 3),
    PLL_RATE(798000000, 133, 2, 1, 2),
    PLL_RATE(784000000, 98, 1, 1, 3),
    PLL_RATE(756000000, 63, 1, 1, 2),
    PLL_RATE(728000000, 91, 1, 1, 3),
    PLL_RATE(714000000, 119, 2, 1, 2),
    PLL_RATE(700000000, 175, 2, 1, 3),
    PLL_RATE(693000000, 231, 4, 1, 2),
    PLL_RATE(672000000, 28, 1, 1, 1),
    PLL_RATE(672000000, 28, 1, 1, 1),
    PLL_RATE(651000000, 217, 4, 1, 2),
    PLL_RATE(630000000, 105, 2, 1, 2),
    PLL_RATE(609000000, 203, 4, 1, 2),
    PLL_RATE(588000000, 49, 1, 1, 2),
    PLL_RATE(567000000, 189, 2, 1, 4),
    PLL_RATE(546000000, 91, 1, 1, 4),
    PLL_RATE(532000000, 133, 2, 1, 3),
    PLL_RATE(525000000, 175, 2, 1, 4),
    PLL_RATE(518000000, 259, 3, 1, 4),
    PLL_RATE(504000000, 42, 1, 1, 2),
    PLL_RATE(490000000, 245, 3, 1, 4),
    PLL_RATE(483000000, 161, 2, 1, 4),
    PLL_RATE(476000000, 119, 2, 1, 3),
    PLL_RATE(462000000, 77, 1, 1, 4),
    PLL_RATE(448000000, 56, 1, 1, 3),
    PLL_RATE(441000000, 147, 2, 1, 4),
    PLL_RATE(434000000, 217, 3, 1, 4),
    PLL_RATE(420000000, 35, 1, 1, 2),
};

// PLL HWDESC
static struct ingenic_pll_hwdesc x2600_pll_hwdescs[] = {
    PLL_DESC(CPAPCR, 20,12, 14, 6, 11, 3, 8, 3, 3, 0),// apll
    PLL_DESC(CPMPCR, 20,12, 14, 6, 11, 3, 8, 3, 3, 0),// mpll
    PLL_DESC(CPEPCR, 20,12, 14, 6, 11, 3, 8, 3, 3, 0),// epll
};

unsigned int clk_pll_get_rate(enum clk_pll_type id)
{
    int bypass_bit = bypass[id];
    if (test_bit(*CPPCR, bypass_bit))
        return CLK_EXT_RATE;

    unsigned long cpxpcr = *reg_addr[id];
    if (!get_bit_field(cpxpcr, PLL_ON))
        return 0;

    unsigned int pllm = get_bit_field(cpxpcr, PLLM);
    unsigned int plln = get_bit_field(cpxpcr, PLLN);
    unsigned int pllod = get_bit_field(cpxpcr, PLLOD);
    unsigned int pllod1 = get_bit_field(cpxpcr, PLLOD1);
    unsigned int freq = (CLK_EXT_RATE/1000);

    // printf("cpxpcr:%08x m=%d n=%d d=%d d1=%d freq=%d\n",
    //      cpxpcr, pllm, plln, pllod, pllod1, freq);

    return freq*pllm/(plln*pllod*pllod1)*1000;
}

static int wait_pll_stable(unsigned long reg, unsigned int shift)
{
    unsigned int timeout = 0xffff;

    while ((!((*(volatile unsigned long *)reg >> shift) & 1)) && --timeout);
    if (!timeout) {
        printf("WARNING : why cannot wait pll stable ???\n");
        return -ETIMEDOUT;
    } else {
        return 0;
    }
}

unsigned int clk_pll_set_rate(enum clk_pll_type id, unsigned int rate)
{
    unsigned int m = 0, n = 0, od0 = 0, od1 = 0;
    unsigned int val, i;

    for (i = 0; i < ARRAY_SIZE(x2600_pll_rate_table); i++) {
        if (rate == x2600_pll_rate_table[i].rate) {
            m = x2600_pll_rate_table[i].m;
            n = x2600_pll_rate_table[i].n;
            od0 = x2600_pll_rate_table[i].od0;
            od1 = x2600_pll_rate_table[i].od1;
        }
    }

    struct ingenic_pll_hwdesc *hwdesc = &x2600_pll_hwdescs[id];
    *(volatile unsigned long *)hwdesc->regoff = 0;
    val = (m << hwdesc->m_sft) | (n << hwdesc->n_sft) | (od0 << hwdesc->od0_sft) | (od1 << hwdesc->od1_sft) | 1;
    *(volatile unsigned long *)hwdesc->regoff = val;

    wait_pll_stable(hwdesc->regoff, hwdesc->on_bit);

    return 0;
}

int clk_pll_is_enable(enum clk_pll_type id)
{
    int bypass_bit = bypass[id];
    if (test_bit(*CPPCR, bypass_bit))
        return 1;

    unsigned long cpxpcr = *reg_addr[id];
    return !!get_bit_field(cpxpcr, PLL_ON);
}
