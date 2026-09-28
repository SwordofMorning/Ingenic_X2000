#ifndef _TCU_REGS_H_
#define _TCU_REGS_H_

#include <soc/base.h>
#include <bit_field2.h>
#define TCU_ADDR(reg)   ((volatile unsigned long *)(TCU_IOBASE + reg))

#define CHN_TDFR	(0x0)    /*channel N data full register*/
#define CHN_TDHR	(0x4)    /*channel N data half register*/
#define CHN_TCNT	(0x8)    /*channel N counter register*/
#define CHN_TCSR	(0xc)    /*channel N control register*/

#define TCU_TSR		(0x1C)   /* Timer Stop Register */
#define TCU_TSSR	(0x2C)   /* Timer Stop Set Register */
#define TCU_TSCR	(0x3C)   /* Timer Stop Clear Register */
#define TCU_TER		(0x10)   /* Timer Counter Enable Register */
#define TCU_TESR	(0x14)   /* Timer Counter Enable Set Register */
#define TCU_TECR	(0x18)   /* Timer Counter Enable Clear Register */
#define TCU_TFR		(0x20)   /* Timer Flag Register */
#define TCU_TFSR	(0x24)   /* Timer Flag Set Register */
#define TCU_TFCR	(0x28)   /* Timer Flag Clear Register */
#define TCU_TMR		(0x30)   /* Timer Mask Register */
#define TCU_TMSR	(0x34)   /* Timer Mask Set Register */
#define TCU_TMCR	(0x38)   /* Timer Mask Clear Register */

#define CHN_CAP(n)      (0xc0 + (n)*0x04) /*Capture register*/
#define CHN_CAP_VAL(n)	(0xe0 + (n)*0x04) /*Capture Value register*/
#define CHN_FIL_VAL(n)	(0x1a0 + (n)*0x04) /*filter value*/

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

static inline void tcu_clear_full_flag(int id)
{
    tcu_write_reg(TCU_TFCR, 1 << id);
}

/*Timer Counter clear to zero*/
static inline void tcu_clear_count(struct tcu_config *config)
{
    tcu_write_chn_reg(config, CHN_TCNT, 0);
}



#endif /* _TCU_REGS_H_ */