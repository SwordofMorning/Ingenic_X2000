#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <delay.h>
#include <driver/systick.h>
#include <driver/clk.h>
#include <driver/irq.h>
#include <cpu/irqflags.h>
#include <cpu/lep_ccu.h>
#include <cpu/tcsm_section.h>
#include <bit_field2.h>

#define MIN_TIMER_ADD_CNT 150

static __tcsm_data unsigned int rate_mhz = 600; // 默认600Mhz
static __tcsm_bss systick_event_handler_t cb;

union cycle_type {
    uint64_t cycle64;
    uint32_t cycle32[2];
};

static inline uint64_t __tcsm_text get_cycle(void)
{
    union cycle_type cycle;

    do {
        cycle.cycle32[1] = *CCU_TIME_H;
        cycle.cycle32[0] = *CCU_TIME_L;
        if (0xffffffff - cycle.cycle32[0] < 3000) {
            if (cycle.cycle32[1] != *CCU_TIME_H)
                continue;
        }
    } while(0);

    return cycle.cycle64;
}

uint64_t __tcsm_text systick_count_to_usec(uint64_t count)
{
    return count / rate_mhz;
}

uint64_t __tcsm_text systick_usec_to_count(uint64_t usec)
{
    return usec * rate_mhz;
}

uint64_t __tcsm_text systick_count_to_nsec(uint64_t count)
{
    return count * 1000 / rate_mhz;
}

uint64_t __tcsm_text systick_nsec_to_count(uint64_t nsec)
{
    return nsec * rate_mhz / 1000;
}

uint64_t __tcsm_text systick_get_time_usec(void)
{
    return systick_count_to_usec(get_cycle());
}

uint64_t __tcsm_text systick_get_time_nsec(void)
{
    return systick_count_to_nsec(get_cycle());
}

uint64_t __tcsm_text systick_get_count(void)
{
    return get_cycle();
}

void print_irq_ack_time(void)
{
    printf("-- %ld %ld %ld\n", *CCU_TIME_L, *CCU_TIME_CMP_L, *CCU_TIME_L - *CCU_TIME_CMP_L);
}

void __tcsm_text systick_handler(int irq, void *data)
{
    // print_irq_ack_time();

    *CCU_TIME_CMP_L = *CCU_TIME_CMP_L;

    if (cb)
        cb();
}

void __tcsm_text systick_set_event_count(uint64_t relative_count)
{
    unsigned long flags;

    static int is_init = 0;
    if (!is_init) {
        is_init = 1;
        request_irq(IRQ_V_MTIMER, 0, systick_handler, "systick", NULL);
    }

    local_irq_save(flags);

    while (1) {
        union cycle_type cycle, cycle1;
        cycle.cycle64 = get_cycle();

        if (relative_count <= MIN_TIMER_ADD_CNT)
            relative_count = MIN_TIMER_ADD_CNT;

        cycle1.cycle64 = cycle.cycle64 + relative_count;
        *CCU_TIME_CMP_H = cycle1.cycle32[1];
        *CCU_TIME_CMP_L = cycle1.cycle32[0];

        cycle1.cycle64 = get_cycle();
        if (cycle1.cycle64 - cycle.cycle64 <= relative_count)
            break;

        /* 如果超过了则可能不会产生中断, 所以多20周期再试 */
        relative_count = cycle1.cycle64 - cycle.cycle64 + 20;
    }

    local_irq_restore(flags);
}

void __tcsm_text systick_set_event_usec(uint64_t relative_usec)
{
    systick_set_event_count(systick_usec_to_count(relative_usec));
}

void __tcsm_text systick_set_next_usec(uint64_t usec)
{
    uint64_t now = systick_get_time_usec();
    uint64_t relative_usec = usec - now;

    if (now >= usec)
        relative_usec = 1;

    systick_set_event_usec(relative_usec);
}

void __tcsm_text udelay(unsigned int usec)
{
    uint64_t start = get_cycle();
    uint64_t cycles = systick_usec_to_count(usec);

    while ((get_cycle() - start) < cycles);
}

void __tcsm_text mdelay(unsigned int msec)
{
    while (msec--)
        udelay(1000);
}

void systick_set_event_callback(systick_event_handler_t callback)
{
    cb = callback;
}

void systick_init(void)
{
    *CCU_TIME_H = 0;
    *CCU_TIME_L = 0;
    *CCU_TIME_CMP_H = 0xffffffff;
    *CCU_TIME_CMP_L = 0xffffffff;

    *CCU_CCSR = set_bit_field(*CCU_CCSR, CCSR_Timer_en, 1);

    rate_mhz = clk_cpccr_get_rate(CLK_CPCCR_L2CLK)/(1000*1000);

}
