#include <bit_field.h>
#include "icodec_regs.h"

void icodec_write_reg(unsigned int reg, int val)
{
    *ICODEC_ADDR(reg) = val;
}

unsigned int icodec_read_reg(unsigned int reg)
{
    return *ICODEC_ADDR(reg);
}

void icodec_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(ICODEC_ADDR(reg), start, end, val);
}

unsigned int icodec_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(ICODEC_ADDR(reg), start, end);
}

static inline void dump_regs(void)
{
    printk("(0x%03x): CGR:        0x%08x\n", CGR, icodec_read_reg(CGR));
    printk("(0x%03x): CDCFGR:     0x%08x\n", CDCFGR, icodec_read_reg(CDCFGR));
    printk("(0x%03x): CACR:       0x%08x\n", CACR, icodec_read_reg(CACR));
    printk("(0x%03x): CACR1:      0x%08x\n", CACR1, icodec_read_reg(CACR1));
    printk("(0x%03x): CDCR:       0x%08x\n", CDCR, icodec_read_reg(CDCR));
    printk("(0x%03x): CDCR1:      0x%08x\n", CDCR1, icodec_read_reg(CDCR1));
    printk("(0x%03x): CDGSR:      0x%08x\n", CDGSR, icodec_read_reg(CDGSR));
    printk("(0x%03x): CDLCBMSR:   0x%08x\n", CDLCBMSR, icodec_read_reg(CDLCBMSR));
    printk("(0x%03x): CLAVR:      0x%08x\n", CLAVR, icodec_read_reg(CLAVR));
    printk("(0x%03x): CGAINR:     0x%08x\n", CGAINR, icodec_read_reg(CGAINR));
    printk("(0x%03x): CDBCR:      0x%08x\n", CDBCR, icodec_read_reg(CDBCR));
    printk("(0x%03x): CCR:        0x%08x\n", CCR, icodec_read_reg(CCR));
    printk("(0x%03x): CAACR:      0x%08x\n", CAACR, icodec_read_reg(CAACR));
    printk("(0x%03x): CMICCR:     0x%08x\n", CMICCR, icodec_read_reg(CMICCR));
    printk("(0x%03x): CMICGAINR:  0x%08x\n", CMICGAINR, icodec_read_reg(CMICGAINR));
    printk("(0x%03x): CAEC:       0x%08x\n", CAEC, icodec_read_reg(CAEC));
    printk("(0x%03x): CALCGR:     0x%08x\n", CALCGR, icodec_read_reg(CALCGR));
    printk("(0x%03x): CANACR:     0x%08x\n", CANACR, icodec_read_reg(CANACR));
    printk("(0x%03x): CANACR1:    0x%08x\n", CANACR1, icodec_read_reg(CANACR1));
    printk("(0x%03x): CHR:        0x%08x\n", CHR, icodec_read_reg(CHR));
    printk("(0x%03x): CHPOUTLGR:  0x%08x\n", CHPOUTLGR, icodec_read_reg(CHPOUTLGR));
    printk("(0x%03x): CMR:        0x%08x\n", CMR, icodec_read_reg(CMR));
    printk("(0x%03x): CTR:        0x%08x\n", CTR, icodec_read_reg(CTR));
    printk("(0x%03x): CAGCCR:     0x%08x\n", CAGCCR, icodec_read_reg(CAGCCR));
    printk("(0x%03x): CPGR:       0x%08x\n", CPGR, icodec_read_reg(CPGR));
    printk("(0x%03x): CSRR:       0x%08x\n", CSRR, icodec_read_reg(CSRR));
    printk("(0x%03x): CALMAXR:    0x%08x\n", CALMAXR, icodec_read_reg(CALMAXR));
    printk("(0x%03x): CAHMAXR:    0x%08x\n", CAHMAXR, icodec_read_reg(CAHMAXR));
    printk("(0x%03x): CALMINR:    0x%08x\n", CALMINR, icodec_read_reg(CALMINR));
    printk("(0x%03x): CAHMINR:    0x%08x\n", CAHMINR, icodec_read_reg(CAHMINR));
    printk("(0x%03x): CAFR:       0x%08x\n", CAFR, icodec_read_reg(CAFR));
}

