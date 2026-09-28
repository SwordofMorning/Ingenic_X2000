#ifndef _CPU_IRQFLAGS_H_
#define _CPU_IRQFLAGS_H_

#include <asm/riscv_csr.h>

static inline void local_irq_disable(void)
{
    csr_clear_bits(MSTATUS, MSTATUS_MIE);
}

static inline void local_irq_enable(void)
{
    csr_set_bits(MSTATUS, MSTATUS_MIE);
}

static inline unsigned long _local_irq_save(void)
{
    unsigned int status = csr_read(MSTATUS);

    csr_clear_bits(MSTATUS, MSTATUS_MIE);

    return status & MSTATUS_MIE;
}

#define local_irq_save(flags)                         \
    do {                                              \
        flags = _local_irq_save();                    \
    } while (0)

static inline void local_irq_restore(unsigned long flags)
{
    // if (flags)
    //     local_irq_enable();
    csr_set_bits(MSTATUS, flags);
}

#endif /* _CPU_IRQFLAGS_H_ */
