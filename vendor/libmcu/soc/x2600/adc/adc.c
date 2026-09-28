#include <driver/gpio.h>
#include <driver/clk.h>
#include <driver/irq.h>
#include <driver/adc.h>
#include <stdio.h>
#include <delay.h>
#include <assert.h>
#include <string.h>

#include "adc_hal.h"

#define GPIO_AUX0 GPIO_PE(5)

static void (*seq0_cb)(void);
static void (*seq1_cb)(void);
static void (*seq2_cb)(void);
static adc_awd_cb awd_cb;

struct dma *seq1_dma;
unsigned long seq1_dma_pos;

struct dma *seq2_dma;
unsigned long seq2_dma_pos;

static unsigned int adc_src_clk = 60*1000*1000;
static unsigned char adc_clk_div = 2;

static unsigned int adc_awd_enable_channels = 0;

static void set_gpio_func(unsigned char *channels, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        gpio_set_func(GPIO_AUX0 + channels[i], GPIO_FUNC_0);
    }
}

void adc_enable_seq0(struct adc_seq0_config *cfg)
{
    adc_disable_seq0(cfg);

    set_gpio_func(cfg->channels, cfg->channel_cnt);

    adc_hal_set_seq0_channels(cfg->channels, cfg->channel_cnt);

    adc_hal_set_seq0_CR_register(cfg->channel_cnt, 1);

    adc_set_bit(ADC_SR, SR_SEQ0_DR, 1);
    adc_set_bit(ADC_SR, SEQ0_EVT_OVR, 1);
    adc_set_bits(ADC_SEQ0_DCR, SEQ0_DRT, 0);

    seq0_cb = cfg->irq_cb;
    adc_set_bit(ADC_IE_ForRiscV, SEQ0_DR, !!cfg->irq_cb);
}

void adc_start_seq0(void)
{
    adc_set_bits(ADC_CR, SEQ0_START, 1);
}

int adc_poll_seq0_data_ready(void)
{
    return adc_get_bit(ADC_SR, SR_SEQ0_DR);
}

void adc_disable_seq0(struct adc_seq0_config *cfg)
{
    adc_set_bits(ADC_CR, SEQ0_RESET, 1);

    adc_set_bit(ADC_IE_ForRiscV, SEQ0_DR, 0);
    seq0_cb = NULL;
}

void adc_read_seq0_data(unsigned short *values, int len)
{
    if (len >= 3) {
        unsigned int dr1 = adc_read_reg(ADC_SEQ0_DR1);
        values[2] = get_bit_field(dr1, SEQ0_DR1_DATA2);
        if (len >= 4)
            values[3] = get_bit_field(dr1, SEQ0_DR1_DATA3);
    }
    if (len >= 1) {
        unsigned int dr0 = adc_read_reg(ADC_SEQ0_DR0);
        values[0] = get_bit_field(dr0, SEQ0_DR0_DATA0);
        if (len >= 2)
            values[1] = get_bit_field(dr0, SEQ0_DR0_DATA1);
    }
}

static void check_group_mode(struct adc_seq1_config *cfg)
{
    if (!cfg->group_cnt)
        return;

    int i, total = 0;
    for (i = 0; i < cfg->group_cnt; i++) {
        total += cfg->groups[i];
    }

    if (total != cfg->channel_cnt)
        panic("total groups must equal to channel_cnt\n");
}

void adc_enable_seq1(struct adc_seq1_config *cfg)
{
    check_group_mode(cfg);

    adc_disable_seq1(cfg);

    set_gpio_func(cfg->channels, cfg->channel_cnt);

    adc_hal_set_seq1_continus_clk_div(cfg->continus_clk_div);
    adc_hal_set_seq1_delay_clk_div(cfg->delay_clk_div);
    adc_hal_set_seq1_channels(cfg->channels, cfg->channel_cnt);
    adc_hal_set_seq1_delay(cfg->channel_delays, cfg->channel_cnt);
    adc_hal_set_seq1_group_mode(cfg->group_cnt, cfg->groups, cfg->group_delays);
    adc_hal_set_seq1_CR_register(cfg->trigger, cfg->channel_cnt, cfg->enable_channel_num);

    if (cfg->dma_buf == NULL || cfg->dma_size == 0)
        cfg->dma_mode = 0;

    unsigned int seq1_dcr = adc_read_reg(ADC_SEQ1_DCR);
    seq1_dcr = set_bit_field(seq1_dcr, SEQ1_DRT, 0);
    seq1_dcr = set_bit_field(seq1_dcr, SEQ1_DMA_MD, cfg->dma_mode);
    adc_write_reg(ADC_SEQ1_DCR, seq1_dcr);

    seq1_dma = NULL;
    seq1_dma_pos = (unsigned long)cfg->dma_buf;

    seq1_cb = cfg->irq_cb;
    adc_set_bit(ADC_IE_ForRiscV, SEQ1_DR, !!cfg->irq_cb && !cfg->dma_mode);

    if (cfg->dma_mode) {
        seq1_dma = dma_request(DMA_RQ_SADC_SEQ1_RX, NULL, NULL, DMA_bus_16bit, 2);
        dma_start_cyclic(seq1_dma, (void*)ADC_SEQ1_DR, (void*)cfg->dma_buf, cfg->dma_size, 1);
        adc_set_bit(ADC_IE_ForRiscV, SEQ1_DMA_FIN, !!cfg->irq_cb);
    }
}