void icodec_power_on(void)
{
    // icodec reset
    icodec_set_bit(CGR, SYS_RSTN, 0);
    usleep_range(1*1000, 1*1000);
    icodec_set_bit(CGR, SYS_RSTN, 1);
    icodec_set_bit(CGR, DIGCORE_RSTN, 1);
    usleep_range(1*1000, 1*1000);

    icodec_set_bit(CANACR1, POP_CTRL_DACL, 1);
    usleep_range(1*1000, 1*1000);

    icodec_set_bit(CCR, SEL_VREF, 1);
    usleep_range(1*1000, 1*1000);

    icodec_set_bit(CDBCR, 6, 6, 1);
    usleep_range(1*1000, 1*1000);

    int i;
    char value = 0;
    for (i = 0; i <= 7; i++) {
        value |= value << 1 | 0x01;
        icodec_set_bit(CCR, SEL_VREF, value);
        usleep_range(20*1000, 20*1000);
    }
    icodec_set_bit(CAACR, EN_VREF, 0x1);
    usleep_range(10*1000, 10*1000);
    icodec_set_bit(CCR, SEL_VREF, 0x02);
    usleep_range(10*1000, 10*1000);
}

void icodec_power_off(void)
{
    // discharge
    icodec_set_bit(CCR, SEL_VREF, 0x01);
    usleep_range(1*1000, 1*1000);
    icodec_set_bit(CDBCR, 6, 6, 0);
    usleep_range(1*1000, 1*1000);

    int i;
    char value = 0;
    for (i = 0; i <= 7; i++) {
        value |= value<<1 | 0x01;
        icodec_set_bit(CCR, SEL_VREF, value);
        usleep_range(20*1000, 20*1000);
    }
}

void icodec_enable_dac(int data_bits, int is_mute, int vol)
{
    int len = 0;

    if (data_bits == 32) len = 3;
    if (data_bits == 24) len = 2;
    if (data_bits == 20) len = 1;
    if (data_bits == 16) len = 0;

    unsigned long cdcr = icodec_read_reg(CDCR);
    set_bit_field(&cdcr, I2S_RX_LRP, 0);
    set_bit_field(&cdcr, I2S_RX_WL, len);
    set_bit_field(&cdcr, I2S_RX_FMT, 2); // Choose DAC I2S interface mode
    set_bit_field(&cdcr, I2S_LR_SWAP, 1);
    icodec_write_reg(CDCR, cdcr);

    icodec_set_bit(CACR1, I2S_RX_PIN_MST, 1); // Choose DAC I2S Master Mode
    icodec_set_bit(CACR1, I2S_RX_FUN_MST, 1); // Choose DAC I2S Master Mode

    icodec_set_bit(CANACR1, EN_IBIAS_DAC, 1);
    icodec_set_bit(CANACR1, EN_BUF_DACL, 1);
    icodec_set_bit(CANACR1, POP_CTRL_DACL, 2);
    icodec_set_bit(CHR, EN_HPOUTL, 1);
    icodec_set_bit(CHR, INITIAL_HPOUTL, 1);
    icodec_set_bit(CANACR1, EN_VREF_DACL, 1);
    icodec_set_bit(CANACR1, EN_CLK_DACL, 1);
    icodec_set_bit(CANACR1, EN_DACL, 1);
    usleep_range(10*1000, 10*1000);

    icodec_set_bit(CANACR1, INITIAL_DACL, 1);
    icodec_set_bit(CHR, MUTE_HPOUTL, !is_mute);
    icodec_set_bit(CDGSR, DAC_VOL, !is_mute ? 0xf1 : 0);
    icodec_set_bit(CHPOUTLGR, GAIN_HPOUTL, vol); // 0x18
    icodec_set_bit(CCR, SEL_VREF, 0x01);
}

void icodec_disable_dac(void)
{
    icodec_set_bit(CHPOUTLGR, GAIN_HPOUTL, 0);
    icodec_set_bit(CHR, MUTE_HPOUTL, 0);
    icodec_set_bit(CHR, INITIAL_HPOUTL, 0);
    icodec_set_bit(CHR, EN_HPOUTL, 0);
    icodec_set_bit(CANACR1, EN_DACL, 0);
    icodec_set_bit(CANACR1, EN_CLK_DACL, 0);
    icodec_set_bit(CANACR1, EN_VREF_DACL, 0);
    icodec_set_bit(CANACR1, POP_CTRL_DACL, 1);
    icodec_set_bit(CANACR1, EN_BUF_DACL, 0);
    icodec_set_bit(CANACR1, EN_IBIAS_DAC, 0);
    icodec_set_bit(CANACR1, INITIAL_DACL, 0);
}

