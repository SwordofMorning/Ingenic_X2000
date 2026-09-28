#include <linux/spinlock.h>
#include <utils/gpio.h>

#include "adc.h"
#include "adc_hal.h"
#include "adc_reg.h"

#define GPIO_AUX0                       GPIO_PE(5)

static DEFINE_SPINLOCK(lock);

static unsigned int adc_awd_enable_channels = 0;

static void (*seq0_cb)(void);
static void (*seq1_cb)(void);
static void (*seq2_cb)(void);
static adc_awd_cb awd_cb;

static void set_gpio_func(unsigned char *channels, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        gpio_set_func(GPIO_AUX0 + channels[i], GPIO_FUNC_0);
    }
}

void adc_enable_seq0(struct adc_seq0_config *cfg)
{
    unsigned long flags;

    adc_disable_seq0(cfg);

    spin_lock_irqsave(&lock, flags);

    set_gpio_func(cfg->channels, cfg->channel_cnt);

    adc_hal_set_seq0_channels(cfg->channels, cfg->channel_cnt);

    adc_hal_set_seq0_CR_register(cfg->channel_cnt, 1);

    adc_set_bit(ADC_SR, SR_SEQ0_DR, 1);
    adc_set_bit(ADC_SR, SR_SEQ0_EVT_OVR, 1);
    adc_set_bit(ADC_SEQ0_DCR, SEQ0_DRT, 0);

    seq0_cb = cfg->irq_cb;
    adc_set_bit(ADC_IE, IE_SEQ0_DR, !!cfg->irq_cb);

    spin_unlock_irqrestore(&lock, flags);
}

void adc_start_seq0(void)
{
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    adc_set_bit(ADC_CR, CR_SEQ0_START, 1);

    spin_unlock_irqrestore(&lock, flags);
}

int adc_poll_seq0_data_ready(void)
{
    return adc_get_bit(ADC_SR, SR_SEQ0_DR);
}

void adc_disable_seq0(struct adc_seq0_config *cfg)
{
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    adc_set_bit(ADC_CR, CR_SEQ0_RESET, 1);

    adc_set_bit(ADC_IE, IE_SEQ0_DR, 0);
    seq0_cb = NULL;

    spin_unlock_irqrestore(&lock, flags);
}

