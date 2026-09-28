#include <stdio.h>
#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/clk.h>
#include <errno.h>
#include <bit_field2.h>

#define cgu_cs   30, 31
#define cgu_ce   29
#define cgu_busy 28
#define cgu_stop 27

struct cgu_clk {
    unsigned char src[4];
    unsigned char cdr_bits; // size of cdr
    unsigned char coe;      // 分频值div = (cdr + 1)*coe
    unsigned char reg;
    unsigned short save_div;
} __packed;

#define index(id) ((id) - CLK_ID_CGU_DDR)

static struct cgu_clk cgu_clks[] = {
    [index(CLK_ID_CGU_DDR)]    = { {CLK_ID_EXT,   CLK_ID_SCLK_A, CLK_ID_MPLL, CLK_ID_EXT},  4, 1, CPM_DDCDR},
    [index(CLK_ID_CGU_MACPHY)] = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT},  8, 1, CPM_MACPHYCDR},
    [index(CLK_ID_CGU_LCD)]    = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT},  8, 1, CPM_LPCDR},
    [index(CLK_ID_CGU_MSC0)]   = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT},  8, 2, CPM_MSC0CDR},
    [index(CLK_ID_CGU_MSC1)]   = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT},  8, 2, CPM_MSC1CDR},
    [index(CLK_ID_CGU_SFC)]    = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT},  8, 1, CPM_SFCCDR},
    [index(CLK_ID_CGU_SSI)]    = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT},  8, 1, CPM_SSICDR},
    [index(CLK_ID_CGU_CIM)]    = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT},  8, 1, CPM_CIMCDR},
    [index(CLK_ID_CGU_PWM)]    = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT},  4, 1, CPM_PWMCDR},
    [index(CLK_ID_CGU_CAN0)]   = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT1}, 8, 1, CPM_CAN0CDR},
    [index(CLK_ID_CGU_CAN1)]   = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT1}, 8, 1, CPM_CAN1CDR},
    [index(CLK_ID_CGU_CDBUS)]  = { {CLK_ID_SCLK_A, CLK_ID_MPLL,  CLK_ID_EPLL, CLK_ID_EXT1}, 8, 1, CPM_CDBUSCDR},
};

static inline void cpm_set_xcdr(unsigned long xcdr, int id)
{
    cpm_outl(xcdr, cgu_clks[id].reg);
}

static inline unsigned long cpm_get_xcdr(int id)
{
    return cpm_inl(cgu_clks[id].reg);
}

static inline void cpm_set_parent(int id, int parent)
{
    unsigned long xcdr = cpm_get_xcdr(id);

    /* 设置时钟源 */
    xcdr = set_bit_field(xcdr, cgu_cs, parent);
    cpm_set_xcdr(xcdr, id);
}

static inline void cpm_cgu_disable(unsigned long *xcdr, int id)
{
    *xcdr = set_bit_field(*xcdr, cgu_stop, cgu_stop, 1);
    *xcdr = set_bit_field(*xcdr, cgu_ce, cgu_ce, 1);
    cpm_set_xcdr(*xcdr, id);

    cpm_clear_bit(cgu_ce, cgu_clks[id].reg);
}

static inline void cpm_cgu_enable(unsigned long *xcdr, int id)
{
    *xcdr = set_bit_field(*xcdr, cgu_stop, cgu_stop, 0);
    *xcdr = set_bit_field(*xcdr, cgu_ce, cgu_ce, 1);
    cpm_set_xcdr(*xcdr, id);

    while(cpm_test_bit(cgu_busy, cgu_clks[id].reg))
        printf("wait stable.\n");

    cpm_clear_bit(cgu_ce, cgu_clks[id].reg);
}

static inline int cgu_is_enabled(unsigned long xcdr)
{
    return !get_bit_field(xcdr, cgu_stop, cgu_stop);
}

