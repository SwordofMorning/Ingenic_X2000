#include <bit_field.h>
#include "icodec_regs.h"

static inline void icodec_write_reg(unsigned int reg, int val)
{
    *ICODEC_ADDR(reg) = val;
}

static inline unsigned int icodec_read_reg(unsigned int reg)
{
    return *ICODEC_ADDR(reg);
}

static inline void icodec_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(ICODEC_ADDR(reg), start, end, val);
}

static inline unsigned int icodec_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(ICODEC_ADDR(reg), start, end);
}

void icodec_power_on(void)
{
    icodec_set_bit(RSTR, SYSRST, 0);
    icodec_set_bit(RSTR, DACRST, 0);
    icodec_set_bit(RSTR, ADCRST, 0);
    usleep_range(10*1000, 10*1000);

    icodec_set_bit(RSTR, SYSRST, 1);
    icodec_set_bit(RSTR, DACRST, 1);
    icodec_set_bit(RSTR, ADCRST, 1);

    icodec_set_bit(DACLCR, POPCTRL, 1);
    icodec_set_bit(BIASCR2, SELVREF, 0x01);

    icodec_set_bit(RSTR, SYSRST, 1);
    icodec_set_bit(RSTR, DACRST, 1);
    icodec_set_bit(RSTR, ADCRST, 1);

    icodec_set_bit(BIASCR1, VREFEN, 1);
    icodec_set_bit(BIASCR2, SELVREF, 0xff);

    usleep_range(20*1000, 20*1000);
    usleep_range(20*1000, 20*1000);

    icodec_set_bit(BIASCR2, SELVREF, 0x02);
}

void icodec_power_off(void)
{
    icodec_set_bit(BIASCR2, SELVREF, 0x01);

    icodec_set_bit(BIASCR1, VREFEN, 0);

    icodec_set_bit(BIASCR2, SELVREF, 0xff);

    usleep_range(20*1000, 20*1000);
    usleep_range(20*1000, 20*1000);
}

void icodec_enable_dac(int is_mute, int dgain, int hpl_gain)
{
    icodec_set_bit(BIASCR1, DACIBIASEN, 1);

    icodec_set_bit(DACLCR, DACLBUFEN, 1);

    icodec_set_bit(DACLCR, POPCTRL, 2);

    icodec_set_bit(HPCR, HPLEN, 1);

    icodec_set_bit(HPCR, HPLINITIAL, 1);

    icodec_set_bit(DACLCR, DACLVREFEN, 1);

    icodec_set_bit(DACLCR, DACLCLKEN, 1);

    icodec_set_bit(DACLCR, DACLEN, 1);

    icodec_set_bit(DACLCR, DACLINITIAL, 1);

    icodec_set_bit(HPCR, HPLMUTE, !is_mute);

    icodec_set_bit(HPGR, HPLGAIN, hpl_gain);

    icodec_set_bit(DACDGR, DAC_DIGGAIN, dgain);
}

void icodec_disable_dac(void)
{
    icodec_set_bit(HPGR, HPLGAIN, 0);
    icodec_set_bit(HPCR, HPLMUTE, 0);
    icodec_set_bit(HPCR, HPLINITIAL, 0);
    icodec_set_bit(HPCR, HPLEN, 0);
    icodec_set_bit(DACLCR, DACLEN, 0);
    icodec_set_bit(DACLCR, DACLCLKEN, 0);
    icodec_set_bit(DACLCR, DACLVREFEN, 0);
    icodec_set_bit(DACLCR, POPCTRL, 1);
    icodec_set_bit(DACLCR, DACLBUFEN, 0);
    icodec_set_bit(BIASCR1, DACIBIASEN, 0);
    icodec_set_bit(DACLCR, DACLINITIAL, 0);
}

void icodec_enable_adc(int bias_enable, int bias_level)
{
    icodec_set_bit(BIASCR1, ADCIBIASEN, 1);
    icodec_set_bit(BIASCR1, MICBIASEN, bias_enable);
    icodec_set_bit(BIASCR1, MICBIASGAIN, bias_level);

    icodec_set_bit(ADCLCR, ADCLBUFEN, 1);

    icodec_set_bit(MICCR, MICLEN, 1);

    icodec_set_bit(MICCR, ALCLEN, 1);

    icodec_set_bit(ADCLCR, ADCLCLKEN, 1);

    icodec_set_bit(ADCLCR, ADCLEN, 1);

    icodec_set_bit(ADCLCR, ADCLINITIAL, 1);

    icodec_set_bit(MICCR, ALCLINITIAL, 1);

    icodec_set_bit(MICCR, MICLINITIAL, 1);
}

