#include <bit_field.h>
#include "aic_regs.h"
#include <utils/clock.h>

#define AIC_IOBASE 0x10020000

#define AIC_REG_BASE  KSEG1ADDR(AIC_IOBASE)

#define AIC_ADDR(reg) ((volatile unsigned long *)(AIC_REG_BASE + (reg)))

void aic_write_reg(unsigned int reg, int val)
{
    *AIC_ADDR(reg) = val;
}

unsigned int aic_read_reg(unsigned int reg)
{
    return *AIC_ADDR(reg);
}

void aic_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(AIC_ADDR(reg), start, end, val);
}

unsigned int aic_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(AIC_ADDR(reg), start, end);
}

static inline void aic_dump(void)
{
    printk("=============aic_dump=============\n");
    printk("(0x%02x)AICFR: 0x%08x\n", AICFR, aic_read_reg(AICFR));
    printk("(0x%02x)AICCR: 0x%08x\n", AICCR, aic_read_reg(AICCR));
    printk("(0x%02x)I2SCR: 0x%08x\n", I2SCR, aic_read_reg(I2SCR));
    printk("(0x%02x)AICSR: 0x%08x\n", AICSR, aic_read_reg(AICSR));
    printk("(0x%02x)I2SSR: 0x%08x\n", I2SSR, aic_read_reg(I2SSR));
    printk("(0x%02x)I2SDIV: 0x%08x\n", I2SDIV, aic_read_reg(I2SDIV));
    printk("(0x%02x)AICDR: 0x%08x\n", AICDR, aic_read_reg(AICDR));
    printk("(0x%02x)AICLR: 0x%08x\n", AICLR, aic_read_reg(AICLR));
    printk("(0x%02x)AICFR2: 0x%08x\n", AICFR2, aic_read_reg(AICFR2));
    printk("(0x%02x)CDCCR: 0x%08x\n", CDCCR, aic_read_reg(CDCCR));
    printk("==================================\n");
}

static unsigned int sub_pos(unsigned int size, unsigned int pos, unsigned int delta)
{
    return (pos + size - delta) % size;
}

static unsigned int add_pos(unsigned int size, unsigned int pos, unsigned int delta)
{
    return (pos + delta) % size;
}

static int to_sample_bits(unsigned int fmt)
{
    if (fmt == SNDRV_PCM_FORMAT_S8) return 0;
    if (fmt == SNDRV_PCM_FORMAT_S16_LE) return 1;
    if (fmt == SNDRV_PCM_FORMAT_S24_LE) return 4;
    return 0;
}

static void aic_reset_div(struct aic_data *data)
{
    unsigned int div = aic_dev.clk_div / 64;
    unsigned int mdiv, tdiv;

    if (aic_dev.as_master) {
        tdiv = div;
        if (tdiv % 2)
            tdiv--;

        aic_set_bit(I2SDIV, TDIV, tdiv);
    }
    if (aic_dev.clk_id == SELECT_INNER_CODEC) {
        mdiv = div / 4;
        if (mdiv % 2)
            mdiv--;

        aic_set_bit(I2SDIV, MDIV, mdiv);
    } else
         aic_set_bit(I2SDIV, MDIV, 0);

}

void aic_hal_init_common_setting(struct aic_data *data)
{
    aic_reset_div(data);

    /* reset aic
     */
    unsigned long aicfr = aic_read_reg(AICFR);
    set_bit_field(&aicfr, TFTH, (TX_FIFO_TRIGGER - 1) / 2);
    set_bit_field(&aicfr, RFTH, (RX_FIFO_TRIGGER - 1) / 2);
    set_bit_field(&aicfr, TRST, 1);
    set_bit_field(&aicfr, RRST, 1);
    set_bit_field(&aicfr, MSB, 0);
    set_bit_field(&aicfr, LSMP, 1);
    set_bit_field(&aicfr, ICDC, aic_dev.clk_id == SELECT_INNER_CODEC);
    set_bit_field(&aicfr, AUSEL, 1);
    set_bit_field(&aicfr, TMASTER, 1);
    set_bit_field(&aicfr, ENB, 0);
    aic_write_reg(AICFR, aicfr);

    /* 等待aic reset 完成
     */
    int i = 1000;
    while (i--) {
        if (!aic_get_bit(AICFR, TRST) && !aic_get_bit(AICFR, RRST))
            break;
        else if (i == 0)
            panic("AIC: reset aic failure\n");
    }
    aic_set_bit(AICFR, TMASTER, aic_dev.clk_id == SELECT_EXT_CODEC && aic_dev.as_master);

    unsigned long aiccr = aic_read_reg(AICCR);
    set_bit_field(&aiccr, ETFLOR, 0);
    set_bit_field(&aiccr, ETFLS, 0);
    set_bit_field(&aiccr, ETFL, 0);
    set_bit_field(&aiccr, TLDMS, 0);
    set_bit_field(&aiccr, RDMS, 0);
    set_bit_field(&aiccr, TDMS, 0);
    set_bit_field(&aiccr, ENDSW, 0);
    set_bit_field(&aiccr, ASVTSU, 0);
    set_bit_field(&aiccr, EROR, 0);
    set_bit_field(&aiccr, ETUR, 0);
    set_bit_field(&aiccr, ERFS, 0);
    set_bit_field(&aiccr, ETFS, 0);
    set_bit_field(&aiccr, ENLBF, 0);
    set_bit_field(&aiccr, ERPL, 0);
    set_bit_field(&aiccr, EREC, 0);
    aic_write_reg(AICCR, aiccr);

    aic_set_bit(AICSR, ROR, 0);
    aic_set_bit(AICSR, TUR, 0);

    unsigned long i2scr = aic_read_reg(I2SCR);
    set_bit_field(&i2scr, FstChnlLeft, 1);
    set_bit_field(&i2scr, RFIRST, 0);
    set_bit_field(&i2scr, SWLH, 0);
    set_bit_field(&i2scr, AMSL, aic_dev.interface);
    aic_write_reg(I2SCR, i2scr);
}

