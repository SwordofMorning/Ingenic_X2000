#include <stdio.h>
#include <stdlib.h>
#include <common.h>
#include <assert.h>
#include <driver/clk.h>
#include <driver/dma.h>
#include <soc/base.h>

#include <driver/irq.h>

#include "tpc_regs.h"
#include "tpc_hal.h"

#define DEFALT_SHIFT_SETTIME  0
#define DEFALT_SHFIT_HOLDTIME 0

#define MOTOR_DMA_THRESHOLD    1
#define SHIFT_DMA_THRESHOLD    1
#define HEAT_DMA_THRESHOLD     1

struct {
    int is_init;
    struct clk *cgu_clk;
    struct clk *gate_clk;
    int tpc_clk_rate;
}tpc;

static int bit_field_start(int start, int end)
{
    return start;
}

static void tpc_set_clk_div(int div)
{
    tpc_set_bit(PDSCD, PRESCALE, div);
}


static void tpc_enable_all_interupt(void)
{
    /*PINTM0 使能xburst2，PINTM1 使能RISCV*/
    // tpc_write_reg(PINTM0, 0xffffffff);
    tpc_write_reg(PINTM1, 0x0);
}

void dump_tpc_reg(void)
{
    int i;
    printf("---------TPC DUMP-----------\n");
    printf("PFCEN: 0x%08x\n", tpc_read_reg(0x0008));
    printf("PDEN: 0x%08x\n", tpc_read_reg(PDEN));
    printf("PIOC: 0x%08x\n", tpc_read_reg(PIOC));
    printf("PM0EN: 0x%08x\n", tpc_read_reg(PMEN(0)));
    printf("PM1EN: 0x%08x\n", tpc_read_reg(PMEN(1)));
    printf("PMCILC: 0x%08x\n", tpc_read_reg(PMCILC));
    printf("PMCSLC: 0x%08x\n", tpc_read_reg(PMCSLC));
    printf("PMCELC: 0x%08x\n", tpc_read_reg(PMCELC));
    printf("PM0BC: 0x%08x\n", tpc_read_reg(PMBC(0)));
    printf("PM1BC: 0x%08x\n", tpc_read_reg(PMBC(1)));
    printf("PM0SSC: 0x%08x\n", tpc_read_reg(PMSSC(0)));
    printf("PM0ESC: 0x%08x\n", tpc_read_reg(PMESC(0)));
    printf("PM1SSC: 0x%08x\n", tpc_read_reg(PMSSC(1)));
    printf("PM1ESC: 0x%08x\n", tpc_read_reg(PMESC(1)));
    printf("PM0MC: 0x%08x\n", tpc_read_reg(PMMC(0)));
    printf("PM1MC: 0x%08x\n", tpc_read_reg(PMMC(1)));
    for (i = 0; i < 8; i++) {
        printf("PMC0RLC0(%d) = 0x%08x\n", i, tpc_read_reg(PMCRLC0(i)));
        printf("PMC0RLC1(%d) = 0x%08x\n", i, tpc_read_reg(PMCRLC1(i)));
    }
    printf("PMCMS: 0x%08x\n", tpc_read_reg(PMCMS));
    printf("PM0MSC: 0x%08x\n", tpc_read_reg(PMMSC(0)));
    printf("PM1MSC: 0x%08x\n", tpc_read_reg(PMMSC(1)));
    // printf("PAFC: 0x%08x\n", tpc_read_reg(PAFC));
    printf("PDSBC: 0x%08x\n", tpc_read_reg(PDSBC));
    printf("PDSCD: 0x%08x\n", tpc_read_reg(PDSCD));
    printf("PDSMC: 0x%08x\n", tpc_read_reg(PDSMC));
    printf("PDLMC: 0x%08x\n", tpc_read_reg(PDLMC));
    printf("PDDMC: 0x%08x\n", tpc_read_reg(PDDMC));
    printf("PDSDFC: 0x%08x\n", tpc_read_reg(PDSDFC));
    printf("PDSWC: 0x%08x\n", tpc_read_reg(PDSWC));
    printf("PDLBC: 0x%08x\n", tpc_read_reg(PDLBC));
    printf("PDLWC: 0x%08x\n", tpc_read_reg(PDLWC));
    printf("PDLAC: 0x%08x\n", tpc_read_reg(PDLAC));
    printf("PDDBC: 0x%08x\n", tpc_read_reg(PDDBC));
    printf("PDDWC: 0x%08x\n", tpc_read_reg(PDDWC));
    printf("PDDMAC: 0x%08x\n", tpc_read_reg(PDDMAC));
    printf("PDDTFC: 0x%08x\n", tpc_read_reg(PDDTFC));
    printf("PWDTF: 0x%08x\n", tpc_read_reg(PWDTF));
    printf("PWDTFE: 0x%08x\n", tpc_read_reg(PWDTFE));
    printf("PWDTV: 0x%08x\n", tpc_read_reg(PWDTV));
    printf("PWDTC: 0x%08x\n", tpc_read_reg(PWDTC));

    printf("PFFS: 0x%08x\n", tpc_read_reg(PFFS));
}

