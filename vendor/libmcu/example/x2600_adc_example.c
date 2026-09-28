#include <stdio.h>
#include <driver/systick.h>
#include <driver/adc.h>
#include <bits_opt.h>

#define SEQ0_CHANNEL_CNT 4

static struct adc_seq0_config seq0_adc = {
    .channel_cnt = SEQ0_CHANNEL_CNT,
    .channels = {0, 1, 2, 3},
    .irq_cb = NULL,
};

static void print_seq0_data(void)
{
    unsigned short values[SEQ0_CHANNEL_CNT];
    adc_read_seq0_data(values, SEQ0_CHANNEL_CNT);
    printf("adc data: %d %d %d %d\n", values[0],values[1],values[2],values[3]);
}

static void seq0_irq_cb(void)
{
    print_seq0_data();
}

void adc_test_seq0_irq(void)
{
    seq0_adc.irq_cb = seq0_irq_cb;
    adc_enable_seq0(&seq0_adc);

    adc_start_seq0();
}

void adc_test_seq0_poll(void)
{
    adc_enable_seq0(&seq0_adc);

    adc_start_seq0();

    while (!adc_poll_seq0_data_ready());

    print_seq0_data();
}


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
};

static void print_seq1_data(void)
{
    int len = SEQ1_CHANNEL_CNT;
    unsigned short values[SEQ1_CHANNEL_CNT];

    adc_read_seq1_data(values, len);

    printf("seq1: %08lld", systick_get_time_usec());

    int i;
    for (i = 0; i < len; i++)
        printf(" %d:%d", values[i] >> 12, values[i] & 0xfff);
    printf("\n");
}

static void seq1_irq_cb(void)
{
    print_seq1_data();
}

void adc_test_seq1_irq(void)
{
    seq1_adc.irq_cb = seq1_irq_cb;
    adc_enable_seq1(&seq1_adc);

    adc_start_seq1();
}

void adc_test_seq1_poll(void)
{
    adc_enable_seq1(&seq1_adc);

    adc_start_seq1();

    while (1) {
        while (!adc_poll_seq1_data_ready());

        print_seq1_data();
    }
}


#define SEQ2_CHANNEL_CNT 8

void print_seq2_data(void)
{
    int len = SEQ2_CHANNEL_CNT;
    unsigned short values[SEQ2_CHANNEL_CNT];

    adc_read_seq2_data(values, len);

    printf("seq2: %08lld", systick_get_time_usec());

    int i;
    for (i = 0; i < len; i++)
        printf(" %d:%d", values[i] >> 12, values[i] & 0xfff);
    printf("\n");
}

static void print_awd_data(unsigned short low_flags, unsigned short high_flags)
{
    if (test_bit(low_flags, 0))
        printf("adc0 watch dog low threshold trigger\n");

    if (test_bit(high_flags, 0))
        printf("adc0 watch dog high threshold trigger\n");

    if (test_bit(high_flags, 1))
        printf("adc1 watch dog high threshold trigger\n");

    if (test_bit(low_flags, 3))
        printf("adc3 watch dog low threshold trigger\n");
}

static struct adc_seq2_config seq2_adc = {
    .delay_clk_div = 30000,
    .trigger = adc_trigger_software,
    .enable_channel_num = 1,
    .channel_cnt = SEQ2_CHANNEL_CNT,
    .channels = { 0, 1, 2, 3, 8, 9, 10, 11},
    // .channel_delays = {10,10,10,10,10,10,10,10,},
    .irq_cb = NULL,
};

static void seq2_irq_cb(void)
{
    print_seq2_data();
}

static void awd_irq_cb(unsigned short low_flags, unsigned short high_flags)
{
    print_awd_data(low_flags, high_flags);
}

void adc_test_seq2_tcu_irq(void)
{
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
}

void adc_test_seq2_tcu_poll(void)
{
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
        while (!adc_poll_seq2_data_ready());

        print_seq2_data();
    }
}

void adc_test_seq2_tcu_awd_irq(void)
{
    adc_enable_awd(0, 400, 1400);
    adc_enable_awd(1, -1, 2000);
    adc_enable_awd(3, 500, -1);

    adc_set_awd_cb(awd_irq_cb);

    seq2_adc.trigger = adc_trigger_tcu0_half;
    // seq2_adc.irq_cb = seq2_irq_cb;
    adc_enable_seq2(&seq2_adc);
    // adc_start_seq2();

    // tcu 用做 纯 timer
    struct tcu_config tcu_timer;
    tcu_config_timer_count(&tcu_timer, 0, 20000, NULL);
    tcu_timer.clk_div = clk_div_1024;
    tcu_timer.enable_irq = 0;
    tcu_timer.half_value = 10000;
    tcu_enable(&tcu_timer);
}
