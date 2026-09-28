#include "adc_regs.h"
#include <driver/adc.h>
#include <stdio.h>

static void adc_hal_power_on_phy(void)
{
    if (!adc_get_bits(ADC_CFR, PHY_PD))
        return;

    adc_set_bits(ADC_CFR, PHY_PD, 0);

    unsigned int cfr = adc_read_reg(ADC_CFR);
    cfr = set_bit_field(cfr, PHY_SEL_EN, 1);
    cfr = set_bit_field(cfr, PHY_RESET, 0);
    adc_write_reg(ADC_CFR, cfr);
}

static void adc_hal_power_off_phy(void)
{
    unsigned int cfr = adc_read_reg(ADC_CFR);
    cfr = set_bit_field(cfr, PHY_SEL_EN, 0);
    cfr = set_bit_field(cfr, PHY_RESET, 1);
    adc_write_reg(ADC_CFR, cfr);

    adc_set_bits(ADC_CFR, PHY_PD, 1);
}

static void adc_hal_set_seq0_channels(unsigned char *channels, int len)
{
    unsigned int seq0_cnr0 = adc_read_reg(ADC_SEQ0_CNR0);
    if (len >= 4)
        seq0_cnr0 = set_bit_field(seq0_cnr0, SEQ0_CH_NUM3, channels[3]);
    if (len >= 3)
        seq0_cnr0 = set_bit_field(seq0_cnr0, SEQ0_CH_NUM2, channels[2]);
    if (len >= 2)
        seq0_cnr0 = set_bit_field(seq0_cnr0, SEQ0_CH_NUM1, channels[1]);
    if (len >= 1)
        seq0_cnr0 = set_bit_field(seq0_cnr0, SEQ0_CH_NUM0, channels[0]);
    adc_write_reg(ADC_SEQ0_CNR0, seq0_cnr0);
}

static void adc_hal_set_seq0_CR_register(int channel_cnt, int enable_channel_num)
{
    unsigned int seq0_cr = adc_read_reg(ADC_SEQ0_CR);
    seq0_cr = set_bit_field(seq0_cr, SEQ0_LEN, channel_cnt-1);
    seq0_cr = set_bit_field(seq0_cr, SEQ0_CH_NUM_EN, enable_channel_num);
    adc_write_reg(ADC_SEQ0_CR, seq0_cr);
}

static void adc_hal_set_seq1_continus_clk_div(unsigned int div)
{
    if (div == 0)
        div = 1;

    adc_set_bits(ADC_CLKR0, SEQ1_CONTCLK_DIV, div-1);
}

static void adc_hal_set_seq1_delay_clk_div(unsigned int div)
{
    if (div == 0)
        div = 1;

    adc_set_bits(ADC_CLKR1, SEQ1_DLYCLK_DIV, div-1);
}

static void adc_hal_set_seq1_group_mode(
    unsigned char group_len, unsigned char *lens, unsigned short *delays)
{
    unsigned int cr = adc_read_reg(ADC_SEQ1_CR);
    if (group_len) {
        cr = set_bit_field(cr, SEQ1_CONT_EN, 1);
        cr = set_bit_field(cr, SEQ1_CONT_GLEN, group_len-1);
    } else {
        cr = set_bit_field(cr, SEQ1_CONT_EN, 0);
        cr = set_bit_field(cr, SEQ1_CONT_GLEN, 0);
    }
    adc_write_reg(ADC_SEQ1_CR, cr);

    int i;
    for (i = 0; i < group_len; i++) {
        unsigned int cr = 0;
        cr = set_bit_field(cr, SEQ1_GRP_LEN, lens[i]-1);
        cr = set_bit_field(cr, SEQ1_CONT_CNT, delays[i]);
        adc_write_reg(ADC_SEQ1_CONT_CR(i), cr);
    }
}

static void adc_hal_set_seq1_delay(unsigned short *delays, int len)
{
    int i;
    for (i = 0; i < len; i+=2) {
        unsigned delay = 0;
        delay = set_bit_field(delay, SEQ1_DLY_CNT0, delays[i]);
        if (i+1 < len)
            delay = set_bit_field(delay, SEQ1_DLY_CNT1, delays[i+1]);
        adc_write_reg(ADC_SEQ1_DLY(i/2), delay);
    }
}

static void adc_hal_set_seq1_channels(unsigned char *channels, int len)
{
    int i, n;
    unsigned int seq1_cnr0 = 0;
    for (i = 0, n = 0; i < 8 && i < len; i++, n+=4) {
        seq1_cnr0 = set_bit_field(seq1_cnr0, n, n+3, channels[i]);
    }
    adc_write_reg(ADC_SEQ1_CNR0, seq1_cnr0);

    unsigned int seq1_cnr1= 0;
    for (i = 8, n = 0; i < len; i++, n+=4) {
        seq1_cnr1 = set_bit_field(seq1_cnr1, n, n+3, channels[i]);
    }
    adc_write_reg(ADC_SEQ1_CNR1, seq1_cnr1);
}

