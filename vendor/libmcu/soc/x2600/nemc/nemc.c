#include <stdio.h>
#include <string.h>
#include <soc/base.h>
#include <bit_field2.h>
#include <assert.h>
#include <driver/clk.h>
#include <driver/dma.h>
#include <driver/nemc.h>

#include "nemc_reg.h"

static int nemc_clk = CLK_GATE_NEMC;

static inline void nemc_write(unsigned int reg, unsigned int val)
{
    *NEMC_ADDR(reg) = val;
}

static inline int nemc_read(unsigned int reg)
{
    return *NEMC_ADDR(reg);
}

static void nemc_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    *NEMC_ADDR(reg) = set_bit_field(nemc_read(reg), start, end, val);
}

static void nemc_set_tch(int id, unsigned int tch)
{
    nemc_set_bit(SMC0R1 + id * 0x0004, TCH, tch);
}

static void nemc_set_taw(int id, unsigned int taw)
{
    nemc_set_bit(SMC0R1 + id * 0x0004, TAW, taw);
}

static void nemc_set_tbp(int id, unsigned int tbp)
{
    nemc_set_bit(SMC0R1 + id * 0x0004, TBP, tbp);
}

static void nemc_set_tras(int id, unsigned int tras)
{
    nemc_set_bit(SMC0R1 + id * 0x0004, TRAS, tras);
}

static void nemc_set_twas(int id, unsigned int twas)
{
    nemc_set_bit(SMC0R1 + id * 0x0004, TWAS, twas);
}

static void nemc_set_buswidth(int id, unsigned int bw)
{
    nemc_set_bit(SMC0R1 + id * 0x0004, BW, bw);
}

static void nemc_set_burstlen(int id, unsigned int bl)
{
    nemc_set_bit(SMC0R1 + id * 0x0004, BL, bl);
}

static void nemc_set_smt(int id, unsigned int smt)
{
    nemc_set_bit(SMC0R1 + id * 0x0004, SMT, smt);
}

static void nemc_set_strv(int id, unsigned int strv)
{
    nemc_set_bit(SMC1R1 + id * 0x0004, STRV, strv);
}

static void nemc_set_tavdh(int id, unsigned int tavdh)
{
    nemc_set_bit(SMC1R1 + id * 0x0004, TAVDH, tavdh);
}

static void nemc_set_tavdp(int id, unsigned int tavdp)
{
    nemc_set_bit(SMC1R1 + id * 0x0004, TAVDP, tavdp);
}

static void nemc_set_tavds(int id, unsigned int tavds)
{
    nemc_set_bit(SMC1R1 + id * 0x0004, TAVDS, tavds);
}

static void nemc_set_wait_times_double(int id, unsigned int is_double_timing)
{
    nemc_set_bit(SMC1R1 + id * 0x0004, TIMDOUBLE, is_double_timing);
}

static void nemc_wait_pin_enable(int id)
{
    nemc_set_bit(SMC1R1 + id * 0x0004, WP_EN, 1);
}

int nemc_init(struct nemc_config config, struct nemc_timing timing)
{
    if (!clk_gate_is_enable(nemc_clk)) {
        clk_gate_enable(nemc_clk);
    }
    nemc_set_tras(config.id, timing.tras);
    nemc_set_taw(config.id, timing.taw);
    nemc_set_tch(config.id, timing.tch);
    nemc_set_twas(config.id, timing.twas);
    nemc_set_tbp(config.id, timing.tbp);

    if (config.mode == MUXED_MODE) {
        nemc_set_tavds(config.id, timing.tavds);
        nemc_set_tavdp(config.id, timing.tavdp);
        nemc_set_tavdh(config.id, timing.tavdh);
    }

    if (config.mode == BURST_MODE)
        nemc_set_burstlen(config.id, timing.bl);

    if (timing.is_double_timing)
        nemc_set_wait_times_double(config.id, timing.is_double_timing);
    if (config.use_wait_pin)
        nemc_wait_pin_enable(config.id);

    if (config.data_width == 8)
        nemc_set_buswidth(config.id, 0);
    else
        nemc_set_buswidth(config.id, 1);

    nemc_set_strv(config.id, timing.strv);
    nemc_set_smt(config.id, config.mode);

    nemc_init_gpio(config);
    return 0;
}
