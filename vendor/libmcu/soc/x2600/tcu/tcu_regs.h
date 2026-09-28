#ifndef _TCU_REGS_H_
#define _TCU_REGS_H_

#include <soc/base.h>
#include <bit_field2.h>

/* 大核用tcu0 小核用tcu1
 */
#define TCU_ADDR(reg)   ((volatile unsigned long *)(TCU1_IOBASE + reg))

#define CHN_TDFR	(0x0)    /*channel N data full register*/
#define CHN_TDHR	(0x4)    /*channel N data half register*/
#define CHN_TCNT	(0x8)    /*channel N counter register*/
#define CHN_TCSR	(0xc)    /*channel N control register*/

#define TCU_TER         0x10 /* Enable timer counter */
#define TCU_TESR        0x14 /* Set enable */
#define TCU_TECR        0x18 /* Clear enable */
#define TCU_TSR         0x1C /* Stop timer counter */
#define TCU_TSSR        0x2C /* Set stop */
#define TCU_TSCR        0x3C /* Clear stop */
#define TCU_TFR         0x20 /* Comparison flag */
#define TCU_TFSR        0x24 /* Set comparison flag */
#define TCU_TFCR        0x28 /* Clear comparison flag */
#define TCU_TMR         0x30 /* Interrupt mask */
#define TCU_TMSR        0x34 /* Set interrupt mask */
#define TCU_TMCR        0x38 /* Clear interrupt mask */
#define TCU_TSFR        0x200 /* Store flag */
#define TCU_TSFSR       0x204 /* Set store falg */
#define TCU_TSFCR       0x208 /* Clear store flag */
#define TCU_TSMR        0x210 /* Store interrupt mask */
#define TCU_TSMSR       0x214 /* Set store interrupt mask */
#define TCU_TSMCR       0x218 /* Clear store interrupt mask */
#define TCU_TSVR(n)     (0x220 + (n)*0x04) /* Channel n value of stored conter */
#define TCU_TSFVR(n)    (0x240 + (n)*0x04) /* Channel n value of stored filter conter */

#define CHN_CAP(n)      (0xc0 + (n)*0x04) /*Capture register*/
#define CHN_CAP_VAL(n)	(0xe0 + (n)*0x04) /*Capture Value register*/
#define CHN_FIL_VAL(n)	(0x1a0 + (n)*0x04) /*filter value*/

#define TCR_STORE_NEG_EN        26, 26
#define TCR_STORE_POS_EN        25, 25
#define TCR_STORE_EN            24, 24
#define TCR_COUNT_MODE          22, 23
#define TCR_GPIO1_NEG_EN        21, 21
#define TCR_GPIO1_POS_EN        20, 20
#define TCR_GPIO0_NEG_EN        19, 19
#define TCR_GPIO0_POS_EN        18, 18
#define TCR_CLK_NEG_EN          17, 17
#define TCR_CLK_POS_EN          16, 16
#define TCR_SHUTDOWN            15, 15
#define TCR_GATE_POLA           14, 14
#define TCR_DIRECTION_POLA      13, 13
#define TCR_GATE_SEl            11, 12
#define TCR_DIRECTION_SEL       8, 10
#define TCR_GPIO1_EN            7, 7
#define TCR_GPIO0_EN            6, 6
#define TCR_PRESCALE            3, 5
#define TCR_EXT_EN              2, 2

#define TCU_FLAG_RD	(1 << 24)   /*HALF comparison match flag of WDT. (TCNT = TDHR) */
#define CSR_DIV_MSK	(0x7 << 3)

#define NR_TCU_CHNS  8
#define TCU_FULL0	(0x40)
#define TCU_CHN_OFFSET	(0x10)

void tcu_write_reg(unsigned int reg, int val)
{
    *TCU_ADDR(reg) = val;
}

unsigned int tcu_read_reg(unsigned int reg)
{
    return *TCU_ADDR(reg);
}

void tcu_write_chn_reg(struct tcu_config *tcu_config, unsigned int reg, int val)
{
    *TCU_ADDR(TCU_FULL0 + tcu_config->id * TCU_CHN_OFFSET + reg) = val;
}

unsigned int tcu_read_chn_reg(struct tcu_config *tcu_config, unsigned int reg)
{
    return *TCU_ADDR(TCU_FULL0 + tcu_config->id * TCU_CHN_OFFSET + reg);
}

static inline unsigned int tcu_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(*TCU_ADDR(reg), start, end);
}

static inline void tcu_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    *TCU_ADDR(reg) = set_bit_field(*TCU_ADDR(reg), start, end, val);
}

static inline void tcu_enable_counter(int id)
{
    tcu_write_reg(TCU_TESR, 1 << id);
}

static inline int tcu_disable_counter(int id)
{
    tcu_write_reg(TCU_TECR, 1 << id);
    return 1;
}

static inline void tcu_start_counter(int id)
{
    tcu_write_reg(TCU_TSCR, 1 << id);
}

static inline void tcu_stop_counter(int id)
{
    tcu_write_reg(TCU_TSSR, 1 << id);
}

/*Timer Mast Register set/clr operation and flag clr operation (full)*/
static inline void tcu_full_disable(int id)
{
    tcu_write_reg(TCU_TMSR, 1 << id);
}

static inline void tcu_full_enable(int id)
{
    tcu_write_reg(TCU_TMCR, 1 << id);
}

/*Timer Data FULL/HALF Register*/
static inline void tcu_set_chn_full(struct tcu_config *config, unsigned int value)
{
    tcu_write_chn_reg(config, CHN_TDFR, value);
}

static inline void tcu_set_chn_half(struct tcu_config *config, unsigned int value)
{
    tcu_write_chn_reg(config, CHN_TDHR, value);
}

static inline void tcu_clear_full_flag(int id)
{
    tcu_write_reg(TCU_TFCR, 1 << id);
}

static inline void tcu_clear_half_flag(int id)
{
    tcu_write_reg(TCU_TFCR, 1 << (id+16));
}

/*Timer Counter clear to zero*/
static inline void tcu_clear_count(struct tcu_config *config)
{
    tcu_write_chn_reg(config, CHN_TCNT, 0);
}



#endif /* _TCU_REGS_H_ */