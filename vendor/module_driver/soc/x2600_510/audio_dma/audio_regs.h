#ifndef _AUDIO_REGS_H_
#define _AUDIO_REGS_H_

#include <linux/io.h>

#define AUDIO_BASE 0x134c0000
#define FIFO_OFF 0x1000
#define DFMT_OFF 0x2000
#define FIFO_DATA_OFF 0x3000
#define IBUS_OFF 0x4000


/*
 * DMA regs
 */
#define DBA  0x78
#define DTC  0x7c
#define DCR  0x80
#define DSR  0x84
#define DCM  0x88
#define DDA  0x8c
#define DGRR 0x100
#define DGER 0x104
#define DEIER 0x108
#define DEISR 0x10c
#define DIPR  0x110

/* DCR */
#define DFS 1, 1 //dma Force stop
#define DCR_RESET 1, 1 // 参考内核中驱动,手册里面没有
#define CTE 0, 0

/* DSR */
#define CDOA  8, 15
#define TT_INT 5, 5
#define LTT_INT 4, 4
#define TT 3, 3
#define LTT 2, 2
#define RST_EN 1, 1  // 参考内核中驱动,手册里面没有
#define LINK 0, 0

/* DCM */
#define NDES 31, 31
#define BAI 8, 8
#define TSZ 4, 6
#define TIE 1, 1
#define LTIE 0, 0

/* DGRR */
#define DMA_RESET 0, 0

/* DGER */
#define DMA_EN 0, 0

/* DEIER */
#define EXP_EN 0, 0

/* DEISR */
#define EXP_STATUS 0, 0

/* DIPR */
#define EXP_INT 11, 11
#define DMIC_INT 10, 10
#define DMA_INT  5, 5

#define D_BAI 8, 8
#define D_TSZ 4, 6

/*
 * FIFO regs
 */
#define FAS (FIFO_OFF + 0x50)
#define FCR (FIFO_OFF + 0x54)
#define FFR (FIFO_OFF + 0x58)
#define FSR (FIFO_OFF + 0x5c)

/* FAS */
#define FAD 0, 12

/* FCR */
#define FLUSH 2, 2
#define FULL_EN 1, 1
#define FIFO_EN 0, 0

/* FFR */
#define FTH 16, 25
#define TUR_ROR_E 9, 9
#define TFS_RFS_E 8, 8
#define FIFO_TD 7, 7

/* FSR */
#define FLEVEL 16, 28
#define TUR_ROR_INPRO 5, 5
#define TFS_RFS_INPRO 4, 4
#define TUR_ROR 2, 2
#define TFS_RFS 0, 0

/*
 * DATA FORMAT regs
 */
#define DFCR (DFMT_OFF + 0x14)

/* DFCR */
#define CHNUM 12, 15
#define SS 8, 10
#define PACK_EN 2, 2
#define RECEN   0, 0

/*
 * FIFO DATA regs
 */
#define FDR (FIFO_DATA_OFF + 0x00)

/*
 * IBUS regs
 */
#define BTSET (IBUS_OFF + 0x0)
#define BTCLR (IBUS_OFF + 0x4)
#define BTSR (IBUS_OFF + 0x8)
#define BFSR (IBUS_OFF + 0xc)
#define BFCR0 (IBUS_OFF + 0x10)
#define BFCR1 (IBUS_OFF + 0x14)
#define BFCR2 (IBUS_OFF + 0x18)

#define BST0 (IBUS_OFF + 0x1c)
#define BTT0 (IBUS_OFF + 0x28)

#define BST0_OFF (0x1c)
#define BTT0_OFF (0x28)

/* BTSET */
#define TSLOT_SET 1, 31
/* BTCLR */
#define TSLOT_CLR 1, 31
/* BTSR */
#define TSLOT_EN 1, 31

/* BFSR */
#define DEV5_TUR 21, 21
#define DEV0_ROR 0, 0

/* BFCR0 */
#define DEV5_TFIFO 21, 21
#define DEV0_RFIFO 0, 0

/* BFCR1 */
#define DEV5_LSMP 5, 5

/* BFCR2 */
#define DEV0_DBE 0, 0

/* BST0 */
#define DEV0_SUR 0, 4

/* BTT0 */
#define DEV5_TAR 26, 30

#define AUDIO_ADDR(reg) ((volatile unsigned long *)KSEG1ADDR(AUDIO_BASE + (reg)))

#define IRQ_AUDIO (IRQ_INTC_BASE + 0)

static inline void audio_write_reg(unsigned int reg, unsigned int value)
{
    *AUDIO_ADDR(reg) = value;
}

static inline unsigned int audio_read_reg(unsigned int reg)
{
    return *AUDIO_ADDR(reg);
}

static inline void audio_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(AUDIO_ADDR(reg), start, end, val);
}

static inline unsigned int audio_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(AUDIO_ADDR(reg), start, end);
}

#endif /* _AUDIO_REGS_H_ */
