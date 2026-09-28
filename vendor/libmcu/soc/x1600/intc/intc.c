#include <stdio.h>
#include <soc/base.h>
#include <bits_opt.h>
#include <driver/irq.h>
#include <cpu/ffs.h>

#define DSR0 0x34 /* Source Register0 */
#define DMR0 0x38 /* Mask Register0 */
#define DPR0 0x3C /* Pending Register0 */
#define DSR1 0x40 /* Source Register1 */
#define DMR1 0x44 /* Mask Register1 */
#define DPR1 0x48 /* Pending Register1 */

#define INTC_ADDR(reg) ((volatile unsigned long *)(MCU_INTC_IOBASE + reg))

static inline unsigned int intc_read_reg(int reg)
{
    return *INTC_ADDR(reg);
}

static inline void intc_write_reg(int reg, unsigned int value)
{
    *INTC_ADDR(reg) = value;
}

static inline void intc_set_bit(int reg, int n)
{
    set_bits(*INTC_ADDR(reg), BIT(n));
}

static inline void intc_clear_bit(int reg, int n)
{
    clear_bits(*INTC_ADDR(reg), BIT(n));
}

void intc_enable_irq(int irq)
{
    int intc = irq - IRQ_INTC_START;
    if (intc >= 32)
        intc_clear_bit(DMR1, intc - 32);
    else
        intc_clear_bit(DMR0, intc);
    printf("dmr: %x %x %d\n", intc_read_reg(DMR0), intc_read_reg(DMR1), intc);
}

void intc_disable_irq(int irq)
{
    int intc = irq - IRQ_INTC_START;
    if (intc >= 32)
        intc_set_bit(DMR1, intc - 32);
    else
        intc_set_bit(DMR0, intc);
}

static void intc_irq_handler(int irq, void *data)
{
    unsigned long ipr;

    ipr = intc_read_reg(DPR0);
    if (ipr) {
        handle_irq(__ffs(ipr) + IRQ_INTC_START);
        return;
    }

    ipr = intc_read_reg(DPR1);
    if (ipr)
        handle_irq(__ffs(ipr) + IRQ_INTC_START + 32);
}

void intc_init_irq(void)
{
    // disable all intc irq at first
    intc_write_reg(DMR0, 0xffffffff);
    intc_write_reg(DMR1, 0xffffffff);

    request_irq(IRQ_CPU_INTC, 0, intc_irq_handler, "intc", NULL);
}