void icodec_enable_adc1(int dgain, int mic_in_gain, int alcl_gain)
{
    icodec_set_bit(MICCR, MICLMUTE, 1);

    icodec_set_bit(MICGR, MICLGAIN, mic_in_gain);

    icodec_set_bit(ALCGR, ALCLGAIN, alcl_gain);

    icodec_set_bit(ADCLCR, ZERODETEN, 1);

    icodec_set_bit(ADCDGR, ADC_DIGGAIN, dgain);
}

void icodec_disable_adc(void)
{
    icodec_set_bit(ADCLCR, ZERODETEN, 0);
    icodec_set_bit(ADCLCR, ADCLEN, 0);
    icodec_set_bit(ADCLCR, ADCLCLKEN, 0);
    icodec_set_bit(MICCR, ALCLEN, 0);
    icodec_set_bit(MICCR, MICLEN, 0);
    icodec_set_bit(ADCLCR, ADCLBUFEN, 0);
    icodec_set_bit(BIASCR1, ADCIBIASEN, 0);
    icodec_set_bit(BIASCR1, MICBIASEN, 0);
    icodec_set_bit(ADCLCR, ADCLINITIAL, 0);
    icodec_set_bit(MICCR, ALCLINITIAL, 0);
    icodec_set_bit(MICCR, MICLINITIAL, 0);
}

static void icodec_config_adc(int data_bits, int channel)
{
    int len = 0;

    if (data_bits == 24) len = 2;
    else if (data_bits == 20) len = 1;
    else if (data_bits == 16) len = 0;

    icodec_set_bit(ADCCR1, ADC_I2SDATSEL, channel == 1 ? 0 : 1); // set mono_left when one channel
    icodec_set_bit(ADCCR1, ADC_I2SWL, len);
    icodec_set_bit(ADCCR1, ADC_I2SFMT, 0x2); // i2s mode
    icodec_set_bit(ADCCR2, ADC_FUNCMSTEN, 1);
    icodec_set_bit(ADCCR2, ADC_PINMSTEN, 1);
    icodec_set_bit(ADCCR3, HPFMODE, 0x3);
}

static void icodec_config_dac(int data_bits, int samplerate)
{
    int len = 0, rate = 0;

    if (data_bits == 24) len = 2;
    else if (data_bits == 20) len = 1;
    else if (data_bits == 16) len = 0;

    if (samplerate == 96000) rate = 7;
    else if (samplerate == 88200) rate = 6;
    else if (samplerate == 48000) rate = 5;
    else if (samplerate == 44100) rate = 4;
    else if (samplerate == 32000) rate = 3;
    else if (samplerate == 24000) rate = 2;
    else if (samplerate == 16000) rate = 1;
    else if (samplerate == 8000) rate = 0;

    icodec_set_bit(DACCR2, DAC_PINMSTEN, 1);
    icodec_set_bit(DACCR2, DAC_FUNCMSTEN, 1);
    icodec_set_bit(DACCR2, 2, 2, 0);
    icodec_set_bit(DACCR1, DAC_I2SWL, len);
    icodec_set_bit(DACCR1, DAC_I2SFMT, 0x2); // i2s mode
    icodec_set_bit(DACCR3, MUTSR, rate);
}

static void icodec_set_adc_gain(int gain)
{
    icodec_set_bit(ADCDGR, ADC_DIGGAIN, gain);
}

static unsigned int icodec_get_adc_gain(void)
{
    return icodec_get_bit(ADCDGR, ADC_DIGGAIN);
}

static void icodec_set_dac_gain(int gain)
{
    icodec_set_bit(DACDGR, DAC_DIGGAIN, gain);
}

static unsigned int icodec_get_dac_gain(void)
{
    return icodec_get_bit(DACDGR, DAC_DIGGAIN);
}

static void icodec_set_dac_mute(int mute)
{
    icodec_set_bit(HPCR, HPLMUTE, !mute);
}

static int icodec_get_dac_mute(void)
{
    return !icodec_get_bit(HPCR, HPLMUTE);
}

