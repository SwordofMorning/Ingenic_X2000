#include <stdio.h>
#include <string.h>
#include <driver/pwm.h>
#include <driver/systick.h>

struct pwm_handle pwm_handle;

struct pwm_config_data config = {
    .shutdown_mode = PWM_graceful_shutdown, /* 设置PWM停止输出时以一个完整的周期结束 */
    .idle_level = PWM_idle_low, /* 设置PWM空闲电平为低电平 */
    .accuracy_priority = PWM_accuracy_levels_first, /* 设置输出PWM时，优先满足pwm的级数 */
    .freq = 120000, /* 设置PWM频率为12kHz，在选择了优先满足PWM级数的情况下最终调制后的频率不等于120kHz */
    .levels = 5000, /* 设置PWM最大级数为5000 */
    .clk_id = "rtc", /* 设置PWM时钟源为RTC，可参考对应SOC中的clk.h,选填"pclk" " "rtc" "ext1"*/
};

void pwm_example(void)
{
    int ret;

    pwm_init();

    ret = pwm_request(&pwm_handle, GPIO_PC(00));
    if (ret)
        printf("pwm_request failed!\n");

    ret = pwm_config(&pwm_handle, &config);
    if (ret < 0)
        printf("pwm_config failed!\n");

    /* 如果有需要可以使用 pwm_get_freq 获取 PWM 最终调制后的频率 */
    printf("real freq: %ld\n", pwm_get_freq(&pwm_handle));

    pwm_set_level(&pwm_handle, 500);/* 设置 level 值，输出相对应的 PWM */

    mdelay(5000);

    pwm_set_level(&pwm_handle, 0);/* 当 level 值为0， 停止输出 PWM */
}

//////////////////////////////////////////
struct pwm_handle pwm_dma_handle;
struct pwm_data pwm_data[4];

struct pwm_dma_data pwm_dma_data = {
    .data = pwm_data,
    .data_count = 4,
    .dma_loop = 1,
    .idle_level = PWM_idle_low,
    .start_level = PWM_start_high,
    .src_rate = 1 * 1000 * 1000,
};

uint64_t get_count(uint64_t time_ns, unsigned long rate)
{
    return time_ns * rate / (1000 * 1000 * 1000);
}

void pwm_dma_example(void)
{
    int ret;
    unsigned long rate;

    pwm_init();

    ret = pwm_request(&pwm_dma_handle, GPIO_PC(00));
    if (ret)
        printf("pwm_request failed!\n");

    rate = pwm_dma_data.src_rate;
    printf("pwm dma rate = %ld\n", rate);

    /* 如果没有cache写回DDR的操作，则需要用uncache_mem_alloc申请pwm_data */
    pwm_data[0].high  = get_count(10 * 1000, rate);//10us
    pwm_data[0].low   = get_count(10 * 1000, rate);//10us
    pwm_data[1].high  = get_count(20 * 1000, rate);//20us
    pwm_data[1].low   = get_count(20 * 1000, rate);//20us
    pwm_data[2].high  = get_count(30 * 1000, rate);//30us
    pwm_data[2].low   = get_count(30 * 1000, rate);//30us
    pwm_data[3].high  = get_count(40 * 1000, rate);//40us
    pwm_data[3].low   = get_count(40 * 1000, rate);//40us

    pwm_dma_start(&pwm_dma_handle, &pwm_dma_data);

    mdelay(5000);//延时5s后关闭pwm

    pwm_dma_stop(&pwm_dma_handle);

    pwm_release(&pwm_dma_handle);
}


struct pwm_handle pwm0_handle;
struct pwm_handle pwm1_handle;

struct pwm_config_data pwm0_config = {
    .shutdown_mode = PWM_graceful_shutdown, /* 设置PWM停止输出时以一个完整的周期结束 */
    .idle_level = PWM_idle_low, /* 设置PWM空闲电平为低电平 */
    .accuracy_priority = PWM_accuracy_freq_first, /* 设置输出PWM时，优先满足pwm的频率 */
    .freq = 1000000, /* 设置PWM频率为1MHz */
    .levels = 500, /* 设置PWM最大级数为500 */
};

struct pwm_config_data pwm1_config = {
    .shutdown_mode = PWM_graceful_shutdown, /* 设置PWM停止输出时以一个完整的周期结束 */
    .idle_level = PWM_idle_low, /* 设置PWM空闲电平为低电平 */
    .accuracy_priority = PWM_accuracy_freq_first, /* 设置输出PWM时，优先满足pwm的频率 */
    .freq = 1000000, /* 设置PWM频率为1MHz */
    .levels = 100, /* 设置PWM最大级数为100 */
};

