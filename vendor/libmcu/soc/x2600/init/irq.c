#include <stdio.h>
#include <asm/riscv_csr.h>
#include <cpu/lep_ccu.h>
#include <cpu/tcsm_section.h>

#include <stdio.h>
#include <driver/irq.h>
#include <assert.h>
#include <cpu/irqflags.h>
#include <bit_field2.h>

void gpio_init_irq(void);
void gpio_enable_irq(int irq);
void gpio_disable_irq(int irq);
void gpio_startup_irq(int irq, unsigned int irqflags);
void gpio_shutdown_irq(int irq);

void intc_init_irq(void);
void intc_enable_irq(int irq);
void intc_disable_irq(int irq);

void cpu_enable_irq(int irq);
void cpu_disable_irq(int irq);

#ifndef APP_libmcu_simple_irq_handle
static __tcsm_bss irq_handler_t irq_funcs[IRQ_NUMS];
static __tcsm_bss void *irq_datas[IRQ_NUMS];
#endif

static inline void __tcsm_text check_irq(int irq)
{
    assert(0 <= irq && irq < IRQ_NUMS);
#ifndef APP_libmcu_simple_irq_handle
    assert(irq_funcs[irq]);
#endif
}

static inline void check_noirq(int irq)
{
    assert(0 <= irq && irq < IRQ_NUMS);
#ifndef APP_libmcu_simple_irq_handle
    assert(!irq_funcs[irq]);
#endif
}

static inline void __tcsm_text set_irq_data(int irq, irq_handler_t func, void *data)
{
#ifndef APP_libmcu_simple_irq_handle
    irq_funcs[irq] = func;
    irq_datas[irq] = data;
#endif
}

static inline int is_cpu_irq(int irq)
{
    return irq <= IRQ_V_MECALL;
}

static inline int is_intc_irq(int irq)
{
    return IRQ_INTC_START <= irq && irq < IRQ_INTC_END;
}

static inline int is_gpio_irq(int irq)
{
    return IRQ_GPIO_START <= irq && irq < IRQ_GPIO_END;
}

void request_irq_disabled(int irq, unsigned int irq_flags,
                irq_handler_t handler, const char *name, void *data)
{
    assert(handler);

    check_noirq(irq);
    set_irq_data(irq, handler, data);

    if (is_gpio_irq(irq))
    {
        gpio_startup_irq(irq, irq_flags);
    }

}

void request_irq(int irq, unsigned int irq_flags,
                irq_handler_t handler, const char *name, void *data)
{
    request_irq_disabled(irq, irq_flags, handler, name, data);

    enable_irq(irq);
}

void release_irq(int irq)
{
    check_irq(irq);

    if (is_gpio_irq(irq))
        gpio_shutdown_irq(irq);

    set_irq_data(irq, NULL, NULL);
}

int __tcsm_text gpio_to_irq(int gpio)
{
    return gpio >= 0 ? IRQ_GPIO_START + gpio : -1;
}

int __tcsm_text irq_to_gpio(int irq)
{
    return irq - IRQ_GPIO_START;
}

void __tcsm_text enable_irq(int irq)
{
    check_irq(irq);

    if (is_cpu_irq(irq)) {
        cpu_enable_irq(irq);
        return;
    }

    if (is_intc_irq(irq)) {
        intc_enable_irq(irq);
        return;
    }

    if (is_gpio_irq(irq)) {
        gpio_enable_irq(irq);
        return;
    }

    panic("irq not valid: %d\n", irq);
}

void __tcsm_text disable_irq(int irq)
{
    check_irq(irq);

    if (is_cpu_irq(irq)) {
        cpu_disable_irq(irq);
        return;
    }

    if (is_intc_irq(irq)) {
        intc_disable_irq(irq);
        return;
    }

    if (is_gpio_irq(irq)) {
        gpio_disable_irq(irq);
        return;
    }

    panic("irq not valid: %d\n", irq);
}

#ifndef APP_libmcu_simple_irq_handle
void __tcsm_text handle_irq(int irq)
{
    if (!irq_funcs[irq])
        panic("irq: %d no cb\n", irq);

    irq_funcs[irq](irq, irq_datas[irq]);
}
#else
__attribute__((weak)) void __tcsm_text handle_irq(int irq)
{
    panic("user must define handle_irq\n");
}
#endif

void _vectors_start(void);

void irq_init(void)
{
    /* 设置异常向量起始地址
     */
    csr_write(MTVEC, (unsigned int)_vectors_start);

    intc_init_irq();

    gpio_init_irq();

    local_irq_enable();
}

void __tcsm_text handler_int_c(void)
{
    unsigned int mcause = csr_read(MCAUSE);
    int code = get_bit_field(mcause, MCAUSE_CODE);

    if (code == 7)
        return handle_irq(IRQ_V_MTIMER);
    if (code == 3)
        return handle_irq(IRQ_V_MSOFT);
    if (code == 11)
        return handle_irq(IRQ_V_MEXTERNAL);
    if (code == 16)
        return handle_irq(IRQ_V_HOST_NOTIFY);


    panic("irq: may be some other exception: %x %x\n", mcause, csr_read(MEPC));
}

void __tcsm_text handler_fault_c(void)
{
    unsigned int mcause = csr_read(MCAUSE);
    int code = get_bit_field(mcause, MCAUSE_CODE);

    if (code == 11) {
        csr_write(MEPC, csr_read(MEPC)+4);
        return handle_irq(IRQ_V_MECALL);
    }

#ifdef APP_libmcu_enable_jtag_debug
    printf("err exception: %x %x\n", mcause, csr_read(MEPC));
    jtag_break();
#else
    panic("err exception: %x %x\n", mcause, csr_read(MEPC));
#endif
}