void adc_start_seq1(void)
{
    adc_set_bits(ADC_CR, SEQ1_START, 1);
}

int adc_dma_poll_seq1_data_ready(struct adc_seq1_config *cfg)
{
    if (dma_read_dst_addr(seq1_dma) == seq1_dma_pos)
        return 0;

    return 1;
}

int adc_dma_seq1_get_readable_size(struct adc_seq1_config *cfg)
{
    unsigned int pos = dma_read_dst_addr(seq1_dma);

    return pos > seq1_dma_pos ? (pos-seq1_dma_pos) : (cfg->dma_size+pos-seq1_dma_pos);
}

unsigned int adc_dma_seq1_read_data(struct adc_seq1_config *cfg, unsigned short *data)
{
    assert(cfg->dma_buf && data);

    int len = adc_dma_seq1_get_readable_size(cfg);
    if (!len)
        return 0;

    if (len > cfg->channel_cnt*2)
        len = cfg->channel_cnt*2;

    unsigned long start_addr = (unsigned long)cfg->dma_buf;

    if ((seq1_dma_pos + len - start_addr) > cfg->dma_size) {
        int n = start_addr + cfg->dma_size - seq1_dma_pos;
        memcpy(data, (void *)seq1_dma_pos, n);
        memcpy(&data[n/2], (void *)start_addr, len - n);
        seq1_dma_pos = start_addr + len - n;
    } else {
        memcpy(data, (void *)seq1_dma_pos, len);
        seq1_dma_pos += len;
    }

    return len;
}

int adc_poll_seq1_data_ready(void)
{
    return adc_get_bit(ADC_SR, SR_SEQ1_DR);
}

void adc_read_seq1_data(unsigned short *values, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        values[i] = adc_read_reg(ADC_SEQ1_DR);
    }
}

void adc_disable_seq1(struct adc_seq1_config *cfg)
{
    if (seq1_dma) {
        dma_stop(seq1_dma);
        dma_release(seq1_dma);
    }

    adc_set_bits(ADC_CR, SEQ1_RESET, 1);

    adc_set_bit(ADC_IE_ForRiscV, SEQ1_DR, 0);
    adc_set_bit(ADC_IE_ForRiscV, SEQ1_DMA_FIN, 0);

    while (adc_get_bit(ADC_SR, SR_SEQ1_DR)) {
        adc_read_reg(ADC_SEQ1_DR);
    }

    seq1_cb = NULL;
}

void adc_enable_seq2(struct adc_seq2_config *cfg)
{
    adc_disable_seq2(cfg);

    set_gpio_func(cfg->channels, cfg->channel_cnt);

    adc_hal_set_seq2_delay_clk_div(cfg->delay_clk_div);
    adc_hal_set_seq2_channels(cfg->channels, cfg->channel_cnt);
    adc_hal_set_seq2_delay(cfg->channel_delays, cfg->channel_cnt);
    adc_hal_set_seq2_CR_register(cfg->trigger, cfg->channel_cnt, cfg->enable_channel_num);

    if (cfg->dma_buf == NULL || cfg->dma_size == 0)
        cfg->dma_mode = 0;

    unsigned int seq2_dcr = adc_read_reg(ADC_SEQ2_DCR);
    seq2_dcr = set_bit_field(seq2_dcr, SEQ2_DRT, 0);
    seq2_dcr = set_bit_field(seq2_dcr, SEQ2_DMA_MD, cfg->dma_mode);
    adc_write_reg(ADC_SEQ2_DCR, seq2_dcr);

    seq2_dma = NULL;
    seq2_dma_pos = (unsigned long)cfg->dma_buf;

    seq2_cb = cfg->irq_cb;
    adc_set_bit(ADC_IE_ForRiscV, SEQ2_DR, !!cfg->irq_cb && !cfg->dma_mode);

    if (cfg->dma_mode) {
        seq2_dma = dma_request(DMA_RQ_SADC_SEQ2_RX, NULL, NULL, DMA_bus_16bit, 2);
        dma_start_cyclic(seq2_dma, (void*)ADC_SEQ2_DR, (void*)cfg->dma_buf, cfg->dma_size, 1);
        adc_set_bit(ADC_IE_ForRiscV, SEQ2_DMA_FIN, !!cfg->irq_cb);
    }
}