void pwm_multi_channel_sync_test(void)
{
    unsigned int channels = 0;

    pwm_init();

    pwm_request(&pwm0_handle, GPIO_PB(12));
    pwm_request(&pwm1_handle, GPIO_PB(13));

    pwm_config(&pwm0_handle, &pwm0_config);
    pwm_config(&pwm1_handle, &pwm1_config);

    pwm_set_not_really_enable(&pwm0_handle, 1);
    pwm_set_not_really_enable(&pwm1_handle, 1);

    pwm_set_not_really_disable(&pwm0_handle, 1);
    pwm_set_not_really_disable(&pwm1_handle, 1);

    pwm_set_level(&pwm0_handle, 100);
    pwm_set_level(&pwm1_handle, 50);

    channels |= (1 << pwm_get_id(&pwm0_handle));
    channels |= (1 << pwm_get_id(&pwm1_handle));

    pwm_enable_channels(channels);

    mdelay(5000);//延时5s后关闭pwm

    pwm_disable_channels(channels);

    pwm_release(&pwm0_handle);
    pwm_release(&pwm1_handle);
}


struct pwm_handle pwm0_dma_handle;
struct pwm_handle pwm1_dma_handle;

struct pwm_data pwm0_dma_data[4];
struct pwm_data pwm1_dma_data[4];

struct pwm_dma_data pwm0_dma_config = {
    .data = pwm0_dma_data,
    .data_count = 4,
    .dma_loop = 1,
    .idle_level = PWM_idle_low,
    .start_level = PWM_start_high,
    .src_rate = 1 * 1000 * 1000,
};

struct pwm_dma_data pwm1_dma_config = {
    .data = pwm1_dma_data,
    .data_count = 4,
    .dma_loop = 1,
    .idle_level = PWM_idle_low,
    .start_level = PWM_start_high,
    .src_rate = 1 * 1000 * 1000,
};

void pwm_dma_multi_channel_sync_test(void)
{
    unsigned long rate;
    unsigned int channels = 0;

    pwm_init();

    pwm_request(&pwm0_dma_handle, GPIO_PB(12));
    pwm_request(&pwm1_dma_handle, GPIO_PB(13));

    rate = pwm0_dma_config.src_rate;
    printf("pwm dma rate = %ld\n", rate);

    pwm_set_not_really_enable(&pwm0_dma_handle, 1);
    pwm_set_not_really_enable(&pwm1_dma_handle, 1);

    pwm_set_not_really_disable(&pwm0_dma_handle, 1);
    pwm_set_not_really_disable(&pwm1_dma_handle, 1);

    pwm0_dma_data[0].high  = get_count(10 * 1000, rate);//10us
    pwm0_dma_data[0].low   = get_count(10 * 1000, rate);//10us
    pwm0_dma_data[1].high  = get_count(20 * 1000, rate);//20us
    pwm0_dma_data[1].low   = get_count(20 * 1000, rate);//20us
    pwm0_dma_data[2].high  = get_count(30 * 1000, rate);//30us
    pwm0_dma_data[2].low   = get_count(30 * 1000, rate);//30us
    pwm0_dma_data[3].high  = get_count(40 * 1000, rate);//40us
    pwm0_dma_data[3].low   = get_count(40 * 1000, rate);//40us

    pwm1_dma_data[0].high  = get_count(40 * 1000, rate);//40us
    pwm1_dma_data[0].low   = get_count(40 * 1000, rate);//40us
    pwm1_dma_data[1].high  = get_count(30 * 1000, rate);//30us
    pwm1_dma_data[1].low   = get_count(30 * 1000, rate);//30us
    pwm1_dma_data[2].high  = get_count(20 * 1000, rate);//20us
    pwm1_dma_data[2].low   = get_count(20 * 1000, rate);//20us
    pwm1_dma_data[3].high  = get_count(10 * 1000, rate);//10us
    pwm1_dma_data[3].low   = get_count(10 * 1000, rate);//10us

    pwm_dma_start(&pwm0_dma_handle, &pwm0_dma_config);
    pwm_dma_start(&pwm1_dma_handle, &pwm1_dma_config);

    channels |= (1 << pwm_get_id(&pwm0_dma_handle));
    channels |= (1 << pwm_get_id(&pwm1_dma_handle));
    pwm_enable_channels(channels);

    mdelay(5000);//延时5s后关闭pwm

    pwm_disable_channels(channels);

    pwm_release(&pwm0_dma_handle);
    pwm_release(&pwm1_dma_handle);
}


static void dma_end_cb(void *data);