void adc_read_seq0_data(unsigned short *values, int len)
{
    if (len >= 3) {
        unsigned long dr1 = adc_read_reg(ADC_SEQ0_DR1);
        values[2] = get_bit_field(&dr1, SEQ0_DR1_DATA2);
        if (len >= 4)
            values[3] = get_bit_field(&dr1, SEQ0_DR1_DATA3);
    }
    if (len >= 1) {
        unsigned long dr0 = adc_read_reg(ADC_SEQ0_DR0);
        values[0] = get_bit_field(&dr0, SEQ0_DR0_DATA0);
        if (len >= 2)
            values[1] = get_bit_field(&dr0, SEQ0_DR0_DATA1);
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
    unsigned long flags;

    check_group_mode(cfg);

    adc_disable_seq1(cfg);

    spin_lock_irqsave(&lock, flags);

    set_gpio_func(cfg->channels, cfg->channel_cnt);

    adc_hal_set_seq1_continus_clk_div(cfg->continus_clk_div);
    adc_hal_set_seq1_delay_clk_div(cfg->delay_clk_div);
    adc_hal_set_seq1_channels(cfg->channels, cfg->channel_cnt);
    adc_hal_set_seq1_delay(cfg->channel_delays, cfg->channel_cnt);
    adc_hal_set_seq1_group_mode(cfg->group_cnt, cfg->groups, cfg->group_delays);
    adc_hal_set_seq1_CR_register(cfg->trigger, cfg->channel_cnt, cfg->enable_channel_num);

    seq1_cb = cfg->irq_cb;
    adc_set_bit(ADC_IE, IE_SEQ1_DR, !!cfg->irq_cb);

    spin_unlock_irqrestore(&lock, flags);
}

void adc_start_seq1(void)
{
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    adc_set_bit(ADC_CR, CR_SEQ1_START, 1);

    spin_unlock_irqrestore(&lock, flags);
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
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    adc_set_bit(ADC_CR, CR_SEQ1_RESET, 1);

    adc_set_bit(ADC_IE, IE_SEQ1_DR, 0);

    while (adc_get_bit(ADC_SR, SR_SEQ1_DR)) {
        adc_read_reg(ADC_SEQ1_DR);
    }

    seq1_cb = NULL;

    int fifo_cnt = adc_get_bits(ADC_SEQ1_DCR, SEQ1_FIFO_CNT);
    if (fifo_cnt) {
        unsigned short values[32];
        adc_read_seq1_data(values, fifo_cnt);
    }

    spin_unlock_irqrestore(&lock, flags);
}

void adc_enable_seq2(struct adc_seq2_config *cfg)
{
    unsigned long flags;

    adc_disable_seq2(cfg);

    spin_lock_irqsave(&lock, flags);

    set_gpio_func(cfg->channels, cfg->channel_cnt);

    adc_hal_set_seq2_delay_clk_div(cfg->delay_clk_div);
    adc_hal_set_seq2_channels(cfg->channels, cfg->channel_cnt);
    adc_hal_set_seq2_delay(cfg->channel_delays, cfg->channel_cnt);
    adc_hal_set_seq2_CR_register(cfg->trigger, cfg->channel_cnt, cfg->enable_channel_num);

    seq2_cb = cfg->irq_cb;
    adc_set_bit(ADC_IE, IE_SEQ2_DR, !!cfg->irq_cb);

    spin_unlock_irqrestore(&lock, flags);
}

void adc_start_seq2(void)
{
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    adc_set_bit(ADC_CR, CR_SEQ2_START, 1);

    spin_unlock_irqrestore(&lock, flags);
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
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    adc_set_bit(ADC_CR, CR_SEQ2_RESET, 1);

    adc_set_bit(ADC_IE, IE_SEQ2_DR, 0);

    while (adc_get_bit(ADC_SR, SR_SEQ2_DR)) {
        adc_read_reg(ADC_SEQ2_DR);
    }

    seq2_cb = NULL;

    int fifo_cnt = adc_get_bits(ADC_SEQ2_DCR, SEQ2_FIFO_CNT);
    if (fifo_cnt) {
        unsigned short values[16];
        adc_read_seq2_data(values, fifo_cnt);
    }

    spin_unlock_irqrestore(&lock, flags);
}

void adc_set_seq_priority(char seq0_pri, char seq1_pri, char seq2_pri, int high_break_low)
{
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    unsigned long cfr = adc_read_reg(ADC_CFR);
    set_bit_field(&cfr, CFR_SEQ0_PRI, seq0_pri);
    set_bit_field(&cfr, CFR_SEQ1_PRI, seq1_pri);
    set_bit_field(&cfr, CFR_SEQ2_PRI, seq2_pri);
    set_bit_field(&cfr, CFR_BREAK_MD, CFR_BREAK_MD, high_break_low);
    adc_write_reg(ADC_CFR, cfr);

    spin_unlock_irqrestore(&lock, flags);
}

void adc_clean_all_interrupt_flag(void)
{
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    adc_write_reg(ADC_IE, 0);
    adc_write_reg(ADC_IR, 0xffffffff);
    adc_write_reg(ADC_AWD_IM, 0xffffffff);

    spin_unlock_irqrestore(&lock, flags);
}

void adc_power_on(void)
{
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    adc_hal_power_on_phy();

    spin_unlock_irqrestore(&lock, flags);
}

void adc_power_off(void)
{
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    adc_hal_power_off_phy();

    spin_unlock_irqrestore(&lock, flags);

}

void adc_enable_awd(int channel, int low_threshold, int high_threshold)
{
    unsigned long awd_cr;
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    awd_cr = adc_read_reg(ADC_AWD_CR(channel));
    if (low_threshold >= 0) {
        set_bit_field(&awd_cr, LTR, low_threshold);
        set_bit_field(&awd_cr, LTR_EN, LTR_EN, 1);

        adc_awd_enable_channels |= BIT(channel);
        adc_set_bit(ADC_AWD_IM, CH0_LTR_FLG + channel, 0);
    }
    if (high_threshold >= 0) {
        set_bit_field(&awd_cr, HTR, high_threshold);
        set_bit_field(&awd_cr, HTR_EN, HTR_EN, 1);

        adc_awd_enable_channels |= BIT(16 + channel);
        adc_set_bit(ADC_AWD_IM, CH0_HTR_FLG + channel, 0);
    }
    adc_write_reg(ADC_AWD_CR(channel), awd_cr);

    spin_unlock_irqrestore(&lock, flags);
}

void adc_disable_awd(int channel)
{
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    adc_set_bit(ADC_AWD_IM, CH0_LTR_FLG + channel, 1);
    adc_set_bit(ADC_AWD_IM, CH0_HTR_FLG + channel, 1);

    adc_awd_enable_channels &= ~BIT(channel);
    adc_awd_enable_channels &= ~BIT(16 + channel);

    adc_set_bit(ADC_AWD_CR(channel), LTR_EN, 0);
    adc_set_bit(ADC_AWD_CR(channel), HTR_EN, 0);

    spin_unlock_irqrestore(&lock, flags);
}

void adc_set_awd_cb(adc_awd_cb cb)
{
    awd_cb = cb;
}

irqreturn_t adc_irq_handler(int irq, void *data)
{
    unsigned int adc_ir = adc_read_reg(ADC_IR);

    /* SEQ(n)_DR 的标志位必须在读取后才能清零 */
    if (adc_ir & BIT(IR_SEQ0_DR)) {
        if (seq0_cb)
            seq0_cb();

        adc_set_bit(ADC_IR, IR_SEQ0_DR, 1);
    }

    if (adc_ir & BIT(IR_SEQ1_DR)) {
        if (seq1_cb)
            seq1_cb();

        adc_set_bit(ADC_IR, IR_SEQ1_DR, 1);
    }

    if (adc_ir & BIT(IR_SEQ2_DR)) {
        if (seq2_cb)
            seq2_cb();

        adc_set_bit(ADC_IR, IR_SEQ2_DR, 1);
    }

    if (adc_ir & BIT(IR_AWD)) {
        unsigned int adc_awd_sr = adc_read_reg(ADC_AWD_SR) & adc_awd_enable_channels;
        if (adc_awd_sr) {
            unsigned short low_flags = adc_awd_sr & 0xFFFF;
            unsigned short high_flags = adc_awd_sr >> 16;
            if (awd_cb)
                awd_cb(low_flags, high_flags);

            adc_write_reg(ADC_AWD_SR, adc_awd_sr);
            adc_set_bit(ADC_IR, IR_AWD, 1);
        }
    }

    return IRQ_HANDLED;
}