void adc_start_seq2(void)
{
    adc_set_bits(ADC_CR, SEQ2_START, 1);
}

int adc_dma_poll_seq2_data_ready(struct adc_seq2_config *cfg)
{
    if (dma_read_dst_addr(seq2_dma) == (unsigned long)seq2_dma_pos)
        return 0;

    return 1;
}

int adc_dma_seq2_get_readable_size(struct adc_seq2_config *cfg)
{
    unsigned int pos = dma_read_dst_addr(seq2_dma);

    return pos > seq2_dma_pos ? (pos-seq2_dma_pos) : (cfg->dma_size+pos-seq2_dma_pos);
}

unsigned int adc_dma_seq2_read_data(struct adc_seq2_config *cfg, unsigned short *data)
{
    assert(cfg->dma_buf && data);

    int len = adc_dma_seq2_get_readable_size(cfg);
    if (!len)
        return 0;

    if (len > cfg->channel_cnt*2)
        len = cfg->channel_cnt*2;

    unsigned long start_addr = (unsigned long)cfg->dma_buf;

    if ((seq2_dma_pos + len - start_addr) > cfg->dma_size) {
        int n = start_addr + cfg->dma_size - seq2_dma_pos;
        memcpy(data, (void *)seq2_dma_pos, n);
        memcpy(&data[n/2], (void *)start_addr, len - n);
        seq2_dma_pos = start_addr + len - n;
    } else {
        memcpy(data, (void *)seq2_dma_pos, len);
        seq2_dma_pos += len;
    }

    return len;
}

int adc_poll_seq2_data_ready(void)
{
    return adc_get_bit(ADC_SR, SR_SEQ2_DR);
}

void adc_read_seq2_data(unsigned short *values, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        values[i] = adc_read_reg(ADC_SEQ2_DR);
    }
}

void adc_disable_seq2(struct adc_seq2_config *cfg)
{
    if (seq2_dma) {
        dma_stop(seq2_dma);
        dma_release(seq2_dma);
    }

    adc_set_bits(ADC_CR, SEQ2_RESET, 1);

    adc_set_bit(ADC_IE_ForRiscV, SEQ2_DR, 0);
    adc_set_bit(ADC_IE_ForRiscV, SEQ2_DMA_FIN, 0);

    while (adc_get_bit(ADC_SR, SR_SEQ2_DR)) {
        adc_read_reg(ADC_SEQ2_DR);
    }

    seq2_cb = NULL;
}

void adc_set_clk(int src_clk_rate, int div)
{
    clk_div_set_rate(CLK_DIV_SADC, src_clk_rate);

    adc_set_bits(ADC_CLKR0, ADCCLK_DIV, div);

    adc_src_clk = src_clk_rate;
    adc_clk_div = div;
}

void adc_set_seq_priority(char seq0_pri, char seq1_pri, char seq2_pri, int high_break_low)
{
    unsigned int cfr = adc_read_reg(ADC_CFR);
    cfr = set_bit_field(cfr, SEQ0_PRI, seq0_pri);
    cfr = set_bit_field(cfr, SEQ1_PRI, seq1_pri);
    cfr = set_bit_field(cfr, SEQ2_PRI, seq2_pri);
    cfr = set_bit_field(cfr, BREAK_MD, high_break_low);
    adc_write_reg(ADC_CFR, cfr);
}

void adc_enable_awd(int channel, int low_threshold, int high_threshold)
{
    unsigned int awd_cr;

    awd_cr = adc_read_reg(ADC_AWD_CR(channel));
    if (low_threshold >= 0) {
        awd_cr = set_bit_field(awd_cr, LTR, low_threshold);
        awd_cr = set_bit_field(awd_cr, LTR_EN, LTR_EN, 1);

        adc_awd_enable_channels |= BIT(channel);
        adc_set_bit(ADC_AWD_IM_ForRiscV, CH0_LTR + channel, 0);
    }
    if (high_threshold >= 0) {
        awd_cr = set_bit_field(awd_cr, HTR, high_threshold);
        awd_cr = set_bit_field(awd_cr, HTR_EN, HTR_EN, 1);

        adc_awd_enable_channels |= BIT(16 + channel);
        adc_set_bit(ADC_AWD_IM_ForRiscV, CH0_HTR + channel, 0);
    }
    adc_write_reg(ADC_AWD_CR(channel), awd_cr);

}

