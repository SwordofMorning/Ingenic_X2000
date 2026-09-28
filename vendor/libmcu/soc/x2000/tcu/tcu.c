
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <driver/irq.h>
#include <driver/gpio.h>
#include <cpu/ffs.h>
#include <driver/tcu.h>
#include <driver/clk.h>
#include <soc/tcu.h>
#include "tcu_regs.h"

struct tcu_irq_event {
    enum tcu_work_mode work_mode;
    int capture_success;
    void (*irq_cb)(int id);
};

struct tcu_irq_event chn_irq_event[NR_TCU_CHNS] = {{0}};
static int tcu_irq;

#ifdef TCU_DEBUG
void tcu_dump_reg(struct tcu_config *config)
{
    static int count = 0;
    count ++;
    printf("\n\n----------------------------------------count N0.%d----------------------------------------------------\n\n",count);
    printf("-stop-----addr-%08x-value-%08x-----------\n", (unsigned int)(TCU_TSR), tcu_read_reg(TCU_TSR));
    printf("-mask-----addr-%08x-value-%08x-----------\n", (unsigned int)(TCU_TMR), tcu_read_reg(TCU_TMR));
    printf("-enable---addr-%08x-value-%08x-----------\n", (unsigned int)(TCU_TER), tcu_read_reg(TCU_TER));
    printf("-flag-----addr-%08x-value-%08x-----------\n", (unsigned int)(TCU_TFR), tcu_read_reg(TCU_TFR));
    printf("-Control--addr-%08x-value-%08x-----------\n", \
    (unsigned int)(TCU_ADDR(TCU_FULL0 + config->id * TCU_CHN_OFFSET + CHN_TCSR)), tcu_read_chn_reg(config,CHN_TCSR));
    printf("-full-----addr-%08x-value-%08x-----------\n", \
    (unsigned int)(TCU_ADDR(TCU_FULL0 + config->id * TCU_CHN_OFFSET + CHN_TDFR)), tcu_read_chn_reg(config,CHN_TDFR));
    printf("-half-   -addr-%08x-value-%08x-----------\n", \
    (unsigned int)(TCU_ADDR(TCU_FULL0 + config->id * TCU_CHN_OFFSET + CHN_TDHR)), tcu_read_chn_reg(config,CHN_TDHR));
    printf("-TCNT-----addr-%08x-value-%08x-----------\n", \
    (unsigned int)(TCU_ADDR(TCU_FULL0 + config->id * TCU_CHN_OFFSET + CHN_TCNT)), tcu_read_chn_reg(config,CHN_TCNT));
    printf("-CAP ---------------value-%08x-----------\n", tcu_read_reg(CHN_CAP(config->id)));
    printf("-CAP_VAL register---value-%08x-----------\n", tcu_read_reg(CHN_CAP_VAL(config->id)));
}
#endif

static inline void tcu_set_control_register(struct tcu_config *config)
{
    int dir_sel = 0;

    /* 使能ext/gpio0/gpio1 功能*/
    int en_gpio0 = !!config->gpio0_mode;
    int en_gpio1 = !!config->gpio1_mode;
    int en_ext = !!config->extclk_mode;

    /* 设置时钟极性 */
    int clk_polarity = (config->gpio0_mode | config->gpio1_mode | config->extclk_mode) & (0x3f << 16);

    /* 设置正交计数模式 */
    if (config->work_mode == quadrature_mode)
        dir_sel = 4; // quadrature mode

    unsigned int tcsr = (en_ext << 2) | (en_gpio0 << 6) | (en_gpio1 << 7) | (config->clk_div << 3) | clk_polarity | dir_sel << 8;
    tcu_write_chn_reg(config, CHN_TCSR, tcsr);
}

static inline void tcu_set_capture_register(struct tcu_config *config)
{
    unsigned int capture_gpio;
    unsigned int capture_num;

    /* 设置捕获引脚或 pos模式sync清零引脚 */
    capture_gpio = (config->gpio0_mode | config->gpio1_mode) & 0x03;

    /* 使能捕获模式 capture_num捕获的周期数，  capture_num > 0x80 会一直捕获 */
    if (config->work_mode == capture_mode)
        capture_num = 0xa0;
    else
        capture_num = 0;

    tcu_set_bit(CHN_CAP(config->id), 16, 18, capture_gpio);
    tcu_set_bit(CHN_CAP(config->id), 0, 7, capture_num);
}

