#include <stdio.h>
#include <driver/clk.h>
#include <driver/systick.h>
#include <driver/irq.h>

#include <soc/base.h>
#include <soc/cpm.h>

#include <bit_field2.h>
#include <bits_opt.h>

#define TCU_IOBASE  0x10002000

#define TCU_TER     (0x10)    /*Timer Counter Enable Register */
#define TCU_TESR    (0x14)    /*Timer Counter Enable Set Register */
#define TCU_TECR    (0x18)    /*Timer Counter Enable Clear Register*/

#define TCU_TMR     (0x30)    /* Timer Mask Register */
#define TCU_TMSR    (0x34)    /* Timer Mask Set Register */
#define TCU_TMCR    (0x38)    /* Timer Mask Clear Register */

#define TMSR_OSTMSK  15
#define TMCR_OSTMCL  15

#define TCU_FLAG_RD     (0x20)
#define TCU_FLAG_SET    (0x24)
#define TCU_FLAG_CLR    (0x28)

#define FLAG_CLR_OSTFCL 15

/* TCU TER */
#define TER_OSTEN       15
#define TER_OSTCL       15

/*OST*/
#define TCU_OSTFULL        (0x100)    /*TCU OST Full*/
#define TCU_OSTCNTL        (0x104)
#define TCU_OSTCNTH        (0x108)
#define TCU_OSTCNTB        (0x10c)
#define TCU_OSTCSR         (0x110)

/* OST TCSR */
#define OSTCSR_CNT_MD      (1 << 22)
#define OSTCSR_SD          (1 << 15)

#define JZ_EXTAL           24000000
#define USEC_PER_SEC       1000000UL
#define EXTAL_CLOCK_MHZ    (JZ_EXTAL / USEC_PER_SEC)

#define OSTCSR_PRESCALE    (0 << 3)
#define OSTCSR_PRESCALE_1  1
#define OSTCSR_EXT_EN      (1 << 2)

#define OST_TIMER_COUNT_MASK           0xFFFFFFFFULL

#define ost_usec_to_count(usec)        ((usec) * 24)
#define ost_count_to_usec(count)       ((count) / 24)

#define ost_nsec_to_count(nsec)        ((nsec * 24) / 1000)
#define ost_count_to_nsec(count)       ((count * 1000) / 24)

#define CPM_CLKGR    0x20
#define CLKGR_TCU    BIT(30)

#define OST_ADDR(reg)    ((volatile unsigned long *)(TCU_IOBASE + reg))

static systick_event_handler_t ost_callback_handler;

static inline uint32_t ost_read_reg(int reg)
{
    return *OST_ADDR(reg);
}

static inline void ost_write_reg(int reg, uint32_t value)
{
    *OST_ADDR(reg) = value;
}

static inline void ost_set_bit(int reg, int n)
{
    set_bits(*OST_ADDR(reg), BIT(n));
}

static inline void ost_clear_bit(int reg, int n)
{
    clear_bits(*OST_ADDR(reg), BIT(n));
}

static inline uint64_t get_cycle(void)
{
    union clycle_type {
        uint64_t cycle64;
        uint32_t cycle32[2];
    } cycle;

    do {
        cycle.cycle32[1] = ost_read_reg(TCU_OSTCNTH);
        cycle.cycle32[0] = ost_read_reg(TCU_OSTCNTL);
    } while(cycle.cycle32[1] != ost_read_reg(TCU_OSTCNTH));

    return cycle.cycle64;
}

static void systick_handler(int irq, void *data)
{
    /* disable ost inter */
    ost_set_bit(TCU_TMSR, TMSR_OSTMSK);

    if (ost_callback_handler)
        ost_callback_handler();
}

uint64_t systick_get_count(void)
{
    return get_cycle();
}

uint64_t systick_get_time_usec(void)
{
    uint64_t count = get_cycle();

    return ost_count_to_usec(count);
}

uint64_t systick_get_time_nsec(void)
{
    uint64_t count = get_cycle();

    return ost_count_to_nsec(count);
}

uint64_t systick_count_to_usec(uint64_t count)
{
    return ost_count_to_usec(count);
}

uint64_t systick_usec_to_count(uint64_t usec)
{
    return ost_usec_to_count(usec);
}

uint64_t systick_count_to_nsec(uint64_t count)
{
    return ost_count_to_nsec(count);
}

uint64_t systick_nsec_to_count(uint64_t nsec)
{
    return ost_nsec_to_count(nsec);
}

void systick_set_event_count(uint64_t relative_count)
{
    uint32_t count;

    static int is_init = 0;
    if (!is_init) {
        is_init = 1;
        request_irq(IRQ_TCU2, 0, systick_handler, "ost", NULL);
    }

    if (relative_count > OST_TIMER_COUNT_MASK)
        count = OST_TIMER_COUNT_MASK;
    else
        count = relative_count;

    count = ost_read_reg(TCU_OSTCNTL) + count;

    ost_set_bit(TCU_FLAG_CLR, FLAG_CLR_OSTFCL);
    ost_write_reg(TCU_OSTFULL, count);

    /* enable ost inter*/
    ost_set_bit(TCU_TMCR, TMCR_OSTMCL);
}

void systick_set_event_usec(uint64_t relative_usec)
{
    uint64_t count = ost_usec_to_count(relative_usec);
    systick_set_event_count(count);
}

void systick_set_next_usec(uint64_t usec)
{
    uint64_t now = systick_get_time_usec();
    uint64_t relative_usec = usec - now;

    if (now >= usec)
        relative_usec = 1;

    systick_set_event_usec(relative_usec);
}

void systick_set_event_callback(systick_event_handler_t callback)
{
    ost_callback_handler = callback;
}

void udelay(uint32_t usec)
{
    uint64_t start = get_cycle();
    uint64_t cycles = ost_usec_to_count(usec);

    while ((get_cycle() - start) < cycles);
}

void mdelay(uint32_t msec)
{
    while (msec--)
        udelay(1000);
}

void systick_init(void)
{
    /* enable clk */
    clk_enable(CLK_GATE_TCU, 1);

    /* enable systick counter */
    ost_set_bit(TCU_TESR, TER_OSTEN);

    ost_write_reg(TCU_OSTFULL, 0);
    ost_write_reg(TCU_OSTCNTL, 0);
    ost_write_reg(TCU_OSTCNTH, 0);

    ost_write_reg(TCU_OSTCSR, (OSTCSR_CNT_MD | OSTCSR_PRESCALE | OSTCSR_EXT_EN));
}