void adc_disable_awd(int channel)
{
    adc_set_bit(ADC_AWD_IM_ForRiscV, CH0_LTR + channel, 1);
    adc_set_bit(ADC_AWD_IM_ForRiscV, CH0_HTR + channel, 1);

    adc_awd_enable_channels &= ~BIT(channel);
    adc_awd_enable_channels &= ~BIT(16 + channel);

    adc_set_bit(ADC_AWD_CR(channel), LTR_EN, 0);
    adc_set_bit(ADC_AWD_CR(channel), HTR_EN, 0);
}

void adc_set_awd_cb(adc_awd_cb cb)
{
    awd_cb = cb;
}

void adc_irq_handler(int irq, void *data)
{
    unsigned int adc_ir = adc_read_reg(ADC_IR_ForRiscV);

    /* SEQ(n)_DR 的标志位必须在读取后才能清零 */
    if (test_bit(adc_ir, SEQ0_DR)) {
        if (seq0_cb)
            seq0_cb();
        adc_set_bit(ADC_IR_ForRiscV, SEQ0_DR, 1);
    }

    if (test_bit(adc_ir, SEQ1_DR)) {
        if (seq1_cb)
            seq1_cb();
        adc_set_bit(ADC_IR_ForRiscV, SEQ1_DR, 1);
    }

    if (test_bit(adc_ir, SEQ1_DMA_FIN)) {
        adc_set_bit(ADC_IR_ForRiscV, SEQ1_DMA_FIN, 1);
        if (seq1_cb)
            seq1_cb();
    }

    if (test_bit(adc_ir, SEQ2_DR)) {
        if (seq2_cb)
            seq2_cb();
        adc_set_bit(ADC_IR_ForRiscV, SEQ2_DR, 1);
    }

    if (test_bit(adc_ir, SEQ2_DMA_FIN)) {
        adc_set_bit(ADC_IR_ForRiscV, SEQ2_DMA_FIN, 1);
        if (seq2_cb)
            seq2_cb();
    }

    if (test_bit(adc_ir, AWD)) {
        unsigned int adc_awd_sr = adc_read_reg(ADC_AWD_SR) & adc_awd_enable_channels;
        if (adc_awd_sr) {
            unsigned short low_flags = adc_awd_sr & 0xFFFF;
            unsigned short high_flags = adc_awd_sr >> 16;
            if (awd_cb)
                awd_cb(low_flags, high_flags);

            adc_write_reg(ADC_AWD_SR, 0xFFFFFFFF);

            adc_set_bit(ADC_IR_ForRiscV, AWD, 1);
        }
    }
}

void adc_init(void)
{
    clk_gate_enable(CLK_GATE_SADC);
    clk_div_enable(CLK_DIV_SADC);

    adc_set_clk(adc_src_clk, adc_clk_div);

    adc_hal_power_on_phy();

    /* 禁止中断,并且清掉已经有效的中断(不然即使禁止了中断也不行,算是个小bug)
     */
    adc_write_reg(ADC_IE_ForRiscV, 0);
    adc_write_reg(ADC_IR_ForRiscV, 0xffffffff);
    adc_write_reg(ADC_AWD_IM_ForRiscV, 0xffffffff);

    adc_awd_enable_channels = 0;

    request_irq(IRQ_SADC, 0, adc_irq_handler, "adc", NULL);
}

void adc_deinit(void)
{
    disable_irq(IRQ_SADC);
    release_irq(IRQ_SADC);

    adc_hal_power_off_phy();

    clk_div_disable(CLK_DIV_SADC);
    clk_gate_disable(CLK_GATE_SADC);
}