void icodec_enable_adc(int data_bits, int bias_enable, int bias_level, int vol)
{
    int len = 0;

    if (data_bits == 32) len = 3;
    if (data_bits == 24) len = 2;
    if (data_bits == 20) len = 1;
    if (data_bits == 16) len = 0;

    unsigned long cacr1 = icodec_read_reg(CACR1);
    set_bit_field(&cacr1, I2S_TX_PIN_MST, 1); // Choose ADC I2S Master Mode
    set_bit_field(&cacr1, I2S_TX_FUN_MST, 1); // Choose ADC I2S Master Mode
    set_bit_field(&cacr1, I2S_TX_LEN, 1);
    set_bit_field(&cacr1, I2S_TX_RSTN, 1);
    set_bit_field(&cacr1, I2S_TX_BCLKINV, 0);
    icodec_write_reg(CACR1, cacr1);

    unsigned long cacr = icodec_read_reg(CACR);
    set_bit_field(&cacr, I2S_TX_LRP, 0);
    set_bit_field(&cacr, I2S_TX_WL, len);
    set_bit_field(&cacr, I2S_TX_FMT, 2);
    set_bit_field(&cacr, I2S_TX_DATSEL, 0);
    icodec_write_reg(CACR, cacr);

    icodec_set_bit(CMICCR, MUTE_MICL, 1);
    icodec_set_bit(CAACR, EN_IBIAS_ADC, 1);
    icodec_set_bit(CAACR, EN_MICBIAS, bias_enable);
    icodec_set_bit(CAACR, GAIN_MICBIAS, bias_level);
    icodec_set_bit(CMICCR, EN_BUF_ADCL, 1);
    icodec_set_bit(CAEC, EN_MICL, 1);
    icodec_set_bit(CAEC, EN_ALCL, 1);
    icodec_set_bit(CANACR, EN_CLK_ADCL, 1);
    icodec_set_bit(CANACR, EN_ADCL, 1);
    usleep_range(10*1000, 10*1000);
    icodec_set_bit(CANACR, INITIAL_ADCL, 1);
    icodec_set_bit(CANACR, INITIAL_ALCL, 1);
    icodec_set_bit(CMICCR, INITIAL_MICL, 1);
    icodec_set_bit(CMICGAINR, GAIN_MICL, 0x3);
    icodec_set_bit(CALCGR, GAIN_ALCL, vol);
    icodec_set_bit(CMICCR, EN_ZERODET_ADCL, 1);
    icodec_set_bit(CPGR, 0, 5, 0x3f);
    icodec_set_bit(CMICGAINR, SEL_IBIAS_ADC, 3);
    usleep_range(1*1000, 1*1000);
}

void icodec_disable_adc(void)
{
    icodec_set_bit(CMICCR, EN_ZERODET_ADCL, 0);
    icodec_set_bit(CANACR, EN_ADCL, 0);
    icodec_set_bit(CANACR, EN_CLK_ADCL, 0);
    icodec_set_bit(CAEC, EN_ALCL, 0);
    icodec_set_bit(CAEC, EN_MICL, 0);
    icodec_set_bit(CMICCR, EN_BUF_ADCL, 0);
    icodec_set_bit(CAACR, EN_IBIAS_ADC, 0);
    icodec_set_bit(CANACR, INITIAL_ADCL, 0);
    icodec_set_bit(CANACR, INITIAL_ALCL, 0);
    icodec_set_bit(CMICCR, INITIAL_MICL, 0);
}

int icodec_playback_pcm_set_mute(int mute)
{
    icodec_set_bit(CHR, MUTE_HPOUTL, !mute);
    icodec_set_bit(CDGSR, DAC_VOL, !mute ? 0xf1 : 0);

    return 0;
}

int icodec_playback_pcm_get_mute(void)
{
    return !icodec_get_bit(CHR, MUTE_HPOUTL);
}

int icodec_playback_pcm_set_volume(int val)
{
    icodec_set_bit(CHPOUTLGR, GAIN_HPOUTL, val & 0x1f);

    return 0;
}

int icodec_playback_pcm_get_volume(void)
{
    return icodec_get_bit(CHPOUTLGR, GAIN_HPOUTL);
}

int icodec_capture_pcm_set_volume(int val)
{
    icodec_set_bit(CAFR, AGC_FUN_SEL, 0);
    usleep_range(1*1000, 1*1000);
    icodec_set_bit(CGAINR, ALCL_EN, 0);
    usleep_range(1*1000, 1*1000);
    icodec_set_bit(CALCGR, GAIN_ALCL, val);
    usleep_range(1*1000, 1*1000);
    return 0;
}

int icodec_capture_pcm_get_volume(void)
{
    return icodec_get_bit(CALCGR, GAIN_ALCL);
}
