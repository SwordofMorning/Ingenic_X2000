#include <stdio.h>
#include <string.h>

#include <soc/base.h>
#include <cpu/io.h>

#include <driver/irq.h>
#include <cpu/host_cpu.h>

#include <driver/gpio.h>
#include <driver/hrtimer.h>
#include <driver/systick.h>

static struct hrtimer g_timer1;
static struct hrtimer g_timer2;
static struct hrtimer g_timer3;

static void timer1_callback(struct hrtimer *timer)
{
    printf("hr timer1_callback.\n");

    hrtimer_cancel(&g_timer2);
}

static void timer2_callback(struct hrtimer *timer)
{
    printf("hr timer2_callback.\n");
}

static void timer3_callback(struct hrtimer *timer)
{
    uint64_t time3_count = systick_nsec_to_count(3ll*1000*1000*1000);
    hrtimer_restart(timer, time3_count);
    printf("hr timer3_callback.\n");
}

/**
 * hrtimer 是利用systick 做的定时器
 * 由于 systick的时钟源为24M，所以每个cycles 的时间为 1000 / 24 = 41.66 ns
**/

void hrtimer_example(void)
{
    hrtimer_init(&g_timer1, timer1_callback);
    hrtimer_init(&g_timer2, timer2_callback);
    hrtimer_init(&g_timer3, timer3_callback);

    /*ns 对应 systick cycles 数*/
    uint64_t time1_count = systick_nsec_to_count(1ll*1000*1000*1000);
    uint64_t time2_count = systick_nsec_to_count(2ll*1000*1000*1000);

    /*us 对应 systick cycles 数*/
    uint64_t time3_count = systick_usec_to_count(3*1000*1000);

    hrtimer_start(&g_timer1, time1_count);

    hrtimer_start(&g_timer2, time2_count);

    hrtimer_start(&g_timer3, time3_count);
}