void adc_dump_regs(void)
{
    int i;

    printf("ADC_SR:0x%08x\n", adc_read_reg(ADC_SR));
    printf("ADC_IE:0x%08x\n", adc_read_reg(ADC_IE));
    printf("ADC_IE_ForRiscV:0x%08x\n", adc_read_reg(ADC_IE_ForRiscV));
    printf("ADC_IR:0x%08x\n", adc_read_reg(ADC_IR));
    printf("ADC_IR_ForRiscV:0x%08x\n", adc_read_reg(ADC_IR_ForRiscV));
    printf("ADC_CR:0x%08x\n", adc_read_reg(ADC_CR));
    printf("ADC_CFR:0x%08x\n", adc_read_reg(ADC_CFR));
    printf("ADC_CLKR0:0x%08x\n", adc_read_reg(ADC_CLKR0));
    printf("ADC_CLKR1:0x%08x\n", adc_read_reg(ADC_CLKR1));
    printf("ADC_CLKR2:0x%08x\n", adc_read_reg(ADC_CLKR2));
    printf("ADC_EXT_GPIO_CR:0x%08x\n", adc_read_reg(ADC_EXT_GPIO_CR));
    for (i = 0; i < 16; i++)
        printf("ADC_AWD_CR(%d):0x%08x\n", i, adc_read_reg(ADC_AWD_CR(i)));
    printf("ADC_AWD_SR:0x%08x\n", adc_read_reg(ADC_AWD_SR));
    printf("ADC_AWD_IM:0x%08x\n", adc_read_reg(ADC_AWD_IM));
    printf("ADC_AWD_IM_ForRiscV:0x%08x\n", adc_read_reg(ADC_AWD_IM_ForRiscV));
    printf("ADC_AWD_IR:0x%08x\n", adc_read_reg(ADC_AWD_IR));
    printf("ADC_AWD_IR_ForRiscV:0x%08x\n", adc_read_reg(ADC_AWD_IR_ForRiscV));
    printf("ADC_DBG_CR:0x%08x\n", adc_read_reg(ADC_DBG_CR));
    printf("ADC_DBG_FSM:0x%08x\n", adc_read_reg(ADC_DBG_FSM));
    printf("ADC_SEQ0_CR:0x%08x\n", adc_read_reg(ADC_SEQ0_CR));
    printf("ADC_SEQ0_CNR0:0x%08x\n", adc_read_reg(ADC_SEQ0_CNR0));
    printf("ADC_SEQ0_DCR:0x%08x\n", adc_read_reg(ADC_SEQ0_DCR));
    printf("ADC_SEQ0_DR0:0x%08x\n", adc_read_reg(ADC_SEQ0_DR0));
    printf("ADC_SEQ0_DR1:0x%08x\n", adc_read_reg(ADC_SEQ0_DR1));
    printf("ADC_SEQ1_CR:0x%08x\n", adc_read_reg(ADC_SEQ1_CR));
    printf("ADC_SEQ1_CNR0:0x%08x\n", adc_read_reg(ADC_SEQ1_CNR0));
    printf("ADC_SEQ1_CNR1:0x%08x\n", adc_read_reg(ADC_SEQ1_CNR1));
    printf("ADC_SEQ1_DCR:0x%08x\n", adc_read_reg(ADC_SEQ1_DCR));
    printf("ADC_SEQ1_DR:0x%08x\n", adc_read_reg(ADC_SEQ1_DR));
    for (i = 0; i < 8; i++)
        printf("ADC_SEQ1_DLY(%d):0x%08x\n", i, adc_read_reg(ADC_SEQ1_DLY(i)));
    for (i = 0; i < 8; i++)
        printf("ADC_SEQ1_CONT_CR(%d):0x%08x\n", i, adc_read_reg(ADC_SEQ1_CONT_CR(i)));
    printf("ADC_SEQ1_RCNT:0x%08x\n", adc_read_reg(ADC_SEQ1_RCNT));
    printf("ADC_SEQ1_DMA_RCNT:0x%08x\n", adc_read_reg(ADC_SEQ1_DMA_RCNT));
    printf("ADC_SEQ2_CR:0x%08x\n", adc_read_reg(ADC_SEQ2_CR));
    printf("ADC_SEQ2_CNR0:0x%08x\n", adc_read_reg(ADC_SEQ2_CNR0));
    printf("ADC_SEQ2_DCR:0x%08x\n", adc_read_reg(ADC_SEQ2_DCR));
    printf("ADC_SEQ2_DR:0x%08x\n", adc_read_reg(ADC_SEQ2_DR));
    for (i = 0; i < 2; i++)
        printf("ADC_SEQ2_DLY(%d):0x%08x\n", i, adc_read_reg(ADC_SEQ2_DLY(i)));
    printf("ADC_SEQ2_RCNT:0x%08x\n", adc_read_reg(ADC_SEQ2_RCNT));
    printf("ADC_SEQ2_DMA_RCNT:0x%08x\n", adc_read_reg(ADC_SEQ2_DMA_RCNT));

}