static inline void tcu_disble_capture(struct tcu_config *config)
{
    /* 捕获次数为1会停止捕获 */
    tcu_set_bit(CHN_CAP(config->id), 0, 7, 0x01);
}

static inline void tcu_get_period_high_level(unsigned int id, int *high_level_time, int *period_time)
{
    /* 得到捕获到的周期以及高电平计数值 */
    *high_level_time = tcu_get_bit(CHN_CAP_VAL(id), 0, 15);
    *period_time = tcu_get_bit(CHN_CAP_VAL(id), 16, 31);
}

static void tcu_interrupt(int irq, void *data)
{
    // watchdog
    // if (tcu_read_reg(TCU_TFR) & TCU_FLAG_RD)
    //     return;

    int id;
    id = __ffs(tcu_read_reg(TCU_TFR) & 0xff);

    tcu_clear_full_flag(id);
    if (chn_irq_event[id].work_mode == capture_mode)
        chn_irq_event[id].capture_success = 1;
    if (chn_irq_event[id].irq_cb)
        chn_irq_event[id].irq_cb(id);
}

void tcu_config_timer_count(struct tcu_config *config, int id, 
     unsigned short full_value, void (*irq_cb)(int id))
{
    memset(config, 0, sizeof(struct tcu_config));
    config->id = id;
    config->work_mode = general_mode;
    config->irq_cb = irq_cb;
    config->enable_irq = 1;
    config->full_value = full_value;
    config->extclk_mode = ext_as_rising_edge_clk;
}

void tcu_config_quadrature_gpio_bothway_count(struct tcu_config *config, int id)
{
    memset(config, 0, sizeof(struct tcu_config));
    config->id = id;
    config->work_mode = quadrature_mode;
    config->gpio0_mode = gpio0_as_rising_falling_edge_clk;
    config->gpio1_mode = gpio1_as_rising_falling_edge_clk;
}

void tcu_config_capture_gpio0_gpio1_srcclk(struct tcu_config *config, int id)
{
    memset(config, 0, sizeof(struct tcu_config));
    config->id = id;
    config->work_mode = capture_mode;
    config->gpio0_mode = gpio0_as_capture;
    config->gpio1_mode = gpio1_as_rising_edge_clk;
}

void tcu_config_capture_gpio1_gpio0_srcclk(struct tcu_config *config, int id)
{
    memset(config, 0, sizeof(struct tcu_config));
    config->id = id;
    config->work_mode = capture_mode;
    config->gpio0_mode = gpio0_as_rising_edge_clk;
    config->gpio1_mode = gpio1_as_capture;
}

void tcu_config_capture_gpio0_ext_srcclk(struct tcu_config *config, int id, enum tcu_clk_div clk_div, int en_irq)
{
    memset(config, 0, sizeof(struct tcu_config));
    config->id = id;
    config->enable_irq = en_irq;
    config->clk_div = clk_div;

    config->work_mode = capture_mode;
    chn_irq_event[id].work_mode = capture_mode;
    config->gpio0_mode = gpio0_as_capture;
    config->extclk_mode = ext_as_rising_edge_clk;
}

void tcu_config_capture_gpio1_ext_srcclk(struct tcu_config *config, int id, enum tcu_clk_div clk_div, int en_irq)
{
    memset(config, 0, sizeof(struct tcu_config));
    config->id = id;
    config->enable_irq = en_irq;
    config->clk_div = clk_div;

    config->work_mode = capture_mode;
    chn_irq_event[id].work_mode = capture_mode;
    config->gpio1_mode = gpio1_as_capture;
    config->extclk_mode = ext_as_rising_edge_clk;
}

void tcu_config_gpio0_up_count(struct tcu_config *config, int id)
{
    memset(config, 0, sizeof(struct tcu_config));
    config->id = id;
    config->work_mode = pos_mode;
    config->gpio0_mode = gpio0_as_rising_edge_clk;
}

void tcu_config_gpio1_up_count(struct tcu_config *config, int id)
{
    memset(config, 0, sizeof(struct tcu_config));
    config->id = id;
    config->work_mode = pos_mode;
    config->gpio1_mode = gpio1_as_rising_edge_clk;
}