static inline int caculate_div(unsigned int rate, unsigned int parent_rate, int id)
{
    /* 通过parent_rate 和rate算出最合理的分频值 */
    int i;
    unsigned int max_div = 1 << cgu_clks[id].cdr_bits;

    parent_rate = parent_rate / cgu_clks[id].coe;
    for (i = 1; i <= max_div; i ++) {
        if (parent_rate/i <= rate)
            break;
    }

    if (i > max_div)
        i = max_div;

    return i * cgu_clks[id].coe;
}

static inline int cgu_get_div(unsigned long xcdr, int id)
{
    int div = get_bit_field(xcdr, 0, cgu_clks[id].cdr_bits);
    div = (div + 1) * cgu_clks[id].coe;

    return div;
}

static inline void cgu_set_div(unsigned long *xcdr, int div, int id)
{
    /* 如果分频值没有变化，就不用设置 */
    if (cgu_get_div(*xcdr, id) == div)
        return ;

    int cdr = (div / cgu_clks[id].coe) - 1;
    *xcdr = set_bit_field(*xcdr, 0, cgu_clks[id].cdr_bits, cdr);
    *xcdr = set_bit_field(*xcdr, cgu_ce, cgu_ce, 1);
    cpm_set_xcdr(*xcdr, id);

    while(cpm_test_bit(cgu_busy, cgu_clks[id].reg))
        printf("wait stable.\n");

    cpm_clear_bit(cgu_ce, cgu_clks[id].reg);
}

unsigned long cgu_get_rate(int clk_id)
{
    int id = index(clk_id);
    unsigned long xcdr = cpm_get_xcdr(id);
    int n = get_bit_field(xcdr, cgu_cs);
    unsigned int rate = ext_pll_get_rate(cgu_clks[id].src[n]);
    int div = cgu_get_div(xcdr, id);

    if (cgu_clks[id].src[n] == CLK_ID_SCLK_A)
        rate = sclk_a_get_rate();

    if (cgu_clks[id].src[n] == CLK_ID_EXT1)
        return rate;

    return rate / div;
}

int cgu_set_rate(int clk_id, unsigned int rate)
{
    int id = index(clk_id);
    unsigned long xcdr = cpm_get_xcdr(id);
    int n = get_bit_field(xcdr, cgu_cs);
    unsigned int parent_rate = ext_pll_get_rate(cgu_clks[id].src[n]);

    if (cgu_clks[id].src[n] == CLK_ID_SCLK_A)
        parent_rate = sclk_a_get_rate();

    /* 算出最合理的分频值 */
    int div = caculate_div(rate, parent_rate, id);

    /* 设置分频值 */
    int enable = cgu_is_enabled(xcdr);

    if (enable)
        cgu_set_div(&xcdr, div, id);
    else
        cgu_clks[id].save_div = div; /* 当前还没使能，就先把分频系数给保存下来 */

    return 0;
}

int cgu_enable(int clk_id, int on)
{
    int id = index(clk_id);
    unsigned long xcdr = cpm_get_xcdr(id);

    int prev_on = cgu_is_enabled(xcdr);
    if (prev_on == on)
        return 0;

    /* 失能时钟输出 */
    if (!on) {
        cpm_cgu_disable(&xcdr, id);
        return 0;
    }

    /* 使能时钟输出， 并设置分频值 */
    int div = cgu_clks[id].save_div;
    cgu_set_div(&xcdr, div, id);
    cpm_cgu_enable(&xcdr, id);

    return 0;
}

int cgu_set_parent(int clk_id, int parent_id)
{
    int i, clk_rate;
    int id = index(clk_id);

    for (i = 0; i < 4; i++) {
        if (cgu_clks[id].src[i] == parent_id)
            break;
    }
    if (i >= 4)
        return -EINVAL;

    /* 设置时钟源 */
    cpm_set_parent(id, i);

    /* 更新时钟频率 */
    clk_rate = cgu_get_rate(clk_id);
    cgu_set_rate(clk_id, clk_rate);

    return 0;
}
