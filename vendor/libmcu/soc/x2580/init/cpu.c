#include <asm/riscv_csr.h>
#include <driver/irq.h>

static int mie_flag[] = {MIE_MSIE, MIE_MTIE, MIE_MEIE};

void cpu_enable_irq(int irq)
{
    csr_set_bits(MIE, mie_flag[irq]);
}

void cpu_disable_irq(int irq)
{
    csr_clear_bits(MIE, mie_flag[irq]);
}
