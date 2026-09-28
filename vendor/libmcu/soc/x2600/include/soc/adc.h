#ifndef _SOC_ADC_H_
#define _SOC_ADC_H_

#include <driver/dma.h>

/**********************************************************************
 * adc 包含16个采样通道,采样深度12bit
 * adc 包含三个采样序列:seq0 seq1 seq2.
 * 每个采样序列可以配置多个采样通道和采样顺序
 * 三个采样序列可以配置优先级,从而在触发时决定执行顺序
 * 三个采样序列的采样率是一样的, 都由 adc_clk/15 决定
 * seq0:
 *      最大支持4个通道,依次进行采样
 *      支持采样结束中断
 *      支持软件触发采样
 *      不支持tcu,gpio事件触发
 *      不支持延时采样功能,采样完一个通道,立即开始下一个通道采样
 *      不支持自动连续采样
 *      不支持dma,
 *
 * seq1:
 *      最大支持16个通道,依次进行采样
 *      支持采样结束中断
 *      支持延时采样,每个通道采样完之后可经过延时,再采样下一个通道
 *      支持自动连续采样,序列完成采样之后,自动重新开始采样
 *      连续采样是分组的,最多可分8组,每组可以定义一个延时,这是分组的核心需求
 *      可以只分一组,设置一个延时,用作简单的连续采样周期
 *      支持tcu 半/满事件触发, 支持外部gpio触发, 支持软件触发采样
 *      支持dma
 *
 * seq2:
 *      最大支持8个通道,依次进行采样
 *      支持采样结束中断
 *      有延时采样功能,采样完一个通道,可以进行延时再进行下一个通道的采样
 *      支持tcu 半/满事件触发, 支持外部gpio触发, 支持软件触发采样
 *      不支持自动连续采样
 *      支持dma
 *
 * adc_src_clk 由 clk CLK_DIV_SADC 得到
 * adc_clk 是adc 的工作时钟, adc_clk = adc_src_clk / adc_clk_div
 * 假设 adc_src_clk = 60M, adc_clk_div = 2
 * 那么adc_clk = 30M, 那么 adc采样率 = 30M/15= 2M/s
 *
 * 延时计数时钟 adc_delay_clk = adc_clk / adc_delay_clk_div
 * adc_delay_clk = adc_clk / adc_delay_clk_div
 * adc_continus_clk = adc_clk / adc_continus_clk_div
 *
 * 假设 adc_delay_clk_div = 30,
 * 那么 adc_delay_clk = 1M, 那么 adc delay 的间隔单位是1us
 * 那么 adc_delay_clk 从 1us 到 65.536 ms
 *
 */

enum adc_trigger_type {
    adc_trigger_tcu0_half,
    adc_trigger_tcu0_full,
    adc_trigger_tcu1_half,
    adc_trigger_tcu1_full,
    adc_trigger_tcu2_half,
    adc_trigger_tcu2_full,
    adc_trigger_tcu3_half,
    adc_trigger_tcu3_full,
    adc_trigger_tcu4_half,
    adc_trigger_tcu4_full,
    adc_trigger_tcu5_half,
    adc_trigger_tcu5_full,
    adc_trigger_tcu6_half,
    adc_trigger_tcu6_full,
    adc_trigger_tcu7_half,
    adc_trigger_tcu7_full,
    adc_trigger_gpio_rising_edge,
    adc_trigger_gpio_falling_edge,
    adc_trigger_gpio_both_edge,
    adc_trigger_software,
};

struct adc_seq0_config {
    unsigned char channel_cnt;        // adc 采样序列使用的adc 通道总数
    unsigned char channels[4];        // 采样序列依次的通道号
    void (*irq_cb)(void);             // 中断回调函数,当一个转换序列完成之后回调,回调中需要读取adc数据
};

struct adc_seq1_config {
    int continus_clk_div; // 连续采样模式的计数时钟 = adc_clk / continus_clk_div; 最大值 2的24次方
    int delay_clk_div;    // 采样延时计数时钟 = adc_clk / delay_clk_div; 最大值 2的24次方
    enum adc_trigger_type trigger;     // adc 采样触发类型
    unsigned char enable_channel_num;  // 使能采样通道号,保存在数据的12-15 4bit位
    unsigned char channel_cnt;         // adc 采样序列使用的adc 通道总数
    unsigned char channels[16];        // 采样序列依次的通道号
    unsigned short channel_delays[16];         // 采样序列每个通道的延时, 单位是 adc delay clk

    unsigned char group_cnt;  // 连续采样模式下分组的个数,0表示不使能连续采样
    unsigned char groups[8];  // 分组模式依次对应每组通道的个数
    unsigned short group_delays[8]; // 每个分组转换完之后的延时, 单位是 adc cont clk

    void (*irq_cb)(void); // 中断回调函数,当一个转换序列完成之后回调,回调中需要读取adc数据

    unsigned int dma_mode;
    unsigned short *dma_buf;
    unsigned int dma_size;
};

struct adc_seq2_config {
    int delay_clk_div;    // 采样延时计数时钟 = adc_clk / delay_clk_div; 最大值 2的24次方
    enum adc_trigger_type trigger;     // adc 采样触发类型
    unsigned char enable_channel_num;  // 使能采样通道号,保存在数据的12-15 4bit位
    unsigned char channel_cnt;         // adc 采样序列使用的adc 通道总数
    unsigned char channels[8];        // 采样序列依次的通道号
    unsigned char channel_delays[8];         // 采样序列每个通道的延时, 单位是 adc delay clk

