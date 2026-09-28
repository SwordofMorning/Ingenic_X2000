#ifndef _TCU_REGS_H_
#define _TCU_REGS_H_

#define TCU0_IOBASE 0x13630000
#define TCU1_IOBASE 0x13640000

static const unsigned long tcu_iobase[] = {
    KSEG1ADDR(TCU0_IOBASE),
    KSEG1ADDR(TCU1_IOBASE),
};

#define TCU_ADDR(id, reg) ((volatile unsigned long *)((tcu_iobase[id]) + (reg)))

/* register */
#define TCU_TER     (0x10)
#define TCU_TESR    (0x14)
#define TCU_TECR    (0x18)

#define TCU_TSR     (0x1C)
#define TCU_TSSR    (0x2C)
#define TCU_TSCR    (0x3C)

#define TCU_TFR     (0x20)
#define TCU_TFSR    (0x24)
#define TCU_TFCR    (0x28)

#define TCU_TMR     (0x30)
#define TCU_TMSR    (0x34)
#define TCU_TMCR    (0x38)

#define TCU_TSFR    (0x200)
#define TCU_TSFSR   (0x204)
#define TCU_TSFCR   (0x208)
#define TCU_TSMR    (0x210)
#define TCU_TSMSR   (0x214)
#define TCU_TSMCR   (0x218)

#define CH_TDFR(n)            (0x40 + (n)*0x10) /* Timer Data Full Reg */
#define CH_TDHR(n)            (0x44 + (n)*0x10) /* Timer Data Half Reg */
#define CH_TCNT(n)            (0x48 + (n)*0x10) /* Timer Counter Reg */
#define CH_TCR(n)             (0x4C + (n)*0x10) /* Timer Control Reg */

#define CH_CAP_CTL(n)         (0xC0 + (n)*0x04) /* Capture Control register */
#define CH_CAP_VAL(n)         (0xE0 + (n)*0x04) /* Capture Value register */
#define CH_FIL_VAL(n)        (0x1A0 + (n)*0x04) /* filter value */

#define TCU_STORE_VAL(n)     (0x220 + (n)*0x04)
#define TCU_STORE_FIL_VAL(n) (0x240 + (n)*0x04)

#define TCU_OST_FULL          0x100
#define TCU_OST_CNTL          0x104
#define TCU_OST_CNTH          0x108
#define TCU_OST_CTRL          0x110

/* Control register bit filed operation */
#define TCR_PRESCALE_MASK   (7 << 3)
#define TCR_PRESCALE(n)     (n << 3)
#define TCR_SHUTDOWN_MASK   (1 << 15)
#define TCR_SHUTDOWN(n)     (n << 15)
#define TCR_COUNT_MODE_MASK (3 << 22)
#define TCR_COUNT_MODE(n)   (n << 22)

#define DIR_SEL_MASK     (7 << 8)
#define DIR_SEL(n)       (n << 8)
#define GATE_SEL_MASK    (3 << 11)
#define GATE_SEL(n)      (n << 11)

#define DIR_POLA_MASK    (1 << 13)
#define DIR_POLA(n)      (n << 13)
#define GATE_POLA_MASK   (1 << 14)
#define GATE_POLA(n)     (n << 14)

//filter value register bit operation
#define  FIL_B_MASK   (0x3ff << 16)
#define  FIL_B(n)         (n << 16)
#define  FIL_A_MASK   (0x3ff << 0)
#define  FIL_A(n)         (n << 0)
#define  CAP_NUM_MASK  (0xff << 0)
#define  CAP_NUM(n)       (n << 0)

/* CAP control register bit filed operation */
#define CAP_POS_SEL_MASK     (7 << 16)
#define CAP_POS_SEL(n)       (n << 16)

/* OST control register bit filed operation */
#define OST_PRESCALE_MASK   (7 << 3)
#define OST_PRESCALE(n)     (n << 3)
#define OST_COUNT_MODE_MASK (3 << 22)
#define OST_COUNT_MODE(n)   (n << 22)

