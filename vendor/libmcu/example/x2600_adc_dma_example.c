#include <stdio.h>
#include <string.h>

#include <delay.h>
#include <driver/systick.h>
#include <driver/adc.h>
#include <cpu/uncache_mem.h>
#include <assert.h>

static unsigned short *seq1_dma_buf;

#define SEQ1_CHANNEL_CNT 12

static struct adc_seq1_config seq1_adc = {
    .continus_clk_div = 30000,
    .delay_clk_div = 30000,
    .trigger = adc_trigger_software,
    .enable_channel_num = 1,
    .channel_cnt = SEQ1_CHANNEL_CNT,
    .channels = { 0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15},
    .channel_delays = {10,10,10,10,10,10,10,10,10,10,10,10,},
    .group_cnt = 1,
    .groups = {SEQ1_CHANNEL_CNT},
    .group_delays = {1000, },
    .irq_cb = NULL,
    .dma_mode = 1,
};

static void print_seq1_data(void)
{
    unsigned short values[seq1_adc.dma_size];
    int ssize = adc_dma_seq1_read_data(&seq1_adc, values);
    if (ssize <= 0)
        return ;

    int i;
    printf("seq1: %08lld=====", systick_get_time_usec());
    for (i = 0; i < ssize / 2; i++)
        printf(" %02d:%04d", values[i] >> 12, values[i] & 0xfff);
    printf("\n");

    return ;
}

static void seq1_irq_cb(void)
{
    print_seq1_data();
}

void adc_dma_test_seq1_tcu_irq(void)
{
    seq1_adc.dma_size = 256;
    seq1_dma_buf = uncache_mem_alloc(seq1_adc.dma_size, 32);
    assert(seq1_dma_buf);
    seq1_adc.dma_buf = seq1_dma_buf;

    seq1_adc.irq_cb = seq1_irq_cb;
    adc_enable_seq1(&seq1_adc);

    adc_start_seq1();

    while (1);

    adc_disable_seq1(&seq1_adc);
}

void adc_dma_test_seq1_tcu_poll(void)
{
    seq1_adc.dma_size = 256;
    seq1_dma_buf = uncache_mem_alloc(seq1_adc.dma_size, 32);
    assert(seq1_dma_buf);
    seq1_adc.dma_buf = seq1_dma_buf;

    adc_enable_seq1(&seq1_adc);

    adc_start_seq1();

    while (1) {
        while (!adc_dma_poll_seq1_data_ready(&seq1_adc));

        print_seq1_data();
    }

    adc_disable_seq1(&seq1_adc);
}

#include <stdio.h>
#include <string.h>

#include <delay.h>
#include <driver/systick.h>
#include <driver/tcu.h>
#include <driver/adc.h>
#include <cpu/uncache_mem.h>
#include <assert.h>

static unsigned short *seq2_dma_buf;

#define SEQ2_CHANNEL_CNT 8

static struct adc_seq2_config seq2_adc = {
    .delay_clk_div = 30000,
    .trigger = adc_trigger_software,
    .enable_channel_num = 1,
    .channel_cnt = SEQ2_CHANNEL_CNT,
    .channels = { 0, 1, 2, 3, 8, 9, 10, 11},
    // .channel_delays = {10,10,10,10,10,10,10,10,},
    .irq_cb = NULL,
    .dma_mode = 1,
};

void print_seq2_data(void)
{
    unsigned short values[seq2_adc.dma_size];
    int ssize = adc_dma_seq2_read_data(&seq2_adc, values);
    if (ssize <= 0)
        return ;

    int i;
    printf("seq2: %08lld=====", systick_get_time_usec());
    for (i = 0; i < ssize / 2; i++)
        printf(" %02d:%04d", values[i] >> 12, values[i] & 0xfff);
    printf("\n");
}

static void seq2_irq_cb(void)
{
    print_seq2_data();
}

void adc_dma_test_seq2_tcu_irq(void)
{
    seq2_adc.dma_size = 256;
    seq2_dma_buf = uncache_mem_alloc(seq2_adc.dma_size, 32);
    assert(seq2_dma_buf);
    seq2_adc.dma_buf = seq2_dma_buf;

    seq2_adc.trigger = adc_trigger_tcu0_half;
    seq2_adc.irq_cb = seq2_irq_cb;
    adc_enable_seq2(&seq2_adc);
    // adc_start_seq2();

    // tcu 用做 纯 timer
    struct tcu_config tcu_timer;
    tcu_config_timer_count(&tcu_timer, 0, 20000, NULL);
    tcu_timer.clk_div = clk_div_1024;
    tcu_timer.enable_irq = 0;
    tcu_timer.half_value = 10000;
    tcu_enable(&tcu_timer);

    while (1);
}

void adc_dma_test_seq2_tcu_poll(void)
{
    seq2_adc.dma_size = 256;
    seq2_dma_buf = uncache_mem_alloc(seq2_adc.dma_size, 32);
    assert(seq2_dma_buf);
    seq2_adc.dma_buf = seq2_dma_buf;

    seq2_adc.trigger = adc_trigger_tcu0_half;
    adc_enable_seq2(&seq2_adc);
    // adc_start_seq2();

    // tcu 用做 纯 timer
    struct tcu_config tcu_timer;
    tcu_config_timer_count(&tcu_timer, 0, 20000, NULL);
    tcu_timer.clk_div = clk_div_1024;
    tcu_timer.enable_irq = 0;
    tcu_timer.half_value = 10000;
    tcu_enable(&tcu_timer);

    while (1) {
        while (!adc_dma_poll_seq2_data_ready(&seq2_adc));

        print_seq2_data();
    }
}
