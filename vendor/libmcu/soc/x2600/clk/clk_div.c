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
#define MSC_EXCLK_EN 21, 21

#define EXCLK_EN 20, 20

#define DDCDR    0x002C
#define MACCDR   0x0054
#define LPCDR    0x0064
#define MSC0CDR  0x0068
#define MSC1CDR  0x00A4
#define SFCCDR   0x0074
#define SSICDR   0x005C
#define PWMCDR   0x006C
#define TPCCDR   0x007C
#define CIMCDR   0x0078
#define G2DCDR   0x0030
#define CAN0CDR  0x00A0
#define CAN1CDR  0x00A8
#define SADCCDR  0x00AC

struct type_data {
    unsigned char reg;
    unsigned char cs_type;
    unsigned char div_end;
};

static struct type_data datas[] = {
    [CLK_DIV_DDR] = {DDCDR, 0, 3},
    [CLK_DIV_MAC] = {MACCDR, 1, 7},
    [CLK_DIV_LPC] = {LPCDR, 1, 7},
    [CLK_DIV_MSC0] = {MSC0CDR, 3, 7},
    [CLK_DIV_MSC1] = {MSC1CDR, 3, 7},
    [CLK_DIV_SFC] = {SFCCDR, 1, 7},
    [CLK_DIV_SSI] = {SSICDR, 1, 7},
    [CLK_DIV_PWM] = {PWMCDR, 1, 3},
    [CLK_DIV_TPC] = {TPCCDR, 1, 3},
    [CLK_DIV_CIM] = {CIMCDR, 1, 7},
    [CLK_DIV_G2D] = {G2DCDR, 1, 3},
    [CLK_DIV_CAN0] = {CAN0CDR, 2, 7},
    [CLK_DIV_CAN1] = {CAN1CDR, 2, 7},
    [CLK_DIV_SADC] = {SADCCDR, 2, 7},
};

static unsigned char parent_type[4][4] = {
 {CLK_DIV_PARENT_NULL, CLK_DIV_PARENT_SCLK_A, CLK_DIV_PARENT_MPLL, CLK_DIV_PARENT_NULL},
 {CLK_DIV_PARENT_SCLK_A, CLK_DIV_PARENT_MPLL, CLK_DIV_PARENT_EPLL, CLK_DIV_PARENT_NULL},
 {CLK_DIV_PARENT_SCLK_A, CLK_DIV_PARENT_MPLL, CLK_DIV_PARENT_EPLL, CLK_DIV_PARENT_EXT_CLK},
 {CLK_DIV_PARENT_SCLK_A, CLK_DIV_PARENT_MPLL, CLK_DIV_PARENT_EXT_CLK, CLK_DIV_PARENT_NULL},
};

static unsigned int get_parent_rate(enum clk_div_type id, int cs, int *is_ext)
{
    unsigned char parent = parent_type[datas[id].cs_type][cs];
    if (parent == CLK_DIV_PARENT_SCLK_A)
        return clk_cpccr_get_rate(CLK_CPCCR_SCLK_A);
    if (parent == CLK_DIV_PARENT_MPLL)
        return clk_pll_get_rate(CLK_PLL_MPLL);
    if (parent == CLK_DIV_PARENT_EPLL)
        return clk_pll_get_rate(CLK_PLL_EPLL);
    if (parent == CLK_DIV_PARENT_EXT_CLK) {
        *is_ext = 1;
        return CLK_EXT_RATE;
    }

    return 0;
}

static unsigned char to_cs(enum clk_div_type id, enum clk_div_parent_type parent)
{
    unsigned char *p = parent_type[datas[id].cs_type];

    if (p[0] == parent) return 0;
    if (p[1] == parent) return 1;
    if (p[2] == parent) return 2;

    return 3;
}

unsigned int clk_div_get_rate(enum clk_div_type id)
{
    volatile unsigned long *XCDR = io_addr(CPM_IOBASE+datas[id].reg);
    unsigned long cdr = *XCDR;
    unsigned int cs = get_bit_field(cdr, CS);
    unsigned int div_end = datas[id].div_end;
    unsigned int div = get_bit_field(cdr, 0, div_end) + 1;
    int is_ext = 0;

    int rate = get_parent_rate(id, cs, &is_ext);
    if (is_ext)
        return rate;
    if (id == CLK_DIV_MSC0 || id == CLK_DIV_MSC1)
        rate /= 4;
    return rate / div;
}

void clk_div_set_rate(enum clk_div_type id, unsigned int rate)
{
    volatile unsigned long *XCDR = io_addr(CPM_IOBASE+datas[id].reg);
    unsigned long cdr = *XCDR;
    unsigned int cs = get_bit_field(cdr, CS);
    unsigned int div_end = datas[id].div_end;
    unsigned int div_max = 1 << (div_end+1);
    int is_ext = 0;

    unsigned int parent_rate = get_parent_rate(id, cs, &is_ext);
    if (is_ext)
        return;

    if (id == CLK_DIV_MSC0 || id == CLK_DIV_MSC1)
        parent_rate /= 4;

    unsigned int div = (parent_rate + rate - 1) / rate;

    if (div == 0)
        div = 1;
    if (div > div_max)
        div = div_max;

    cdr = set_bit_field(cdr, CE, 1);
    cdr = set_bit_field(cdr, 0, div_end, div-1);
    *XCDR = cdr;

    int count = 0;
    while (get_bit_field_v(XCDR, BUSY)) {
        count++;
    }

    // printf("count: %d %d\n", count, parent_rate);
}

void clk_div_set_parent(enum clk_div_type id, enum clk_div_parent_type parent)
{
    volatile unsigned long *XCDR = io_addr(CPM_IOBASE+datas[id].reg);
    int cs = to_cs(id, parent);
    unsigned long cdr = *XCDR;

    cdr = set_bit_field(cdr, CE, 1);
    cdr = set_bit_field(cdr, CS, cs);

    if (parent == CLK_DIV_PARENT_EXT_CLK) {
        if (id == CLK_DIV_MSC0 || id == CLK_DIV_MSC1)
            set_bit_field(cdr, MSC_EXCLK_EN, 1);
        else
            set_bit_field(cdr, EXCLK_EN, 1);
    }

    *XCDR = cdr;

    int count = 0;
    while (get_bit_field_v(XCDR, BUSY)) {
        count++;
    }
}

static void clk_div_do_enable(enum clk_div_type id, int enable)
{
    volatile unsigned long *XCDR = io_addr(CPM_IOBASE+datas[id].reg);
    unsigned long cdr = *XCDR;

    cdr = set_bit_field(cdr, CE, 1);
    cdr = set_bit_field(cdr, STOP, !enable);
    *XCDR = cdr;

    int count = 0;
    while (get_bit_field_v(XCDR, BUSY)) {
        count++;
    }
}

void clk_div_enable(enum clk_div_type id)
{
    clk_div_do_enable(id, 1);
}

void clk_div_disable(enum clk_div_type id)
{
    clk_div_do_enable(id, 0);
}

int clk_div_is_enable(enum clk_div_type id)
{
    volatile unsigned long *XCDR = io_addr(CPM_IOBASE+datas[id].reg);
    unsigned long cdr = *XCDR;
    return !!get_bit_field(cdr, STOP);
}
