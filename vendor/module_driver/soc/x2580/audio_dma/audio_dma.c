#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <bit_field.h>
#include <assert.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <sound/pcm_params.h>

#include "audio_regs.h"
#include "audio.h"

struct dmadesc {
    unsigned long cmd;
    unsigned long addr;
    unsigned long next_desc_off;
    unsigned long trans_count;
};

static inline void audio_dma_dump_regs(int dma_id)
{
    printk("=============audio_dump=============\n");
    printk(KERN_EMERG "dma_id: %d\n", dma_id);
    printk(KERN_EMERG "(%04x)DBA: %x\n", DBA, audio_read_reg(DBA));
    printk(KERN_EMERG "(%04x)DTC: %x\n", DTC, audio_read_reg(DTC));
    printk(KERN_EMERG "(%04x)DCR: %x\n", DCR, audio_read_reg(DCR));
    printk(KERN_EMERG "(%04x)DSR: %x\n", DSR, audio_read_reg(DSR));
    printk(KERN_EMERG "(%04x)DCM: %x\n", DCM, audio_read_reg(DCM));
    printk(KERN_EMERG "(%04x)DDA: %x\n", DDA, audio_read_reg(DDA));
    printk(KERN_EMERG "(%04x)DGRR: %x\n", DGRR, audio_read_reg(DGRR));
    printk(KERN_EMERG "(%04x)DCGR: %x\n", DCGR, audio_read_reg(DCGR));
    printk(KERN_EMERG "(%04x)AEER: %x\n", AEER, audio_read_reg(AEER));
    printk(KERN_EMERG "(%04x)AESR: %x\n", AESR, audio_read_reg(AESR));
    printk(KERN_EMERG "(%04x)AIPR: %x\n", AIPR, audio_read_reg(AIPR));

    printk(KERN_EMERG "(%04x)FAS: %x\n", FAS, audio_read_reg(FAS));
    printk(KERN_EMERG "(%04x)FCR: %x\n", FCR, audio_read_reg(FCR));
    printk(KERN_EMERG "(%04x)FFR: %x\n", FFR, audio_read_reg(FFR));
    printk(KERN_EMERG "(%04x)FSR: %x\n", FSR, audio_read_reg(FSR));

    printk(KERN_EMERG "(%04x)BTSET: %x\n", BTSET, audio_read_reg(BTSET));
    printk(KERN_EMERG "(%04x)BTCLR: %x\n", BTCLR, audio_read_reg(BTCLR));
    printk(KERN_EMERG "(%04x)BTSR: %x\n", BTSR, audio_read_reg(BTSR));
    printk(KERN_EMERG "(%04x)BFSR: %x\n", BFSR, audio_read_reg(BFSR));
    printk(KERN_EMERG "(%04x)BFCR0: %x\n", BFCR0,audio_read_reg(BFCR0));
    printk(KERN_EMERG "(%04x)BFCR1: %x\n", BFCR1, audio_read_reg(BFCR1));
    printk(KERN_EMERG "(%04x)BFCR2: %x\n", BFCR2, audio_read_reg(BFCR2));
    printk(KERN_EMERG "(%04x)BST0: %x\n", BST0, audio_read_reg(BST0));
    printk(KERN_EMERG "(%04x)BTT0: %x\n", BTT0, audio_read_reg(BTT0));
    printk("====================================\n");
}

static inline void set_src_dev_slot(int slot_id)
{
    audio_set_bit(BST0, DEV0_SUR, slot_id);
}

static inline void set_tar_dev_slot(int slot_id)
{
    audio_set_bit(BTT0, DEV5_TAR, slot_id);
}

void audio_connect_dev(void)
{
    int slot_id = 1;

    set_src_dev_slot(slot_id);

    set_tar_dev_slot(slot_id);

    audio_write_reg(BTSET, BIT(slot_id));
}
EXPORT_SYMBOL(audio_connect_dev);

void audio_disconnect_dev(void)
{
    int slot_id = 1;

    set_src_dev_slot(0);

    set_tar_dev_slot(0);

    audio_write_reg(BTCLR, BIT(slot_id));
}
EXPORT_SYMBOL(audio_disconnect_dev);

static inline int to_tsz(int unit_size)
{
    if (unit_size == 1*4) return 0;
    if (unit_size == 4*4) return 1;
    if (unit_size == 8*4) return 2;
    if (unit_size == 16*4) return 3;
    if (unit_size == 32*4) return 4;
    return 0;
}

static inline int to_ss(int data_bits)
{
    if (data_bits == 8) return 0;
    if (data_bits == 12) return 1;
    if (data_bits == 13) return 2;
    if (data_bits == 16) return 3;
    if (data_bits == 18) return 4;
    if (data_bits == 20) return 5;
    if (data_bits == 24) return 6;
    if (data_bits == 32) return 7;
    return 3;
}