/* TCU counter source*/
#define EXT_EN         (1 << 2)
#define GPIO0_EN       (1 << 6)
#define GPIO1_EN       (1 << 7)
#define STORE_EN       (1 << 24)

#define CLK_POS        (1 << 16)
#define CLK_NEG        (1 << 17)
#define GPIO0_POS      (1 << 18)
#define GPIO0_NEG      (1 << 19)
#define GPIO1_POS      (1 << 20)
#define GPIO1_NEG      (1 << 21)
#define STORE_POS_EN   (1 << 25)
#define STORE_NEG_EN   (1 << 26)

/* OST counter source*/
#define PCLK_EN        (1 << 0)
#define RTCCLK_EN      (1 << 1)
#define EXTCLK_EN      (1 << 2)

/* OST register bit operation */
#define OSTST      (1 << 15)   /* set OSTEN bit of TER to 1 */
#define OSTCL      (1 << 15)   /* set OSTEN bit of TER to 0 */
#define OSTSS      (1 << 15)   /* set OSTS  bit of TSR to 1 */
#define OSTSL      (1 << 15)   /* set OSTS  bit of TSR to 0 */

#define OSTFST     (1 << 15)   /* set OSTFLAG bit of TFR to 1 */
#define OSTFCL     (1 << 15)   /* set OSTFLAG bit of TFR to 0 */
#define OSTMST     (1 << 15)   /* set OSTMASK bit of TMR to 1 */
#define OSTMCL     (1 << 15)   /* set OSTMASK bit of TMR to 0 */

//watchdog interrupt
#define TFR_HALF_WDT  (1 << 24)   /* HALF comparison match flag of WDT. (TCNT = TDHR) */
#define TMR_HALF_WDT  (1 << 24)   /* HALF comparison match MASK of WDT. (TCNT = TDHR) */
#define TFR_FULL_WDT  (1 << 25)   /* FULL comparison match flag of WDT. (TCNT = TDFR) */
#define TMR_FULL_WDT  (1 << 25)   /* FULL comparison match MASK of WDT. (TCNT = TDFR) */

unsigned int tcu_read_reg(int index, unsigned int reg)
{
    return *TCU_ADDR(index, reg);
}

void tcu_write_reg(int index, unsigned int reg, int val)
{
    *TCU_ADDR(index, reg) = val;
}

static inline unsigned int tcu_get_bit(int index, unsigned int reg, int start, int end)
{
    return get_bit_field(TCU_ADDR(index, reg), start, end);
}

static inline int is_wdt_half_int(int index)
{
    return (tcu_read_reg(index, TCU_TFR) & TFR_HALF_WDT) && !(tcu_read_reg(index, TCU_TMR) & TMR_HALF_WDT);
}

static inline int is_wdt_full_int(int index)
{
    return (tcu_read_reg(index, TCU_TFR) & TFR_FULL_WDT) && !(tcu_read_reg(index, TCU_TMR) & TMR_FULL_WDT);
}

static inline int is_chn_half_int(int index, int id)
{
    return (tcu_read_reg(index, TCU_TFR) & (1 << (id + 16))) && !(tcu_read_reg(index, TCU_TMR) & (1 << (id + 16)));
}

static inline int is_chn_full_int(int index, int id)
{
    return (tcu_read_reg(index, TCU_TFR) & (1 << id)) && !(tcu_read_reg(index, TCU_TMR) & (1 << id));
}

static inline int is_store_int(int index, int id)
{
    return (tcu_read_reg(index, TCU_TSFR) & (1 << id)) && !(tcu_read_reg(index, TCU_TSMR) & (1 << id));
}

static inline int is_chn_running(int index, int id)
{
    return (tcu_read_reg(index, TCU_TER) & (1 << id));
}

#endif /* _TCU_REGS_H_ */
