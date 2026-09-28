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
    printk("(0x%x)AICFR: 0x%08x\n", AICFR, aic_read_reg(AICFR));
    printk("(0x%x)AICCR: 0x%08x\n", AICCR, aic_read_reg(AICCR));
    printk("(0x%x)I2SCR: 0x%08x\n", I2SCR, aic_read_reg(I2SCR));
    printk("(0x%x)AICSR: 0x%08x\n", AICSR, aic_read_reg(AICSR));
    printk("(0x%x)I2SSR: 0x%08x\n", I2SSR, aic_read_reg(I2SSR));
    printk("(0x%x)I2SDIV: 0x%08x\n", I2SDIV, aic_read_reg(I2SDIV));
    printk("(0x%x)AICDR: 0x%08x\n", AICDR, aic_read_reg(AICDR));
    printk("(0x%x)AICLR: 0x%08x\n", AICLR, aic_read_reg(AICLR));
    printk("(0x%x)AICTFLR: 0x%08x\n", AICTFLR, aic_read_reg(AICTFLR));
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

void aic_hal_init_common_setting(struct aic_data *data)
{
    aic_set_bit(AICFR, ENB, 0);
    schedule_timeout(5);
    aic_set_bit(AICFR, ENB, 0);
    aic_set_bit(AICFR, BCKD, 1);
    aic_set_bit(AICFR, SYNCD, 1);

    /* reset aic
     */
    aic_set_bit(AICFR, RST, 1);

    /* 等待aic reset 完成
     */
    int i = 1000;
    while (i--) {
        if (!aic_get_bit(AICFR, RST_O_STATUS) && !aic_get_bit(AICFR, RST_I_STATUS))
            break;
        else if (i == 0)
            panic("AIC: reset aic failure\n");
    }

    aic_set_bit(AICFR, AUSEL, 1);

    aic_set_bit(AICFR, ICDC, aic_dev.clk_id == SELECT_INNER_CODEC);
    aic_set_bit(AICFR, BCKD, aic_dev.as_master);
    aic_set_bit(AICFR, SYNCD, aic_dev.as_master);

    if (aic_dev.clk_id == SELECT_EXT_CODEC) {
        aic_set_bit(AICFR, DMODE, aic_select_clk != fully_shared);
        aic_set_bit(I2SCR, AMSL, aic_dev.interface);
    } else {
        aic_set_bit(AICFR, DMODE, 0);
        aic_set_bit(I2SCR, AMSL, 0);
    }

    unsigned long aiccr = aic_read_reg(AICCR);
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
    aic_set_bit(AICFR, TFTH, TX_FIFO_TRIGGER / 2);
    aic_set_bit(AICFR, RFTH, RX_FIFO_TRIGGER / 2 - 1);

    aic_set_bit(AICTFLR, 0, 3, 8);

    aic_set_bit(I2SCR, RFIRST, 0);
    aic_set_bit(AICFR, LSMP, 1);
}

void aic_init_capture_setting(struct aic_data *data)
{
    int channels = data->channels;
    unsigned long aiccr = aic_read_reg(AICCR);
    set_bit_field(&aiccr, ISS, to_sample_bits(data->format));
    set_bit_field(&aiccr, MONOCTR, channels == 2 ? 0 : 2);//if one channel,only left
    set_bit_field(&aiccr, ASVTSU, 0);
    aic_write_reg(AICCR, aiccr);
}

void aic_init_playback_setting(struct aic_data *data)
{
    int is_16bit = (data->format == SNDRV_PCM_FORMAT_S16_LE);
    int channels = data->channels;

    unsigned long aiccr = aic_read_reg(AICCR);
    set_bit_field(&aiccr, PACK16, is_16bit && channels == 2);
    set_bit_field(&aiccr, CHANNEL, channels == 2);
    set_bit_field(&aiccr, M2S, channels == 1);
    set_bit_field(&aiccr, OSS, to_sample_bits(data->format));
    set_bit_field(&aiccr, ENDSW, 0);
    set_bit_field(&aiccr, ASVTSU, 0);
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