void tcu_config_gpio0_up_count_gpio1_rising_sync(struct tcu_config *config, int id)
{
    memset(config, 0, sizeof(struct tcu_config));
    config->id = id;
    config->work_mode = pos_mode;
    config->gpio0_mode = gpio0_as_rising_edge_clk;
    config->gpio1_mode = gpio1_as_rising_edge_sync;
}

void tcu_config_gpio1_up_count_gpio0_rising_sync(struct tcu_config *config, int id)
{
    memset(config, 0, sizeof(struct tcu_config));
    config->id = id;
    config->work_mode = pos_mode;
    config->gpio1_mode = gpio1_as_rising_edge_clk;
    config->gpio0_mode = gpio0_as_rising_edge_sync;
}

void tcu_config_gpio1_up_count_gpio0_falling_sync(struct tcu_config *config, int id)
{
    memset(config, 0, sizeof(struct tcu_config));
    config->work_mode = pos_mode;
    config->gpio1_mode = gpio1_as_rising_edge_clk;
    config->gpio0_mode = gpio0_as_falling_edge_sync;
}

static void tcu_config_mode(struct tcu_config *config)
{
    /* set control/capture register */
    tcu_set_control_register(config);
    tcu_set_capture_register(config);

    /* set full event */
    tcu_clear_full_flag(config->id);
    tcu_set_chn_full(config, config->full_value ? : 0xffff);

#ifdef TCU_DEBUG
        tcu_dump_reg(config);
#endif
    return ;
}

static void tcu_gpio_init(struct tcu_config *config)
{
    if (config->gpio0_mode != gpio0_not_use)
        gpio_set_func(GPIO_PC(config->id*2), GPIO_FUNC_0);

    if (config->gpio1_mode != gpio1_not_use)
        gpio_set_func(GPIO_PC(config->id*2 + 1), GPIO_FUNC_0);
}

int tcu_enable(struct tcu_config *config)
{
    if (config->work_mode == not_config)
        return -1;

    tcu_disable(config);

    tcu_gpio_init(config);

    tcu_config_mode(config);

    if (config->enable_irq) {
        chn_irq_event[config->id].irq_cb = config->irq_cb;
        tcu_full_enable(config->id);
        enable_irq(tcu_irq);
    }

    if (config->work_mode != capture_mode) {
        tcu_enable_counter(config->id);
        tcu_start_counter(config->id);
    }

#ifdef TCU_DEBUG
    tcu_dump_reg(config);
#endif
    return 0;
}

int tcu_disable(struct tcu_config *config)
{
    if (config->work_mode == not_config)
        return -1;

    if (config->enable_irq) {
        disable_irq(tcu_irq);
        tcu_full_disable(config->id);
    }

    if (config->work_mode != capture_mode) {

        tcu_disable_counter(config->id);
        tcu_clear_count(config); //先清零后停止,不然先停止再清零就清不了
        tcu_stop_counter(config->id);

    } else {
        tcu_disble_capture(config);
    }

#ifdef TCU_DEBUG
        tcu_dump_reg(config);
#endif
    return 0;
}

int tcu_get_capture(struct tcu_config *config, int *high_level_time, int *period_time)
{
    if (!chn_irq_event[config->id].capture_success)
        return -1;

    tcu_get_period_high_level(config->id, high_level_time, period_time);
    chn_irq_event[config->id].capture_success = 0;
    return 0;
}

int tcu_get_capture_noirq(struct tcu_config *config, int *high_level_time, int *period_time)
{
    unsigned int val = tcu_read_reg(TCU_TFR) & 0xff;
    if ( !(val & (1 << config->id)) )
        return -1;

    tcu_clear_full_flag(config->id);
    tcu_get_period_high_level(config->id, high_level_time, period_time);
    return 0;
}

int tcu_get_count(struct tcu_config *config)
{
    return tcu_read_chn_reg(config, CHN_TCNT);
}

void tcu_set_full_value(struct tcu_config *config, unsigned short full_value)
{
    tcu_set_chn_full(config, full_value);
    config->full_value = full_value;
}

int tcu_init(void)
{
    tcu_irq = IRQ_TCU0;
    request_irq_disabled(tcu_irq, IRQ_TYPE_LEVEL_LOW, tcu_interrupt, "tcu-interrupt", NULL);
    clk_enable(CLK_GATE_TCU, 1);

    return 0;
}

void tcu_deinit(void)
{
    release_irq(tcu_irq);
}