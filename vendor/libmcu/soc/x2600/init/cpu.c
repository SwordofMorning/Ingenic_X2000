#include <asm/riscv_csr.h>
#include <driver/irq.h>
#include <cpu/tcsm_section.h>

static int mie_flag[] = {MIE_MSIE, MIE_MTIE, MIE_MEIE, MIE_MUSER};

void __tcsm_text cpu_enable_irq(int irq)
{
    csr_set_bits(MIE, mie_flag[irq]);
}

void __tcsm_text cpu_disable_irq(int irq)
{
    csr_clear_bits(MIE, mie_flag[irq]);
}