    void (*irq_cb)(void); // 中断回调函数,当一个转换序列完成之后回调,回调中需要读取adc数据

    unsigned int dma_mode;
    unsigned short *dma_buf;
    unsigned int dma_size;
};

/**
 * low_flags 保存低于 low_threshold 触发中断的通道 [0，15]
 * high_flags 保存高于 high_threshold 触发中断的通道 [0，15]
 */
typedef void (*adc_awd_cb)(unsigned short low_flags, unsigned short high_flags);

/**
 * 设置adc_clk, adc_clk=src_clk_rate/div, adc_clk 最大30M, div 最小是2
 * 默认值 adc_clk = 60M/2 = 30M
 */
void adc_set_clk(int src_clk_rate, int div);

/**
 * 设置采样序列优先级, 可选值 0, 1, 2, 值越大优先级越高
 * high_break_low 表示低优先级序列未完成时是否可以被高优先级打断, 1表示打断 0不打断
 */
void adc_set_seq_priority(
    char seq0_pri, char seq1_pri, char seq2_pri, int high_break_low);

/**
 * 使能seq0 采样序列,详细配置信息见 struct adc_seq0_config
 */
void adc_enable_seq0(struct adc_seq0_config *cfg);

/**
 * 失能 seq0 采样序列
 */
void adc_disable_seq0(struct adc_seq0_config *cfg);

/**
 * 触发 seq0 采样
 */
void adc_start_seq0(void);

/**
 * irq_cb 为NULL时,使用此函数查询seq0采样序列数据是否准备好
 */
int adc_poll_seq0_data_ready(void);

/**
 * 读取seq0的采样序列的数据, len 数组大小必须是采样序列的长度/通道数
 */
void adc_read_seq0_data(unsigned short *values, int len);

/**
 * 使能seq1 采样序列,详细配置信息见 struct adc_seq1_config
 */
void adc_enable_seq1(struct adc_seq1_config *cfg);

/**
 * trigger == adc_trigger_software 时,使用本函数触发adc seq1 的采样
 */
void adc_start_seq1(void);

/**
 * irq_cb 为NULL, dma_mode 为1时, 使用此函数查询seq1采样序列数据是否准备好
 */
int adc_dma_poll_seq1_data_ready(struct adc_seq1_config *cfg);

/**
 * 读取当前seq1可读的采样序列的数据个数, 单位为byte
 */
int adc_dma_seq1_get_readable_size(struct adc_seq1_config *cfg);

/**
 * 读取seq1的采样序列的数据, 返回读到的采样序列的数据个数, 单位为byte
 */
unsigned int adc_dma_seq1_read_data(struct adc_seq1_config *cfg, unsigned short *data);

/**
 * irq_cb 为NULL时,使用此函数查询seq1采样序列数据是否准备好
 */
int adc_poll_seq1_data_ready(void);

/**
 * 读取seq1的采样序列的数据, len 数组大小必须是采样序列的长度/通道数
 */
void adc_read_seq1_data(unsigned short *values, int len);

/**
 * 失能 seq1 采样序列
 */
void adc_disable_seq1(struct adc_seq1_config *cfg);

/**
 * 使能seq2 采样序列,详细配置信息见 struct adc_seq2_config
 */
void adc_enable_seq2(struct adc_seq2_config *cfg);

/**
 * trigger == adc_trigger_software 时,使用本函数触发adc seq1 的采样
 */
void adc_start_seq2(void);

/**
 * irq_cb 为NULL, dma_mode 为1时, 使用此函数查询seq2采样序列数据是否准备好
 */
int adc_dma_poll_seq2_data_ready(struct adc_seq2_config *cfg);

/**
 * 读取当前seq2可读的采样序列的数据个数, 单位为byte
 */
int adc_dma_seq2_get_readable_size(struct adc_seq2_config *cfg);

/**
 * 读取seq2的采样序列的数据, 返回读到的采样序列的数据个数, 单位为byte
 */
unsigned int adc_dma_seq2_read_data(struct adc_seq2_config *cfg, unsigned short *data);

/**
 * irq_cb 为NULL时,使用此函数查询seq1采样序列数据是否准备好
 */
int adc_poll_seq2_data_ready(void);

/**
 * 读取seq1的采样序列的数据, len 数组大小必须是采样序列的长度/通道数
 */
void adc_read_seq2_data(unsigned short *values, int len);

/**
 * 失能 seq1 采样序列
 */
void adc_disable_seq2(struct adc_seq2_config *cfg);

/**
 * 使能对应通道的 awd 功能
 * 需要设置seqn采集所需通道，才能触发对应通道的 awd 功能，本身不具备采样功能
 */
void adc_enable_awd(int channel, int low_threshold, int high_threshold);

/**
 * 失能对应通道的 awd 功能
 */
void adc_disable_awd(int channel);

/**
 * 设置 awd 的中断回调函数
 */
void adc_set_awd_cb(adc_awd_cb cb);

#endif /* _SOC_ADC_H_ */