void tpc_hal_set_shift_trigger(int size)
{
    tpc_set_bit(PDSDFC, PDSD_THRSHD, size);
}

void tpc_hal_set_heat_trigger(int size)
{
    tpc_set_bit(PDDTFC, PDDT_THRSHD, size);
}

void tpc_hal_set_motor_trigger(int id, int size)
{
    tpc_set_bit(PMTFC(id), PMT_THRSHD, size);
}

void tpc_hal_shift_config(struct tpc_shift_cfg *config)
{
    uint64_t clk_rate = config->out_clk_rate * 2;

    if (config->line_bytes % config->channels)
        panic("shift data lines bytes = %d not align channels = %d\n", config->line_bytes, config->channels);

    int data_len = config->line_bytes / config->channels;

    int div = tpc.tpc_clk_rate / clk_rate;
   /*配置分频率*/
    tpc_set_clk_div(div);

    tpc_set_bit(PDSBC, PDSIL, config->idle_level);
    tpc_set_bit(PDSBC, PDSDE, config->big_end);
    tpc_set_bit(PDSBC, PDSPHA, config->spi_pha);
    tpc_set_bit(PDSBC, PDSPOL, config->spi_pol);
    tpc_set_bit(PDSBC, PDSTUP, DEFALT_SHIFT_SETTIME);
    tpc_set_bit(PDSBC, PDHD, DEFALT_SHFIT_HOLDTIME);
    tpc_set_bit(PDSBC, PDSCN, config->channels);
    tpc_set_bit(PDSBC, PDSDI, config->data_invert);
    tpc_set_bit(PDSBC, PDSBL, data_len);

    /* 配置data dma fifo阈值 */
    tpc_set_bit(PDSDFC, PDSD_THRSHD, SHIFT_DMA_THRESHOLD);

    tpc_set_bit(PDENS, PDSEN, 1);
}

void tpc_hal_lat_config(struct tpc_lat_cfg *config)
{
    /* 配置锁存到数据移位的时间 */
    tpc_set_bit(PDSWC, PDSWCNT, config->wait_clkcnt_out_shift);

    /* 配置锁存信号无效电平 */
    tpc_set_bit(PDLBC, PDLIL, config->idle_level);

    /* 配置触发锁存后，等待 WCNT 后输出锁存信号 */
    tpc_set_bit(PDLWC, PDLWCNT, config->wait_clkcnt_out);

    /* 配置锁存有效电平时间 */
    tpc_set_bit(PDLAC, PDLACNT, config->active_clkcnt);

    tpc_set_bit(PDENS, PDLEN, 1);
}

void tpc_hal_heat_config(struct tpc_heat_cfg *config)
{
    /* 配置加热通道数 */
    tpc_set_bit(PDDBC, PDDCN, config->channels);

    /* 配置加热初始电平设置 */
    tpc_set_bit(PDDBC, PDLIL, config->idle_level);

    /* 配置触发加热到真正加热等待时间 */
    tpc_set_bit(PDDWC, PDDWCNT, config->wait_clkcnt_out);

    /* 配置最大加热时间 */
    tpc_set_bit(PDDMAC, PDDMCNT, config->max_heat_clkcnt);

    /* 配置热信号 dma fifo阈值 */
    tpc_set_bit(PDDTFC, PDDT_THRSHD, HEAT_DMA_THRESHOLD);

    tpc_set_bit(PDENS, PDDEN, 1);
}

void tpc_hal_heat_flush_fifo(void)
{
    tpc_set_bit(PDDTFC, PDDTFF, 1);
    while(tpc_get_bit(PDDTFC, PDDTFF));
}


void tpc_hal_motor_flush_fifo(int id)
{
    tpc_set_bit(PMTFC(id), PMTFF, 1);
    while(tpc_get_bit(PMTFC(id), PMTFF));
}

void tpc_hal_shift_flush_fifo(void)
{
    tpc_set_bit(PDSDFC, PDSDFF, 1);
    while(tpc_get_bit(PDSDFC, PDSDFF));
}