void aic_init_capture_setting(struct aic_data *data)
{
    int channels = data->channels;
    unsigned long aiccr = aic_read_reg(AICCR);
    set_bit_field(&aiccr, ISS, to_sample_bits(data->format));
    set_bit_field(&aiccr, MONOCTR, channels == 2 ? 0 : 2);//if one channel,only left
    aic_write_reg(AICCR, aiccr);
}

void aic_init_playback_setting(struct aic_data *data)
{
    int is_16bit = (data->format == SNDRV_PCM_FORMAT_S16_LE);
    int channels = data->channels;

    unsigned long aiccr = aic_read_reg(AICCR);
    set_bit_field(&aiccr, PACK16, is_16bit && channels == 2);
    set_bit_field(&aiccr, CHANNEL, channels == 2);
    set_bit_field(&aiccr, OSS, to_sample_bits(data->format));
    aic_write_reg(AICCR, aiccr);
}

static void aic_enable(void)
{
    if (aic_dev.is_enabled++ == 0)
        aic_set_bit(AICFR, ENB, 1);
}

static void aic_disable(void)
{
    if (--aic_dev.is_enabled == 0)
        aic_set_bit(AICFR, ENB, 0);
}

static void aic_hal_start_capture(void)
{
    aic_enable();

    /*
     * 清空 rx fifo 数据
     * 开启接收功能,接收dma
     */
    aic_set_bit(AICCR, RFLUSH, 1);

    aic_set_bit(AICCR, EREC, 1);

    uint64_t old = local_clock_us();
    while (!aic_get_bit(AICSR, RFL)) {
        if (local_clock_us() - old > 10000) {
            if (!aic_get_bit(AICSR, RFL)) {
                printk(KERN_ERR "AIC: failed to wait rx fifo empty: %x\n", aic_read_reg(AICSR));
                return;
            }
        }
    }

    aic_set_bit(AICCR, RDMS, 1);
}

static void aic_hal_stop_capture(void)
{
    aic_set_bit(AICCR, RDMS, 0);
    aic_set_bit(AICCR, EREC, 0);

    aic_set_bit(AICCR, RFLUSH, 1);

    aic_disable();
}

static void aic_hal_start_playback(void)
{
    aic_enable();

    /*
     * 清空 tx fifo 数据
     * 使能播放dma功能
     */
    aic_set_bit(AICCR, ERPL, 1);

    int i;
    for (i = 0; i < 32; i++) {
        int n = aic_get_bit(AICSR, TFL);
        if (n > 32)
            break;
        aic_write_reg(AICDR, 0);
    }

    aic_set_bit(AICSR, TUR, 0);
    aic_set_bit(AICCR, TFLUSH, 1);

    int timeout = 15;
    while (aic_get_bit(AICSR, TFL)) {
        mdelay(10);
        if (timeout-- == 0) {
            printk(KERN_ERR "AIC: failed to wait tx fifo empty: %x\n", aic_read_reg(AICSR));
            return;
        }
    }

    aic_set_bit(AICCR, TDMS, 1);
}

static void aic_hal_stop_playback(void)
{
    aic_set_bit(AICCR, TDMS, 0);
    aic_set_bit(AICCR, ERPL, 0);

    aic_set_bit(AICCR, TFLUSH, 1);

    int timeout = 15;
    while (aic_get_bit(AICSR, TFL)) {
        mdelay(1);
        if (timeout-- == 0) {
            printk(KERN_ERR "AIC: failed to wait tx fifo empty: %x\n", aic_read_reg(AICSR));
            return;
        }
    }

    aic_disable();
}