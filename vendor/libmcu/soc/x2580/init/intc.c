#include <stdio.h>
#include <cpu/lep_ccu.h>
#include <driver/irq.h>
#include <bits_opt.h>

void intc_enable_irq(int irq)
{
    int intc = irq - IRQ_INTC_START;
    volatile unsigned long *INTC_MASK = CCU_INTC_MASK_L;
    if (intc >= 32) {
        intc -= 32;
        INTC_MASK = CCU_INTC_MASK_H;
    }

    set_bits(*INTC_MASK, BIT(intc));
}

void intc_disable_irq(int irq)
{
    int intc = irq - IRQ_INTC_START;
    volatile unsigned long *INTC_MASK = CCU_INTC_MASK_L;
    if (intc >= 32) {
        intc -= 32;
        INTC_MASK = CCU_INTC_MASK_H;
    }

    clear_bits(*INTC_MASK, BIT(intc));
}

static void intc_irq_handler(int irq, void *data)
{
    unsigned long ipr = *CCU_INTC_PEND_L & *CCU_INTC_MASK_L;

    if (ipr) {
        handle_irq(31 - __builtin_clz(ipr) + IRQ_INTC_START);
        return;
    }

    ipr = *CCU_INTC_PEND_H & *CCU_INTC_MASK_H;
    if (ipr)
        handle_irq(31 - __builtin_clz(ipr) + IRQ_INTC1_START);
}

void intc_init_irq(void)
{
    *CCU_INTC_MASK_L = 0;
    *CCU_INTC_MASK_H = 0;

    request_irq(IRQ_V_MEXTERNAL, 0, intc_irq_handler, "intc", NULL);
}