static void adc_hal_set_seq1_CR_register(
    enum adc_trigger_type trigger, int channel_cnt, int enable_channel_num)
{
    unsigned int seq1_cr = adc_read_reg(ADC_SEQ1_CR);
    seq1_cr = set_bit_field(seq1_cr, SEQ1_STORAGE0, 31, 0xffff);
    seq1_cr = set_bit_field(seq1_cr, SEQ1_LEN, channel_cnt-1);
    if (trigger == adc_trigger_software)
        seq1_cr = set_bit_field(seq1_cr, SEQ1_EXTSEL_software);
    else if (trigger >= adc_trigger_gpio_rising_edge) {
        seq1_cr = set_bit_field(seq1_cr, SEQ1_EXTSEL_gpio);
        seq1_cr = set_bit_field(seq1_cr, SEQ1_EXT_TCU_CH_SEL, trigger-adc_trigger_gpio_rising_edge);
    } else {
        seq1_cr = set_bit_field(seq1_cr, SEQ1_EXTSEL_tcu1);
        seq1_cr = set_bit_field(seq1_cr, SEQ1_EXT_TCU_CH_SEL, trigger);
    }

    seq1_cr = set_bit_field(seq1_cr, SEQ1_CH_NUM_EN, enable_channel_num);
    adc_write_reg(ADC_SEQ1_CR, seq1_cr);
}


static void adc_hal_set_seq2_delay_clk_div(unsigned int div)
{
    if (div == 0)
        div = 1;

    adc_set_bits(ADC_CLKR2, SEQ2_DLYCLK_DIV, div-1);
}

static unsigned int seq2_delay(unsigned char *delays, int len)
{
    unsigned delay = 0;
    if (len >= 1)
        delay = set_bit_field(delay, SEQ2_DLY_CNT04, delays[0]);
    if (len >= 2)
        delay = set_bit_field(delay, SEQ2_DLY_CNT15, delays[1]);
    if (len >= 3)
        delay = set_bit_field(delay, SEQ2_DLY_CNT26, delays[2]);
    if (len >= 4)
        delay = set_bit_field(delay, SEQ2_DLY_CNT37, delays[3]);
    return delay;
}

static void adc_hal_set_seq2_delay(unsigned char *delays, int len)
{
    if (len <= 4) {
        adc_write_reg(ADC_SEQ2_DLY(0), seq2_delay(delays, len));
    } else {
        adc_write_reg(ADC_SEQ2_DLY(0), seq2_delay(delays, 4));
        adc_write_reg(ADC_SEQ2_DLY(1), seq2_delay(delays+4, len-4));
    }
}

static void adc_hal_set_seq2_channels(unsigned char *channels, int len)
{
    int i, n;
    unsigned int seq2_cnr0 = 0;
    for (i = 0, n = 0; i < len; i++, n+=4) {
        seq2_cnr0 = set_bit_field(seq2_cnr0, n, n+3, channels[i]);
    }
    adc_write_reg(ADC_SEQ2_CNR0, seq2_cnr0);
}

static void adc_hal_set_seq2_CR_register(
    enum adc_trigger_type trigger, int channel_cnt, int enable_channel_num)
{
    unsigned int seq2_cr = adc_read_reg(ADC_SEQ2_CR);
    seq2_cr = set_bit_field(seq2_cr, SEQ2_STORAGE0, 23, 0xff);
    seq2_cr = set_bit_field(seq2_cr, SEQ2_LEN, channel_cnt-1);
    if (trigger == adc_trigger_software)
        seq2_cr = set_bit_field(seq2_cr, SEQ2_EXTSEL_software);
    else if (trigger >= adc_trigger_gpio_rising_edge) {
        seq2_cr = set_bit_field(seq2_cr, SEQ2_EXTSEL_gpio);
        seq2_cr = set_bit_field(seq2_cr, SEQ2_EXT_TCU_CH_SEL, trigger-adc_trigger_gpio_rising_edge);
    } else {
        seq2_cr = set_bit_field(seq2_cr, SEQ2_EXTSEL_tcu1);
        seq2_cr = set_bit_field(seq2_cr, SEQ2_EXT_TCU_CH_SEL, trigger);
    }

    seq2_cr = set_bit_field(seq2_cr, SEQ2_CH_NUM_EN, enable_channel_num);
    adc_write_reg(ADC_SEQ2_CR, seq2_cr);
}
