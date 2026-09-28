#ifndef _SOC_TCU_H_
#define _SOC_TCU_H_

#include <bits_opt.h>

#define TCU_CHANNEL_NUM   7
#define TCU_FULL_NUM 0xffff
#define TCU_HALF_NUM 0x7000

enum tcu_work_mode {
    not_config,
    general_mode,
    gate_mode,
    direction_mode,
    quadrature_mode,
    pos_mode,
    capture_mode,
    filter_mode
};

enum tcu_clk_div {
    clk_div_1,
    clk_div_4,
    clk_div_16,
    clk_div_64,
    clk_div_256,
    clk_div_1024
};

enum tcu_gpio0_mode {
    gpio0_not_use,
    gpio0_as_capture = 1,
    gpio0_as_rising_edge_sync = 1,
    gpio0_as_falling_edge_sync = 3,
    gpio0_as_rising_edge_clk = BIT(18),
    gpio0_as_falling_edge_clk = BIT(19),
    gpio0_as_rising_falling_edge_clk = BIT(18) | BIT(19),
};

enum tcu_gpio1_mode {
    gpio1_not_use,
    gpio1_as_capture = 2,
    gpio1_as_rising_edge_sync = 2,
    gpio1_as_rising_edge_clk = BIT(20),
    gpio1_as_falling_edge_clk = BIT(21),
    gpio1_as_rising_falling_edge_clk = BIT(20) | BIT(21)
};

enum tcu_extclk_mode {
    ext_not_use,
    ext_as_rising_edge_clk = BIT(16),
    ext_as_falling_edge_clk = BIT(17),
    ext_as_rising_falling_edge_clk = BIT(16) | BIT(17)
};

struct tcu_config {
    int id;     /*tcu channel number */
    int enable_irq;
    unsigned short full_value;
    void (*irq_cb)(int id);
    int gpio0, gpio1;

    enum tcu_clk_div clk_div;
    enum tcu_work_mode work_mode;
    enum tcu_gpio0_mode gpio0_mode;
    enum tcu_gpio1_mode gpio1_mode;
    enum tcu_extclk_mode extclk_mode;
};

/* 获得tcu 的计数值 */
int tcu_get_count(struct tcu_config *config);

/* 设置tcu 的最大计数值,达到最大计数值tcu 会从0开始计数 */
void tcu_set_full_value(struct tcu_config *config, unsigned short full_value);

/* 将ext clk作为输入时钟源, 计数, 默认使能中断*/
void tcu_config_timer_count(struct tcu_config *config, int id,
     unsigned short full_value, void (*irq_cb)(int id));

/* 将gpio0作为输入时钟源, 在上升沿的时候计数器计数*/
void tcu_config_gpio0_up_count(struct tcu_config *tcu_config, int tcu_id);

/* 将gpio1作为输入时钟源,在上升沿的时候计数器计数*/
void tcu_config_gpio1_up_count(struct tcu_config *tcu_config, int tcu_id);

/* 将gpio0作为输入时钟源,在上升沿的时候计数器计数
 * 将gpio1设为清零信号，在上升沿的时候清零计数值
 */
void tcu_config_gpio0_up_count_gpio1_rising_sync(struct tcu_config *tcu_config, int tcu_id);

/* 将gpio1作为输入时钟源,在上升沿的时候计数器计数
 * 将GPIO0设为清零信号，在上升沿的时候清零计数值
 */
void tcu_config_gpio1_up_count_gpio0_rising_sync(struct tcu_config *tcu_config, int tcu_id);

/* 将gpio1作为输入时钟源,在上升沿的时候计数器计数
 * 将GPIO0设为清零信号，在下降沿的时候清零计数值
 */
void tcu_config_gpio1_up_count_gpio0_falling_sync(struct tcu_config *tcu_config, int tcu_id);

/* 正交双向计数 */
void tcu_config_quadrature_gpio_bothway_count(struct tcu_config *tcu_config, int tcu_id);

/* 对gpio0进行捕获,获得周期以及高电平时间,
 * 将gpio1作为输入时钟源
*/
void tcu_config_capture_gpio0_gpio1_srcclk(struct tcu_config *tcu_config, int tcu_id);

/* 对gpio1进行捕获,获得周期以及高电平时间,
 * 将gpio0作为输入时钟源
 */
void tcu_config_capture_gpio1_gpio0_srcclk(struct tcu_config *tcu_config, int tcu_id);

/* 对gpio0进行捕获,获得周期以及高电平时间,
 * 将外部时钟ext作为输入时钟源
 */
void tcu_config_capture_gpio0_ext_srcclk(struct tcu_config *tcu_config, int tcu_id, enum tcu_clk_div clk_div, int en_irq);

/* 对gpio1进行捕获,获得周期以及高电平时间,
 * 将外部时钟ext作为输入时钟源
 */
void tcu_config_capture_gpio1_ext_srcclk(struct tcu_config *tcu_config, int tcu_id, enum tcu_clk_div clk_div, int en_irq);

#endif /* _SOC_TCU_H_ */