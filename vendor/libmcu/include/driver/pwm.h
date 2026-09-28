#ifndef _PWM_H_
#define _PWM_H_

#include <soc/pwm.h>

enum pwm_shutdown_mode {
    /**
     * pwm停止输出时,尽量保证pwm的信号结尾是一个完整的周期
     */
    PWM_graceful_shutdown,
    /**
     * pwm停止输出时,立刻将pwm设置成空闲时电平
     */
    PWM_abrupt_shutdown,
};

enum pwm_idle_level {
    /**
     * pwm 空闲时电平为低
     */
    PWM_idle_low,
    /**
     * pwm 空闲时电平为高
     */
    PWM_idle_high,
};

enum pwm_accuracy_priority {
    /**
     * 优先满足pwm的目标频率的精度,级数可能不准确
     */
    PWM_accuracy_freq_first,
    /**
     * 优先满足pwm的级数设置,pwm频率可能不准确
     */
    PWM_accuracy_levels_first,
};

struct pwm_config_data {
    enum pwm_shutdown_mode shutdown_mode;
    enum pwm_idle_level idle_level;
    enum pwm_accuracy_priority accuracy_priority;
    char *clk_id;   /* 指定时钟源ID，可参考对应SOC中的clk.h,比如 "rtc" "ext1" */
    unsigned long freq;
    unsigned long levels;
};

int pwm_init(void);

/**
 * 申请 PWM 资源
 * gpio: 指定 GPIO 号
 * pwmdata: PWM 信息结构体
 * 返回值: 0表示成功, 其他表示失败
 */
int pwm_request(struct pwm_handle *pwmdata, int gpio);

/**
 * 设置 PWM 调制后的频率和周期级数
 * pwmdata: PWM 信息结构体
 * config: pwm 的配置数据
 * 返回值: 0表示成功, 其他表示失败
 */
int pwm_config(struct pwm_handle *pwmdata, struct pwm_config_data *config);

/**
 * 释放 PWM 资源, 无返回值
 * pwmdata: PWM 信息结构体
 */
void pwm_release(struct pwm_handle *pwmdata);

/**
 * 设置 PWM 调制的级数
 * pwmdata: PWM 信息结构体
 * level: pwm 调制的级数,即一个周期内非空闲电平的长度
 *
 * NOTE: 当 level >  0 时，PWM 调制波形会输出
 *       当 level == 0 时, PWM 调制波形会停止输出
 *
 */
int pwm_set_level(struct pwm_handle *pwmdata, unsigned long level);

/**
 * 获取 PWM 调制后频率
 * pwmdata: PWM 信息结构体
 *
 * 返回值: PWM 调制后频率
 *
 */
unsigned long pwm_get_freq(struct pwm_handle *pwmdata);

/**
 * 获取 pwm_handle 对应的 id
 * pwmdata: PWM 信息结构体
 * 返回值: PWM 对应的 id
 */
int pwm_get_id(struct pwm_handle *pwmdata);


/* ---- dma mode ---- */

enum pwm_dma_start_level {
    PWM_start_low,   /* pwm dma模式的起始电平为低 */
    PWM_start_high,  /* pwm dma模式的起始电平为高 */
};

/*
    请确保dma数据的高低电平不能有零计数，
    如果不能确保，手动把宏定义打开。
 */
//#define PWM_CHECK_DMA_DATA

/*
    注意: high和low不能为零
        时间单位由pwm2_dma_init返回
 */
struct pwm_data {
    /* 低电平个数 */
    unsigned low:16;
    /* 高电平个数 */
    unsigned high:16;
};

struct pwm_dma_data {
    struct pwm_data *data;
    enum pwm_idle_level idle_level;
    enum pwm_dma_start_level start_level;
    unsigned int data_count;
    unsigned int dma_loop;
    void (*dma_complete_cb)(void *data);
    unsigned long src_rate;
};

/**
 * 初始化 pwm 的 dma 模式
 * pwmdata: PWM 信息结构体
 * dma_data:DMA 传输信息
 * 返回值: 0表示成功, 其他表示失败
 */
int pwm_dma_init(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data);

/**
 * 使用dma模式连续更新pwm的频率
 * 普通dma模式: 需要调用pwm_dma_wait_end等待dma传输完成(并不是波形输出完成)
 * 循环dma模式：函数不会阻塞，需要调用pwm_dma_stop(pwm_release)停止dma
 * pwmdata: PWM 信息结构体
 * dma_data:DMA 传输信息
 * 返回值: 0表示成功, 其他表示失败
 */
int pwm_dma_update(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data);

/**
 * 开始 pwm dma 传输(相当于依次调用pwm_dma_init 和 pwm_dma_update)
 * pwmdata: PWM 信息结构体
 * dma_data:DMA 传输信息
 * 返回值: 0表示成功, 其他表示失败
 *
 */
int pwm_dma_start(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data);

/**
 * 等待 pwm dma 传输完成，用于非循环dma模式下阻塞:函数会阻塞到dma数据全部转换成对应pwm输出
 * pwmdata: PWM 信息结构体
 * dma_data:DMA 传输信息
 * 无返回值
 */
void pwm_dma_wait_end(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data);

/**
 * 停止 pwm dma 传输
 * pwmdata: PWM 信息结构体
 * 返回值: 0表示成功, 其他表示失败
 *
 */
int pwm_dma_stop(struct pwm_handle *pwmdata);

/**
 * 获取 pwm dma 模式频率
 * pwmdata: PWM 信息结构体
 * 返回值: pwm dma 模式频率
 *
 */
unsigned long pwm_dma_get_freq(struct pwm_handle *pwmdata);

/**
 * 用于多个通道同时开启的使能，开启后在未调用pwm_enable_channels之前不会开始输出波形，
 * pwmdata: PWM 信息结构体
 * enable: 是否开启
 * 无返回值
 */
void pwm_set_not_really_enable(struct pwm_handle *pwmdata, int enable);

/**
 * 用于多个通道同时开启的失能
 * pwmdata: PWM 信息结构体
 * enable: 是否开启
 * 无返回值
 */
void pwm_set_not_really_disable(struct pwm_handle *pwmdata, int enable);

/**
 * 多通道同时开启
 * channels: 需要启动的通道，每位bit对应通道号
 * 无返回值
 */
void pwm_enable_channels(unsigned int channels);

/**
 * 多通道同时关闭
 * channels: 需要启动的通道，每位bit对应通道号
 * 无返回值
 */
void pwm_disable_channels(unsigned int channels);

int pwm_dma_queue_init(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data);

int pwm_dma_queue_add(struct pwm_handle *pwmdata, void *buf, unsigned int count);

void pwm_dma_queue_start(struct pwm_handle *pwmdata);

int pwm_dma_queue_get_available_size(struct pwm_handle *pwmdata);

int pwm_dma_queue_stop(struct pwm_handle *pwmdata);

#endif