struct pwm_handle pwm0_queue_handle;
struct pwm_handle pwm1_queue_handle;

struct pwm_data pwm0_queue_data[100];
struct pwm_data pwm1_queue_data[40];

struct pwm_dma_data pwm0_queue_config = {
    .data = pwm0_queue_data,
    .data_count = 100,
    .idle_level = PWM_idle_low,
    .start_level = PWM_start_high,
    .dma_complete_cb = dma_end_cb,
    .src_rate = 1000 * 1000,
};

struct pwm_dma_data pwm1_queue_config = {
    .data = pwm1_queue_data,
    .data_count = 40,
    .idle_level = PWM_idle_low,
    .start_level = PWM_start_high,
    .dma_complete_cb = dma_end_cb,
    .src_rate = 1000 * 1000,
};

static void dma_end_cb(void *data)
{

}

void pwm_dma_queue_multi_channel_sync_test(void)
{
    unsigned long rate;
    unsigned int channels = 0;

    pwm_init();

    pwm_request(&pwm0_queue_handle, GPIO_PB(12));
    pwm_request(&pwm1_queue_handle, GPIO_PB(13));

    pwm_dma_queue_init(&pwm0_queue_handle, &pwm0_queue_config);
    pwm_dma_queue_init(&pwm1_queue_handle, &pwm1_queue_config);

    rate = pwm_dma_get_freq(&pwm0_queue_handle);
    printf("pwm dma rate = %ld\n", rate);

    pwm_set_not_really_enable(&pwm0_queue_handle, 1);
    pwm_set_not_really_enable(&pwm1_queue_handle, 1);

    pwm_set_not_really_disable(&pwm0_queue_handle, 1);
    pwm_set_not_really_disable(&pwm1_queue_handle, 1);

    int i;
    for (i = 0; i < 99; i++) {
        pwm0_queue_data[i].high = get_count(20 * 1000, rate);
        pwm0_queue_data[i].low = get_count(20 * 1000, rate);
    }

    pwm0_queue_data[i].high = get_count(1000 * 1000, rate);
    pwm0_queue_data[i].low = get_count(1000 * 1000, rate);

    for (i = 0; i < 39; i++) {
        pwm1_queue_data[i].high = get_count(20 * 1000, rate);
        pwm1_queue_data[i].low = get_count(20 * 1000, rate);
    }
    pwm1_queue_data[i].high = get_count(200 * 1000, rate);
    pwm1_queue_data[i].low = get_count(200 * 1000, rate);

    pwm_dma_queue_add(&pwm0_queue_handle, pwm1_queue_data, pwm1_queue_config.data_count);
    pwm_dma_queue_add(&pwm0_queue_handle, pwm1_queue_data, pwm1_queue_config.data_count);

    pwm_dma_queue_add(&pwm1_queue_handle, pwm0_queue_data, pwm0_queue_config.data_count);
    pwm_dma_queue_add(&pwm1_queue_handle, pwm0_queue_data, pwm0_queue_config.data_count);

    pwm_dma_queue_start(&pwm0_queue_handle);
    pwm_dma_queue_start(&pwm1_queue_handle);

    channels |= (1 << pwm_get_id(&pwm0_queue_handle));
    channels |= (1 << pwm_get_id(&pwm1_queue_handle));
    pwm_enable_channels(channels);

    int pwm0_count = 0;
    int pwm1_count = 0;
    while (pwm0_count < 1000 && pwm1_count < 1000) {
        if (pwm_dma_queue_get_available_size(&pwm0_queue_handle)) {
            if ((pwm0_count++) % 2 == 0)
                pwm_dma_queue_add(&pwm0_queue_handle, pwm0_queue_data, pwm0_queue_config.data_count);
            else
                pwm_dma_queue_add(&pwm0_queue_handle, pwm1_queue_data, pwm1_queue_config.data_count);
        }
        if (pwm_dma_queue_get_available_size(&pwm1_queue_handle)) {
            if ((pwm1_count++) % 2 == 0)
                pwm_dma_queue_add(&pwm1_queue_handle, pwm1_queue_data, pwm1_queue_config.data_count);
            else
                pwm_dma_queue_add(&pwm1_queue_handle, pwm0_queue_data, pwm0_queue_config.data_count);
        }
    }

    pwm_disable_channels(channels);

    pwm_dma_queue_stop(&pwm0_queue_handle);
    pwm_dma_queue_stop(&pwm1_queue_handle);

    pwm_release(&pwm0_queue_handle);
    pwm_release(&pwm1_queue_handle);

    printf("pwm dma stop\n");
}
//////////////////////////////////////////