static inline int is_packed(snd_pcm_format_t format)
{
    switch (format) {
        case SNDRV_PCM_FORMAT_S24_3LE:
        case SNDRV_PCM_FORMAT_U24_3LE:
        case SNDRV_PCM_FORMAT_S20_3LE:
        case SNDRV_PCM_FORMAT_U20_3LE:
        case SNDRV_PCM_FORMAT_S18_3LE:
        case SNDRV_PCM_FORMAT_U18_3LE:
        case SNDRV_PCM_FORMAT_S24_3BE:
        case SNDRV_PCM_FORMAT_U24_3BE:
        case SNDRV_PCM_FORMAT_S20_3BE:
        case SNDRV_PCM_FORMAT_U20_3BE:
        case SNDRV_PCM_FORMAT_S18_3BE:
        case SNDRV_PCM_FORMAT_U18_3BE:
            return 1;
        default:
            break;
   }
    return 0;
}

void audio_dma_desc_init(
    struct audio_dma_desc *desc,
    void *buf, int buf_size, int unit_size,
    struct audio_dma_desc *next)
{
    unsigned long cmd = 0;
    set_bit_field(&cmd, D_BAI, 1);
    set_bit_field(&cmd, D_TSZ, to_tsz(unit_size));
    desc->cmd = cmd;
    desc->dma_addr = virt_to_phys(buf);
    desc->next_desc = virt_to_phys(next);
    desc->trans_count = buf_size / unit_size;

    dma_cache_sync(NULL, desc, sizeof(*desc), DMA_MEM_TO_DEV);
}
EXPORT_SYMBOL(audio_dma_desc_init);

void audio_dma_config(int channels, int data_bits, int unit_size, snd_pcm_format_t format)
{
    audio_set_bit(DCM, NDES, 0);

    unsigned long ffr = 0;
    set_bit_field(&ffr, FTH, unit_size / 4);
    set_bit_field(&ffr, TUR_ROR_E, 0);
    set_bit_field(&ffr, TFS_RFS_E, 0);
    set_bit_field(&ffr, FIFO_TD, 1);
    audio_write_reg(FFR, ffr);

    unsigned long dfcr = 0;
    set_bit_field(&dfcr, CHNUM, channels - 1);
    set_bit_field(&dfcr, SS, to_ss(data_bits));
    set_bit_field(&dfcr, PACK_EN, is_packed(format));
    audio_write_reg(DFCR, dfcr);
}
EXPORT_SYMBOL(audio_dma_config);

void audio_dma_start(struct audio_dma_desc *desc)
{
    /* 清除此前的中断标志
     */
    audio_write_reg(DSR, bit_field_mask(TT_INT) |
                         bit_field_mask(LTT_INT) |
                         bit_field_mask(TT) |
                         bit_field_mask(LTT));

    audio_write_reg(DDA, virt_to_phys(desc));

    /* 听说要回读一下
     */
    audio_read_reg(DDA);

    // dma enable
    audio_set_bit(DCR, CTE, 1);

    // fifo enable
    audio_set_bit(FCR, FIFO_EN, 1);

    audio_set_bit(DCM, TIE, 0);

    audio_set_bit(DFCR, ENABLE, 1);
}
EXPORT_SYMBOL(audio_dma_start);

void audio_dma_stop(void)
{
    audio_set_bit(DCM, TIE, 0);

    /* 清除此前的中断标志
     */
    audio_write_reg(DSR, bit_field_mask(TT_INT) |
                         bit_field_mask(LTT_INT) |
                         bit_field_mask(TT) |
                         bit_field_mask(LTT));

    // fifo disable
    audio_write_reg(FCR, 0);

    audio_write_reg(DCR, 0);

    int count = 0xfff;
    while (audio_get_bit(DSR, RST_EN) && count--);

    audio_write_reg(DCR, bit_field_mask(DCR_RESET));
    udelay(100);

    audio_set_bit(DFCR, ENABLE, 0);
}
EXPORT_SYMBOL(audio_dma_stop);

unsigned int audio_dma_get_current_addr(void)
{
    return audio_read_reg(DBA);
}
EXPORT_SYMBOL(audio_dma_get_current_addr);

static int audio_module_init(void)
{
    struct clk *gate_clk = clk_get(NULL, "gate_dmic");
    if (IS_ERR_OR_NULL(gate_clk))
        panic("AUDIO: get gate_clk failed\n");

    clk_prepare_enable(gate_clk);

    audio_write_reg(DGRR, bit_field_mask(DMA_RESET));

    /* 必须如此,用作fifo数据接收同步
     */
    audio_write_reg(BFCR1,
        bit_field_mask(bit_field_start(DEV5_LSMP), bit_field_start(DEV5_LSMP)));
    audio_write_reg(BFCR2,
        bit_field_mask(bit_field_start(DEV0_DBE), bit_field_start(DEV0_DBE)));

    audio_write_reg(FAS, 384);

    clk_disable_unprepare(gate_clk);
    clk_put(gate_clk);

    return 0;
}

static void audio_module_exit(void)
{
}

module_init(audio_module_init);

module_exit(audio_module_exit);

MODULE_LICENSE("GPL");