void tpc_hal_motor_config(int id, struct tpc_motor_cfg *config)
{
    int channels = config->channels;

    /*当id为0时, 并且通道数大于4时才设置该寄存器，否则当两个电机使用*/
    if (!id && config->channels > 4)
        tpc_set_bit(PMCMS, PMCMS_SELECT, 1);

    int ch_start = bit_field_start(PMC0IL);

    if (id)
        ch_start = bit_field_start(PMC4IL);

    /* 配置idle电平 */
    tpc_set_bit(PMCILC, ch_start, ch_start + channels - 1, config->idle_level);

    /* 配置strat步电平 */
    tpc_set_bit(PMCSLC, ch_start, ch_start + channels - 1, config->start_step_io);

    /* 配置end步电平 */
    tpc_set_bit(PMCELC, ch_start, ch_start + channels - 1, config->stop_step_io);

    /* 配置strat和end步电平时间 */
    tpc_write_reg(PMSSC(id), config->start_step_clkcnt);
    tpc_write_reg(PMESC(id), config->stop_step_clkcnt);

    /*配置循环步数以及对齐步index*/
    tpc_set_bit(PMBC(id), PMRSN, config->repeat_step);
    tpc_set_bit(PMBC(id), PMASI, config->align_step);

    /* 配置循环步电平 */
    int i;
    for (i = ch_start; i < channels + ch_start; i++) {
        tpc_write_reg(PMCRLC0(i), config->repeat_step_io[i]);
        tpc_write_reg(PMCRLC1(i), config->repeat_step_io[i] >> 32);
    }

    /* 配置最大步长 */
    tpc_write_reg(PMMSC(id), config->max_step_clkcnt);

    /*先关闭循环步的每步中断*/
    tpc_write_reg(PMRIM0(id), 0xffffffff);
    tpc_write_reg(PMRIM1(id), 0xffffffff);

    /*开启对齐步的中断，以便注册motor 回调*/
    if (config->align_step > 32)
        tpc_set_bit(PMRIM1(id), config->align_step - 32, config->align_step - 32, 0);
    else
        tpc_set_bit(PMRIM0(id), config->align_step - 1, config->align_step - 1, 0);

    /* 配置motor dma fifo阈值 */
    tpc_set_bit(PMTFC(id), PMT_THRSHD, MOTOR_DMA_THRESHOLD);

    int ch_en_start = bit_field_start(PMCEN(ch_start));
    tpc_set_bit(PMENS(id), ch_en_start, ch_en_start + channels - 1, 0xff);
}

void tpc_hal_disable_sync_mode(int motor_id)
{
    /*清除电机自动对齐使能*/
    if (motor_id >= 0) {
        tpc_set_bit(PMENC(motor_id), PMAEN, 1);
        tpc_set_bit(PMENC(motor_id), PMAAEN, 1);
    }

    tpc_set_bit(PDENC, PDAAEN, 1);
    tpc_set_bit(PDENC, PDAEN, 1);
    tpc_set_bit(PFCENC, PFCEN, 1);
}

void tpc_hal_enable_sync_mode(int motor_id)
{
    tpc_set_bit(PDENS, PDAEN, 1);

    if (motor_id >= 0) {
        tpc_set_bit(PMENS(motor_id), PMAEN, 1);
        tpc_set_bit(PMENS(motor_id), PMAAEN, 1);
        tpc_set_bit(PFCENS, PFCEN, 1);
        tpc_set_bit(PDENS, PDAAEN, 1);
    }
}

void tpc_hal_wait_heat_fifo_not_empty(void)
{
    while (tpc_get_bit(PFFS, PDFE));
}

void tpc_hal_wait_shift_fifo_not_empty(void)
{
    while (tpc_get_bit(PFFS, PSFE));
}

void tpc_hal_wait_motor_fifo_not_empty(int id)
{
    while (tpc_get_bit(PFFS, PSMFE(id)));
}


void tpc_hal_wait_heat_fifo_empty(void)
{
    while (!tpc_get_bit(PFFS, PDFE));
}

void tpc_hal_wait_shift_fifo_empty(void)
{
    while (!tpc_get_bit(PFFS, PSFE));
}

void tpc_hal_wait_motor_fifo_empty(int id)
{
    while (!tpc_get_bit(PFFS, PSMFE(id)));
}

void tpc_hal_init(void)
{
    if (tpc.is_init++ != 0)
        return;

    clk_gate_enable(CLK_GATE_TPC);

    clk_div_set_rate(CLK_DIV_TPC, DEFAULT_TPC_CLK_RATE);

    clk_div_enable(CLK_DIV_TPC);

    tpc.tpc_clk_rate = clk_div_get_rate(CLK_DIV_TPC);

    tpc_enable_all_interupt();
}