static inline void dump_regs(void)
{
    printk("============================icodec dump=============================\n");
    printk("(0x%03x): RSTR:     0x%08x\n", RSTR, icodec_read_reg(RSTR));
    printk("(0x%03x): DACCR1:   0x%08x\n", DACCR1, icodec_read_reg(DACCR1));
    printk("(0x%03x): DACCR2:   0x%08x\n", DACCR2, icodec_read_reg(DACCR2));
    printk("(0x%03x): DACCR3:   0x%08x\n", DACCR3, icodec_read_reg(DACCR3));
    printk("(0x%03x): DACCR4:   0x%08x\n", DACCR4, icodec_read_reg(DACCR4));
    printk("(0x%03x): DACCR5:   0x%08x\n", DACCR5, icodec_read_reg(DACCR5));
    printk("(0x%03x): DACDGR:   0x%08x\n", DACDGR, icodec_read_reg(DACDGR));
    printk("(0x%03x): ADCCR1:   0x%08x\n", ADCCR1, icodec_read_reg(ADCCR1));
    printk("(0x%03x): ADCCR2:   0x%08x\n", ADCCR2, icodec_read_reg(ADCCR2));
    printk("(0x%03x): ADCCR3:   0x%08x\n", ADCCR3, icodec_read_reg(ADCCR3));
    printk("(0x%03x): ADCCR4:   0x%08x\n", ADCCR4, icodec_read_reg(ADCCR4));
    printk("(0x%03x): ADCDGR:   0x%08x\n", ADCDGR, icodec_read_reg(ADCDGR));
    printk("(0x%03x): BIASCR1:  0x%08x\n", BIASCR1, icodec_read_reg(BIASCR1));
    printk("(0x%03x): BIASCR2:  0x%08x\n", BIASCR2, icodec_read_reg(BIASCR2));
    printk("(0x%03x): BIASCR3:  0x%08x\n", BIASCR3, icodec_read_reg(BIASCR3));
    printk("(0x%03x): DACLCR:   0x%08x\n", DACLCR, icodec_read_reg(DACLCR));
    printk("(0x%03x): HPCR:     0x%08x\n", HPCR, icodec_read_reg(HPCR));
    printk("(0x%03x): HPDSR:    0x%08x\n", HPDSR, icodec_read_reg(HPDSR));
    printk("(0x%03x): HPGR:     0x%08x\n", HPGR, icodec_read_reg(HPGR));
    printk("(0x%03x): ADCLCR:   0x%08x\n", ADCLCR, icodec_read_reg(ADCLCR));
    printk("(0x%03x): MICCR:    0x%08x\n", MICCR, icodec_read_reg(MICCR));
    printk("(0x%03x): ALCGR:    0x%08x\n", ALCGR, icodec_read_reg(ALCGR));
    printk("(0x%03x): MICGR:    0x%08x\n", MICGR, icodec_read_reg(MICGR));
    printk("(0x%03x): AGCCR1:   0x%08x\n", AGCCR1, icodec_read_reg(AGCCR1));
    printk("(0x%03x): AGCCR2:   0x%08x\n", AGCCR2, icodec_read_reg(AGCCR2));
    printk("(0x%03x): AGCCR3:   0x%08x\n", AGCCR3, icodec_read_reg(AGCCR3));
    printk("(0x%03x): PGAGR:    0x%08x\n", PGAGR, icodec_read_reg(PGAGR));
    printk("(0x%03x): AGCSRR:   0x%08x\n", AGCSRR, icodec_read_reg(AGCSRR));
    printk("(0x%03x): AGCMAXLR: 0x%08x\n", AGCMAXLR, icodec_read_reg(AGCMAXLR));
    printk("(0x%03x): AGCMAXHR: 0x%08x\n", AGCMAXHR, icodec_read_reg(AGCMAXHR));
    printk("(0x%03x): AGCMINLR: 0x%08x\n", AGCMINLR, icodec_read_reg(AGCMINLR));
    printk("(0x%03x): AGCMINHR: 0x%08x\n", AGCMINHR, icodec_read_reg(AGCMINHR));
    printk("(0x%03x): AGCGR:    0x%08x\n", AGCGR, icodec_read_reg(AGCGR));
    printk("(0x%03x): ALCOGR:   0x%08x\n", ALCOGR, icodec_read_reg(ALCOGR));
    printk("====================================================================\n");
}
