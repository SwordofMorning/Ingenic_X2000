#include <stdio.h>
#include <asm/riscv_csr.h>
#include <cpu/lep_ccu.h>

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
static irq_handler_t irq_funcs[IRQ_NUMS];
static void *irq_datas[IRQ_NUMS];
#endif

static inline void check_irq(int irq)
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

static inline void set_irq_data(int irq, irq_handler_t func, void *data)
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

int gpio_to_irq(int gpio)
{
    return gpio >= 0 ? IRQ_GPIO_START + gpio : -1;
}

int irq_to_gpio(int irq)
{
    return irq - IRQ_GPIO_START;
}

void enable_irq(int irq)
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

void disable_irq(int irq)
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
void handle_irq(int irq)
{
    if (!irq_funcs[irq])
        panic("irq: %d no cb\n", irq);

    irq_funcs[irq](irq, irq_datas[irq]);
}
#else
__attribute__((weak)) void handle_irq(int irq)
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

void handler_int_c(void)
{
    unsigned int mcause = csr_read(MCAUSE);
    int code = get_bit_field(mcause, MCAUSE_CODE);

    if (code == 7)
        return handle_irq(IRQ_V_MTIMER);
    if (code == 3)
        return handle_irq(IRQ_V_MSOFT_HOST_NOTIFY);
    if (code == 11)
        return handle_irq(IRQ_V_MEXTERNAL);

    panic("irq: may be some other exception: %x %x\n", mcause, csr_read(MEPC));
}

void handler_fault_c(void *data)
{
    unsigned int mcause = csr_read(MCAUSE);
    int code = get_bit_field(mcause, MCAUSE_CODE);

    if (code == 11) {
        csr_write(MEPC, csr_read(MEPC)+4);
        return handle_irq(IRQ_V_MECALL);
    }

    unsigned long *r = (unsigned long *)data;
    printf("zero  ra  sp  gp: %08lx %08lx %08lx %08lx\n", 0l, r[1], (unsigned long)r, r[3]);
    printf("  tp  t0  t1  t2: %08lx %08lx %08lx %08lx\n", r[4], r[5], r[6], r[7]);
    printf("  s0  s1  a0  a1: %08lx %08lx %08lx %08lx\n", r[32], r[9], r[10], r[11]);
    printf("  a2  a3  a4  a5: %08lx %08lx %08lx %08lx\n", r[12], r[13], r[14], r[15]);
    printf("  a6  a7  s2  s3: %08lx %08lx %08lx %08lx\n", r[16], r[17], r[18], r[19]);
    printf("  s4  s5  s6  s7: %08lx %08lx %08lx %08lx\n", r[20], r[21], r[22], r[23]);
    printf("  s8  a9 s10 s11: %08lx %08lx %08lx %08lx\n", r[24], r[25], r[26], r[27]);
    printf("  t3  t4  t5  t6: %08lx %08lx %08lx %08lx\n", r[28], r[29], r[30], r[31]);
    panic("Game over! err exception: %x %x, code: %d\n", mcause, csr_read(MEPC), code);
}
