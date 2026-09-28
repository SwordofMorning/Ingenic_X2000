#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <irq.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/ctype.h>
#include <linux/syscore_ops.h>
#include <linux/mutex.h>
#include <asm/div64.h>
#include <linux/mfd/core.h>
#include <linux/clk.h>
#include <linux/of_clk.h>
#include <linux/of_irq.h>
#include <soc/gpio.h>
#include <utils/gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <utils/clock.h>
#include <bit_field.h>

#include "tcu.h"

#define IRQ_TCU0           (26)
#define IRQ_TCU1           (25)

#define GPIO_COUNTER_GET_MODE_NUM                       _IOWR('t', 194, unsigned long *)
#define GPIO_COUNTER_GET_MODE_NAME                      _IOWR('t', 195, unsigned long *)
#define GPIO_COUNTER_CONFIG                             _IOWR('t', 196, unsigned long *)
#define GPIO_COUNTER_ENABLE                             _IOWR('t', 197, unsigned int)
#define GPIO_COUNTER_DISABLE                            _IOWR('t', 198, unsigned int)
#define GPIO_COUNTER_GET_COUNT                          _IOWR('t', 199, unsigned long *)
#define GPIO_COUNTER_GET_CAPTURE                        _IOWR('t', 200, unsigned long *)

#define NR_TCU_CHNS 8
#define CAPTURE_ALWAYS_ON_FLAGS (0x1 << 7)

static struct ingenic_tcu tcu_dev[2] = {
    {
        .index             = 0,
        .irq               = IRQ_INTC_BASE + IRQ_TCU0,
        .irq_name          = "tcu_int0",
        .clk_name          = "gate_tcu0",
    },

    {
        .index             = 1,
        .irq               = IRQ_INTC_BASE + IRQ_TCU1,
        .irq_name          = "tcu_int1",
        .clk_name          = "gate_tcu1",
    },
};

module_param_named(tcu0_is_enable,  tcu_dev[0].is_enable, int, 0644);
module_param_named(tcu1_is_enable,  tcu_dev[1].is_enable, int, 0644);

struct tcu_private_data {
    unsigned int index;
};

static char *counter_mode_name[] = {
    "pos_gpio0_up_count",                               // gpio0递增计数
    "pos_gpio1_up_count",                               // gpio1递增计数
    "pos_gpio0_up_count_gpio1_rising_edge_clear",       // gpio0递增计数，gpio1上升沿时清除计数
    "pos_gpio1_up_count_gpio0_rising_edge_clear",       // gpio1递增计数，gpio0上升沿时清除计数
    "pos_gpio1_up_count_gpio0_falling_edge_clear",      // gpio1递增计数，gpio0下降沿时清除计数
    "capture_gpio0_gpio1_srcclk",                   // gpio1作为时钟源对gpio0进行捕获，获得周期和高电平时间
    "capture_gpio1_gpio0_srcclk",                   // gpio0作为时钟源对gpio1进行捕获，获得周期和高电平时间
    "capture_gpio0_ext_srcclk_clk/1",               // 外部时钟的1分频作为时钟源对gpio0进行捕获，获得周期和高电平时间
    "capture_gpio0_ext_srcclk_clk/4",
    "capture_gpio0_ext_srcclk_clk/16",
    "capture_gpio0_ext_srcclk_clk/64",
    "capture_gpio0_ext_srcclk_clk/265",
    "capture_gpio0_ext_srcclk_clk/1024",
    "capture_gpio1_ext_srcclk_clk/1",               // 外部时钟的1分频作为时钟源对gpio1进行捕获，获得周期和高电平时间
    "capture_gpio1_ext_srcclk_clk/4",
    "capture_gpio1_ext_srcclk_clk/16",
    "capture_gpio1_ext_srcclk_clk/64",
    "capture_gpio1_ext_srcclk_clk/265",
    "capture_gpio1_ext_srcclk_clk/1024",
    "quadrature_gpio_bothway_count",                // 正交模式，可进行双向计数，计数方向由gpio0和gpio1的正交结果而决定。
};

struct jz_tcu_gpio {
    const char *name;
    int id;
    int gpio;
    int func;
};

struct jz_tcu_gpio tcu_gpio_array[] = {
    { .name = "tcu0_ch0", .id = 0, .func = GPIO_FUNC_2, .gpio = GPIO_PB(20)},
    { .name = "tcu1_ch0", .id = 1, .func = GPIO_FUNC_2, .gpio = GPIO_PB(21)},
    { .name = "tcu0_ch1", .id = 2, .func = GPIO_FUNC_2, .gpio = GPIO_PB(22)},
    { .name = "tcu1_ch1", .id = 3, .func = GPIO_FUNC_2, .gpio = GPIO_PB(23)},
    { .name = "tcu0_ch2", .id = 4, .func = GPIO_FUNC_2, .gpio = GPIO_PB(24)},
    { .name = "tcu1_ch2", .id = 5, .func = GPIO_FUNC_2, .gpio = GPIO_PB(25)},
    { .name = "tcu0_ch3", .id = 6, .func = GPIO_FUNC_2, .gpio = GPIO_PB(26)},
    { .name = "tcu1_ch3", .id = 7, .func = GPIO_FUNC_2, .gpio = GPIO_PB(27)},

    { .name = "tcu0_ch4", .id = 8, .func = GPIO_FUNC_1, .gpio = GPIO_PC(7)},
    { .name = "tcu1_ch4", .id = 9, .func = GPIO_FUNC_1, .gpio = GPIO_PC(8)},
    { .name = "tcu0_ch5", .id = 10, .func = GPIO_FUNC_1, .gpio = GPIO_PC(9)},
    { .name = "tcu1_ch5", .id = 11, .func = GPIO_FUNC_1, .gpio = GPIO_PE(10)},
    { .name = "tcu0_ch6", .id = 12, .func = GPIO_FUNC_1, .gpio = GPIO_PC(11)},
    { .name = "tcu1_ch6", .id = 13, .func = GPIO_FUNC_1, .gpio = GPIO_PC(12)},
    { .name = "tcu0_ch7", .id = 14, .func = GPIO_FUNC_1, .gpio = GPIO_PC(13)},
    { .name = "tcu1_ch7", .id = 15, .func = GPIO_FUNC_1, .gpio = GPIO_PC(14)},

    { .name = "tcu0_store", .id = 16, .func = GPIO_FUNC_1, .gpio = GPIO_PC(20)},
    { .name = "tcu0_store", .id = 17, .func = GPIO_FUNC_1, .gpio = GPIO_PD(12)},
};

static struct jz_tcu_gpio *tcu_get_gpio(int id)
{
    int i;
    struct jz_tcu_gpio *def;

    for (i = 0; i < ARRAY_SIZE(tcu_gpio_array); i++) {
        def = &tcu_gpio_array[i];
        if (def->id == id)
            return def;
    }

    return NULL;
}

static int tcu_gpio_requset(struct jz_tcu_gpio *tcu_gpio)
{
    int ret = gpio_request(tcu_gpio->gpio, tcu_gpio->name);
    gpio_set_func(tcu_gpio->gpio, tcu_gpio->func);
    return ret;
}

static int tcu_gpio_init(struct tcu_chn *tcu_chn)
{
    int ret = 0;

    if (tcu_chn->clksrc_gpio0 == TCU_CLKSRC_GPIO0)
        ret |= tcu_gpio_requset(tcu_chn->gpio0);

    if (tcu_chn->clksrc_gpio1 == TCU_CLKSRC_GPIO1)
        ret |= tcu_gpio_requset(tcu_chn->gpio1);

    if (tcu_chn->clksrc_store == TCU_CLKSRC_STORE) {
        if (tcu_chn->trigger_chn == 0)
            ret |= tcu_gpio_requset(tcu_chn->trigger0);
        if (tcu_chn->trigger_chn == 1)
            ret |= tcu_gpio_requset(tcu_chn->trigger1);
    }

    return ret;
}

static void tcu_gpio_deinit(struct tcu_chn *tcu_chn)
{
    if (tcu_chn->clksrc_gpio0 == TCU_CLKSRC_GPIO0)
        gpio_free(tcu_chn->gpio0->gpio);
    if (tcu_chn->clksrc_gpio1 == TCU_CLKSRC_GPIO1)
        gpio_free(tcu_chn->gpio1->gpio);

    if (tcu_chn->clksrc_store == TCU_CLKSRC_STORE) {
        if (tcu_chn->trigger_chn == 0)
            gpio_free(tcu_chn->trigger0->gpio);
        if (tcu_chn->trigger_chn == 1)
            gpio_free(tcu_chn->trigger1->gpio);
    }
}

/*config gate work mode */
static inline void tcu_config_gate_mode(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) & ~(GATE_SEL_MASK | GATE_POLA_MASK);
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr | GATE_SEL(tcu_chn->gate_sel) | GATE_POLA(tcu_chn->gate_pola));
}

/*config direction work mode*/
static inline void tcu_config_direction_mode(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) & ~(DIR_SEL_MASK | DIR_POLA_MASK);
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr | DIR_SEL(tcu_chn->dir_sel) | DIR_POLA(tcu_chn->dir_pola));
}

/*config quadrature work mode*/
static inline void tcu_config_quadrature_mode(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_TCR(tcu_chn->id)) & ~(DIR_SEL_MASK | DIR_POLA_MASK);
    tcu_write_reg(index, CH_TCR(tcu_chn->id), tcsr | DIR_SEL(tcu_chn->dir_sel) | DIR_POLA(tcu_chn->dir_pola));
}

/*config pos work mode*/
static inline void tcu_config_pos_mode(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    if (tcu_chn->pos_sel > 0 && tcu_chn->pos_sel < 4) {
        tcsr = tcu_read_reg(index, CH_CAP_CTL(tcu_chn->id)) & ~(CAP_POS_SEL_MASK);
        tcu_write_reg(index, CH_CAP_CTL(tcu_chn->id), tcsr | CAP_POS_SEL(tcu_chn->pos_sel));
    }
}

/*config capture work mode*/
static inline void tcu_config_capture_mode(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    if (tcu_chn->cap_sel >= 0 && tcu_chn->cap_sel < 3) {
        tcsr = tcu_read_reg(index, CH_CAP_CTL(tcu_chn->id)) & ~(CAP_POS_SEL_MASK | CAP_NUM_MASK);
        tcu_write_reg(index, CH_CAP_CTL(tcu_chn->id), tcsr | CAP_POS_SEL(tcu_chn->cap_sel) | CAP_NUM(tcu_chn->capture_num));
    }
}

static inline void tcu_quit_capture_mode(int index, struct tcu_chn *tcu_chn) {
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_CAP_CTL(tcu_chn->id)) & ~0xFF;
    tcu_write_reg(index, CH_CAP_CTL(tcu_chn->id), tcsr);
}

/*config filter work mode*/
static inline void tcu_config_filter_mode(int index, struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(index, CH_FIL_VAL(tcu_chn->id)) & ~(FIL_B_MASK | FIL_A_MASK);
    tcu_write_reg(index, CH_FIL_VAL(tcu_chn->id),tcsr | FIL_B(tcu_chn->fil_b_num) | FIL_A(tcu_chn->fil_a_num));
}

/*Choose a working mode*/
static inline void tcu_sel_work_mode(int index, struct tcu_chn *tcu_chn)
{
    switch (tcu_chn->mode) {
    case GENERAL_MODE:
        break;
    case GATE_MODE:
        tcu_config_gate_mode(index, tcu_chn);
        break;
    case DIRECTION_MODE:
        tcu_config_direction_mode(index, tcu_chn);
        break;
    case QUADRATURE_MODE:
        tcu_config_quadrature_mode(index, tcu_chn);
        break;
    case POS_MODE:
        tcu_config_pos_mode(index, tcu_chn);
        break;
    case CAPTURE_MODE:
        tcu_config_capture_mode(index, tcu_chn);
        break;
    case FILTER_MODE:
        tcu_config_filter_mode(index, tcu_chn);
        break;
    case STORE_MODE:
        break;
    default:
        break;
    }
}

static inline void tcu_set_irq_mode(int index, struct tcu_chn *tcu_chn)
{
    switch (tcu_chn->irq_type) {
    case NULL_IRQ_MODE:
        tcu_full_mask(index, tcu_chn);
        tcu_half_mask(index, tcu_chn);
        break;
    case FULL_IRQ_MODE:
        tcu_full_unmask(index, tcu_chn);
        tcu_half_mask(index, tcu_chn);
        break;
    case HALF_IRQ_MODE:
        tcu_full_mask(index, tcu_chn);
        tcu_half_unmask(index, tcu_chn);
        break;
    case FULL_HALF_IRQ_MODE:
        tcu_full_unmask(index, tcu_chn);
        tcu_half_unmask(index, tcu_chn);
        break;
    case STORE_IRQ_MODE:
        tcu_full_mask(index, tcu_chn);
        tcu_half_mask(index, tcu_chn);
        tcu_store_unmask(index, tcu_chn);
        break;
    default:
        break;
    }
}

static void tcu_irq_handler(int index, struct tcu_chn *channel)
{
    unsigned long flags;
    int id ;
    for(id = 0; id < NR_TCU_CHNS; id++) {
        if(channel[id].enable_flag) {
            spin_lock_irqsave(&channel[id].tcu->lock, flags);
            switch (channel[id].irq_type) {
            case FULL_IRQ_MODE:
                if (is_chn_full_int(index, id))
                    tcu_clear_full_flag(index, &channel[id]);
                break;
            case HALF_IRQ_MODE:
                if (is_chn_half_int(index, id))
                    tcu_clear_half_flag(index, &channel[id]);
                break;
            case FULL_HALF_IRQ_MODE:
                if (is_chn_half_int(index, id))
                    tcu_clear_half_flag(index, &channel[id]);
                if (is_chn_full_int(index, id))
                    tcu_clear_full_flag(index, &channel[id]);
                break;
            case STORE_IRQ_MODE:
                if (is_store_int(index, id)) {
                    channel[id].str_val = tcu_store_get_val(index, &channel[id]);
                    printk("store trriger, channel_%d Value = 0x%x\n", id, channel[id].str_val);

                    tcu_clear_store_flag(index, &channel[id]);
                }
                break;
            default:
                break;
            }
            spin_unlock_irqrestore(&channel[id].tcu->lock, flags);

            if (channel[id].mode == CAPTURE_MODE) {
                channel[id].capture_flag = 1;
                wake_up_all(&channel[id].capture_waiter);
            }
        }
    }
}

static irqreturn_t tcu_interrupt(int irq, void *dev_id)
{
    struct ingenic_tcu *tcu = (struct ingenic_tcu *)(dev_id);
    int index = tcu->index;
    struct tcu_chn *channel = tcu->channel;

    if (is_wdt_full_int(index) || is_wdt_half_int(index)) {
        //wdt_irq_handler are not  here
    } else {
        tcu_irq_handler(index, channel);
    }

    return IRQ_HANDLED;
}

#ifdef TCU_DEBUG
void tcu_dump_reg(int index, struct tcu_chn *tcu_chn, int count)
{
    printk("\n\n----------------------------------------count N0.%d----------------------------------------------------\n\n",count);
    printk("-stop-----addr-%08x-value-%08x-----------\n",(unsigned int)TCU_ADDR(index, TCU_TSR),tcu_read_reg(index, TCU_TSR));
    printk("-mask-----addr-%08x-value-%08x-----------\n",(unsigned int)TCU_ADDR(index, TCU_TMR),tcu_read_reg(index, TCU_TMR));
    printk("-enable---addr-%08x-value-%08x-----------\n",(unsigned int)TCU_ADDR(index, TCU_TER),tcu_read_reg(index, TCU_TER));
    printk("-flag-----addr-%08x-value-%08x-----------\n",(unsigned int)TCU_ADDR(index, TCU_TFR),tcu_read_reg(index, TCU_TFR));
    printk("-Control--addr-%08x-value-%08x-----------\n",(unsigned int)TCU_ADDR(index, CH_TCR(tcu_chn->id)),tcu_read_reg(index, CH_TCR(tcu_chn->id)));
    printk("-full-----addr-%08x-value-%08x-----------\n",(unsigned int)TCU_ADDR(index, CH_TDFR(tcu_chn->id)),tcu_read_reg(index, CH_TDFR(tcu_chn->id)));
    printk("-half-   -addr-%08x-value-%08x-----------\n",(unsigned int)TCU_ADDR(index, CH_TDHR(tcu_chn->id)),tcu_read_reg(index, CH_TDHR(tcu_chn->id)));
    printk("-TCNT-----addr-%08x-value-%08x-----------\n",(unsigned int)TCU_ADDR(index, CH_TCNT(tcu_chn->id)),tcu_read_reg(index, CH_TCNT(tcu_chn->id)));
    printk("-CAP_CTR         ---value-%08x-----------\n",tcu_read_reg(index, CH_CAP_CTL(tcu_chn->id)));
    printk("-CAP_VAL         ---value-%08x-----------\n",tcu_read_reg(index, CH_CAP_VAL(tcu_chn->id)));
}
#endif

static void tcu_reset_reg(int index, int id)
{
    tcu_write_reg(index, CH_TDFR(id), 0);
    tcu_write_reg(index, CH_TDHR(id), 0);
    tcu_write_reg(index, CH_TCNT(id), 0);
    tcu_write_reg(index, CH_TCR(id), 0);

    tcu_write_reg(index, CH_CAP_CTL(id), 0);
    tcu_write_reg(index, CH_FIL_VAL(id), 0);
}

static void tcu_reset_channel(int id, struct tcu_chn *channel)
{
    channel[id].capture_num  = 0;
    channel[id].fil_a_num    = 0;
    channel[id].fil_b_num    = 0;

    channel[id].clksrc_ext   = TCU_CLKSRC_NULL;
    channel[id].clksrc_gpio0 = TCU_CLKSRC_NULL;
    channel[id].clksrc_gpio1 = TCU_CLKSRC_NULL;
    channel[id].clksrc_store = TCU_CLKSRC_NULL;

    channel[id].prescale     = TCU_PRESCALE_1;
    channel[id].dir_sel      = DIR_SEL_HH;
    channel[id].gate_sel     = GATE_SEL_HZ;
    channel[id].count_mode   = FULL_CLEAR_ZERO;

    channel[id].gate_pola    = GATE_POLA_LOW;
    channel[id].dir_pola     = DIR_POLA_LOW;
}

static void tcu_set_channel_default_config(int id, struct ingenic_tcu *tcu)
{
    tcu->channel[id].id       = id;
    tcu->channel[id].tcu      = tcu;
    tcu->channel[id].gpio0    = tcu_get_gpio(id * 2);
    tcu->channel[id].gpio1    = tcu_get_gpio(id * 2 + 1);
    tcu->channel[id].trigger0 = tcu_get_gpio(16);
    tcu->channel[id].trigger1 = tcu_get_gpio(17);

    tcu->channel[id].enable_flag   = 0;

    tcu->channel[id].capture_num   = 0;
    tcu->channel[id].fil_a_num     = 0;
    tcu->channel[id].fil_b_num     = 0;

    tcu->channel[id].irq_type      = NULL_IRQ_MODE;
    tcu->channel[id].prescale      = TCU_PRESCALE_64;
    tcu->channel[id].shutdown      = RCNT_AFTER_FULL;
    tcu->channel[id].count_mode    = FULL_CLEAR_ZERO;

    tcu->channel[id].mode          = GENERAL_MODE;
    tcu->channel[id].clksrc_ext    = TCU_CLKSRC_NULL;
    tcu->channel[id].clksrc_gpio0  = TCU_CLKSRC_NULL;
    tcu->channel[id].clksrc_gpio1  = TCU_CLKSRC_NULL;
    tcu->channel[id].gate_sel      = GATE_SEL_HZ;
    tcu->channel[id].dir_sel       = DIR_SEL_HH;
    tcu->channel[id].gate_pola     = GATE_POLA_LOW;
    tcu->channel[id].dir_pola      = DIR_POLA_LOW;
}

static void capture_mode_analy(struct tcu_chn *tcu_chn, char *name)
{
    char *str = NULL;

    tcu_chn->capture_num = 0xa0;

    /* 设置输入时钟源 */
    if (strstr(name, "gpio1_srcclk")) {
        tcu_chn->clksrc_gpio1 = TCU_CLKSRC_GPIO1;
        tcu_chn->sig_gpio1 = SIG_POS_EN;
    }
    if (strstr(name, "gpio0_srcclk")) {
        tcu_chn->clksrc_gpio0 = TCU_CLKSRC_GPIO0;
        tcu_chn->sig_gpio0 = SIG_POS_EN;
    }
    if (strstr(name, "ext_srcclk")) {
        tcu_chn->clksrc_ext = TCU_CLKSRC_EXT;
        tcu_chn->sig_ext = SIG_POS_EN;
        /* 得到ext分频值*/
        str = strstr(name, "_clk/");
        str += strlen("_clk/");
        kstrtoint(str, 10, &tcu_chn->clk_div);
    }

    /* 设置待捕获信号源 */
    if (strstr(name, "capture_gpio0")) {
        tcu_chn->cap_sel = CAPTURE_GPIO0;
        tcu_chn->clksrc_gpio0 = TCU_CLKSRC_GPIO0;
    }
    if (strstr(name, "capture_gpio1")) {
        tcu_chn->cap_sel = CAPTURE_GPIO1;
        tcu_chn->clksrc_gpio1 = TCU_CLKSRC_GPIO1;
    }
}

static void pos_mode_analy(struct tcu_chn *tcu_chn, char *name)
{
    if (strstr(name, "gpio0_up_count")) {
        tcu_chn->clksrc_gpio0 = TCU_CLKSRC_GPIO0;
        tcu_chn->sig_gpio0 = SIG_POS_EN;
    }
    if (strstr(name, "gpio1_up_count")) {
        tcu_chn->clksrc_gpio1 = TCU_CLKSRC_GPIO1;
        tcu_chn->sig_gpio1 = SIG_POS_EN;
    }

    /* 只支持这3种 */
    if (strstr(name, "gpio0_rising_edge_clear")) {
        tcu_chn->pos_sel = GPIO0_POS_CLR;
        tcu_chn->clksrc_gpio0 = TCU_CLKSRC_GPIO0;
    }
    if (strstr(name, "gpio1_rising_edge_clear")) {
        tcu_chn->pos_sel = GPIO1_POS_CLR;
        tcu_chn->clksrc_gpio1 = TCU_CLKSRC_GPIO1;
    }
    if (strstr(name, "gpio0_falling_edge_clear")) {
        tcu_chn->pos_sel = GPIO0_NEG_CLR;
        tcu_chn->clksrc_gpio0 = TCU_CLKSRC_GPIO0;
    }
}

static int tcu_get_counter_mode_num(void)
{
    return ARRAY_SIZE(counter_mode_name);
}

static int tcu_get_counter_mode(struct tcu_chn *tcu_chn, char *name)
{
    int i;
    int num = tcu_get_counter_mode_num();

    for (i = 0; i < num; i++) {
        if (strcmp(counter_mode_name[i], name) == 0)
            break;
    }
    if (i == num)
        return -1;

    if (strstr(name, "capture")) {
        tcu_chn->mode = CAPTURE_MODE;
        return 0;
    }
    if (strstr(name, "pos")) {
        tcu_chn->mode = POS_MODE;
        return 0;
    }
    if (strstr(name, "quadrature")) {
        /* 正交模式 */
        tcu_chn->mode = QUADRATURE_MODE;
        return 0;
    }

    return -1;
}

static void tcu_config_chn(int index, struct tcu_chn *tcu_chn)
{
    spin_lock(&tcu_chn->tcu->lock);

    tcu_clear_full_flag(index, tcu_chn);
    tcu_clear_half_flag(index, tcu_chn);
    tcu_clear_tcnt(index, tcu_chn);

    tcu_set_chn_full(index, tcu_chn);
    tcu_set_chn_half(index, tcu_chn);

    tcu_set_clksrc(index, tcu_chn);
    if (!is_chn_running(index, tcu_chn->id))
        tcu_set_prescale(index, tcu_chn);

    tcu_set_shutdown(index, tcu_chn);
    tcu_set_count_mode(index, tcu_chn);

    tcu_sel_work_mode(index, tcu_chn);

    spin_unlock(&tcu_chn->tcu->lock);
}

static void tcu_config_mode(struct tcu_chn *tcu_chn)
{
    tcu_chn->irq_type   = FULL_IRQ_MODE;
    tcu_chn->prescale   = TCU_PRESCALE_1024;//内部时种缩放为24khz
    tcu_chn->shutdown   = RCNT_AFTER_FULL;
    tcu_chn->count_mode = FULL_CLEAR_ZERO;
    tcu_chn->full_num   = TCU_FULL_NUM;
    tcu_chn->half_num   = TCU_HALF_NUM;

    switch (tcu_chn->mode) {
    case GENERAL_MODE:
        /*Enable external clock to use rising edge counting , result TCNT != 0*/
        tcu_chn->clksrc_ext      = TCU_CLKSRC_EXT;
        tcu_chn->sig_ext         = SIG_POS_EN;
        break;
    case GATE_MODE:
        /*gate signal hold on 0,counter start when control signal is 1,result TCNT == 0*/
        tcu_chn->clksrc_ext      = TCU_CLKSRC_EXT;
        tcu_chn->sig_ext         = SIG_POS_EN;

        tcu_chn->clksrc_gpio0    = TCU_CLKSRC_GPIO0;
        tcu_chn->gate_sel        = GATE_SEL_GPIO0;
        tcu_chn->gate_pola       = GATE_POLA_HIGH;
        break;
    case DIRECTION_MODE:
        /*use gpio0 with direction signa. counter sub when control signal is 1.result TCNT add and sub */
        tcu_chn->clksrc_ext      = TCU_CLKSRC_EXT;
        tcu_chn->sig_ext         = SIG_POS_EN;

        tcu_chn->clksrc_gpio0    = TCU_CLKSRC_GPIO0;
        tcu_chn->dir_sel         = DIR_SEL_GPIO0;
        tcu_chn->dir_pola        = DIR_POLA_HIGH;
        break;
    case QUADRATURE_MODE:
        tcu_chn->clksrc_ext      = TCU_CLKSRC_EXT;
        tcu_chn->clksrc_gpio0    = TCU_CLKSRC_GPIO0;
        tcu_chn->clksrc_gpio1    = TCU_CLKSRC_GPIO1;
        tcu_chn->dir_sel         = DIR_SEL_GPIO_QUA;
        tcu_chn->sig_gpio0       = SIG_POS_NEG_EN;
        tcu_chn->sig_gpio1       = SIG_POS_NEG_EN;
        break;
    case POS_MODE:
        pos_mode_analy(tcu_chn, tcu_chn->mode_name);
        break;
    case CAPTURE_MODE:
        capture_mode_analy(tcu_chn, tcu_chn->mode_name);
        break;
    case FILTER_MODE:
        /*暂时只有gpio 0*/
        tcu_chn->clksrc_gpio0    = TCU_CLKSRC_GPIO0;
        tcu_chn->sig_gpio0       = SIG_POS_EN;

        tcu_chn->fil_a_num       = 0x3ff;
        tcu_chn->fil_b_num       = 0x3ff;
        break;
    case STORE_MODE:
        tcu_chn->clksrc_ext      = TCU_CLKSRC_EXT;
        tcu_chn->sig_ext         = SIG_POS_EN;
        tcu_chn->clksrc_store    = TCU_CLKSRC_STORE;
        tcu_chn->sig_store       = SIG_POS_EN;

        tcu_chn->irq_type        = STORE_IRQ_MODE;
        break;
    default:
            break;
    }
}

#define ERROR_IF(condition, msg, ...) do { \
    if (condition) { \
        printk(KERN_ERR "TCU:" msg "\n", ##__VA_ARGS__); \
        return -1; \
    } \
} while(0)

static int tcu_config(int index, int id, char *mode_name, struct tcu_chn *channel)
{
    int ret = 0;

    ERROR_IF(id < 0 || id > TCU_CHANNEL_NUM, "Please select the correct channel 0 ~ 7 range");

    ret = tcu_get_counter_mode(&channel[id], mode_name);
    if (ret < 0) {
        printk(KERN_ERR "TCU: config channel:%d failre\n", id);
        return ret;
    }
    strncpy(channel[id].mode_name, mode_name, MODE_NAME_LEN);

    tcu_reset_channel(id, channel);
    tcu_reset_reg(index, id);

    tcu_gpio_deinit(&channel[id]);// 释放gpio

    tcu_config_mode(&channel[id]);
    tcu_config_chn(index, &channel[id]);

    tcu_gpio_init(&channel[id]);  // 申请gpio

    channel[id].config_flag = 1;

    return ret;
}

static int tcu_enable(int index, int id, struct tcu_chn *channel)
{
    ERROR_IF(id < 0 || id > TCU_CHANNEL_NUM, "Please select the correct channel 0 ~ 7 range");
    ERROR_IF(channel[id].config_flag == 0, "channel %d not config", id);
    ERROR_IF(channel[id].enable_flag, "channel %d already enable", id);

    tcu_set_irq_mode(index, &channel[id]);
    channel[id].enable_flag = 1;

    if (channel[id].mode != CAPTURE_MODE) {
        tcu_enable_chn_counter(index, &channel[id]);
        tcu_start_chn_counter(index, &channel[id]);
    }

#ifdef TCU_DEBUG
    int i;
    msleep(200);
    for (i = 0; i < 10 ;i++)
        tcu_dump_reg(index, &channel[id], i);
#endif

    return 0;
}

static int tcu_disable(int index, int id, struct tcu_chn *channel)
{
    ERROR_IF(id < 0 || id > TCU_CHANNEL_NUM, "Please select the correct channel 0 ~ 7 range");
    ERROR_IF(channel[id].config_flag == 0, "channel %d not config", id);
    ERROR_IF(!channel[id].enable_flag, "channel %d not enable", id);

    if (channel[id].mode == STORE_MODE) {
        tcu_store_unmask(index, &channel[id]);
        tcu_store_neg_disable(index, &channel[id]);
        tcu_store_cnt_disable(index, &channel[id]);
    }

    if (channel[id].mode == CAPTURE_MODE) {
        if ((channel[id].capture_num & CAPTURE_ALWAYS_ON_FLAGS) != 0) {
            /* 退出capture mode */
            tcu_quit_capture_mode(index, &channel[id]);
        }
    } else {
        tcu_disable_chn_counter(index, &channel[id]);
        tcu_clear_tcnt(index, &channel[id]);
        tcu_stop_chn_counter(index, &channel[id]);
    }

    tcu_full_mask(index, &channel[id]);
    tcu_half_mask(index, &channel[id]);
    channel[id].enable_flag = 0;

#ifdef TCU_DEBUG
    int i;
    msleep(200);
    for (i = 0; i < 10 ;i++)
        tcu_dump_reg(index, &channel[id], i);
#endif

    return 0;
}

static unsigned int tcu_get_period_high_level(int index, unsigned int id, int *high_level_time, int *period_time)
{
    *high_level_time = tcu_get_bit(index, CH_CAP_VAL(id), 0, 15);
    *period_time = tcu_get_bit(index, CH_CAP_VAL(id), 16, 31);
    return 0;
}

static unsigned int tcu_wait_capture(int index, unsigned int id, int *high_level_time, int *period_time, struct tcu_chn *channel)
{
    ERROR_IF(!channel[id].enable_flag, "channel %d not enable", id);

    if (!channel[id].capture_flag)
        wait_event_interruptible_timeout(channel[id].capture_waiter,
                                channel[id].capture_flag, msecs_to_jiffies(3000));

    tcu_get_period_high_level(index, id, high_level_time, period_time);
    channel[id].capture_flag = 0;

    return 0;
}

static unsigned int tcu_get_count(int index, unsigned int id, int *count, struct tcu_chn *channel)
{
    if (!channel[id].enable_flag) {
        printk(KERN_ERR "TCU:Please enable channel before getting a count\n");
        return -1;
    }

    *count = tcu_get_tcnt(index, &channel[id]);
    return 0;
}

static int tcu_get_counter_mode_name(char *buf)
{
    int id;
    int length = MODE_NAME_LEN;
    int num = tcu_get_counter_mode_num();

    for(id = 0; id < num; id++)
        strncpy(buf + id*length, counter_mode_name[id], length);

    return 0;
}

static long tcu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int *count;
    int channel_id;
    int *num, *size;
    int *high_level_time;
    int *period_time;

    struct tcu_private_data *data = filp->private_data;
    unsigned int index = data->index;
    struct ingenic_tcu *tcu = &tcu_dev[index];
    struct tcu_chn *channel = tcu->channel;

    unsigned long *argv = (void *)arg;

    switch (cmd) {
    case GPIO_COUNTER_CONFIG:
        channel_id = argv[0];
        char *mode_name = (char *)argv[1];
        ret = tcu_config(index, channel_id, mode_name, channel);
        break;
    case GPIO_COUNTER_ENABLE:
        channel_id = arg;
        ret = tcu_enable(index, channel_id, channel);
        break;
    case GPIO_COUNTER_DISABLE:
        channel_id = arg;
        ret = tcu_disable(index, channel_id, channel);
        break;
    case GPIO_COUNTER_GET_CAPTURE:
        channel_id = argv[0];
        high_level_time = (int *)argv[1];
        period_time = (int *)argv[2];
        ret = tcu_wait_capture(index, channel_id, high_level_time, period_time, channel);
        break;
    case GPIO_COUNTER_GET_COUNT:
        channel_id = argv[0];
        count = (int *)argv[1];
        ret = tcu_get_count(index, channel_id, count, channel);
        break;
    case GPIO_COUNTER_GET_MODE_NUM:
        num = (int *)argv[0];
        size = (int *)argv[1];
        *num = tcu_get_counter_mode_num();
        *size = MODE_NAME_LEN;
        return ret;
    case GPIO_COUNTER_GET_MODE_NAME:
        ret = tcu_get_counter_mode_name((char *)arg);
        break;
    default:
        ret = -1;
        printk(KERN_ERR "TCU:no support your cmd:%x\n", cmd);
    }
    return ret;
}

static int tcu_open(struct inode *inode, struct file *filp)
{
    struct ingenic_tcu *tcu = container_of(filp->private_data, struct ingenic_tcu, tcu_mdev);
    struct tcu_private_data *data = kmalloc(sizeof(*data), GFP_KERNEL);

    data->index = tcu->index;

    filp->private_data = data;
    return 0;
}

static int tcu_release(struct inode *inode, struct file *filp)
{
    struct tcu_private_data *data = filp->private_data;

    if (data) {
        kfree(data);
    }

    return 0;
}

static struct file_operations tcu_misc_fops = {
    .open = tcu_open,
    .release = tcu_release,
    .unlocked_ioctl = tcu_ioctl,
};

static struct miscdevice tcu_mdevice[2] = {
    {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "jz_tcu",
        .fops = &tcu_misc_fops,
    },

    {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "jz_tcu1",
        .fops = &tcu_misc_fops,
    },
};

static int tcu_init(int index)
{
    struct ingenic_tcu *tcu;
    int  id, ret = 0;

    if (index == 0) {
        tcu = &tcu_dev[0];
        tcu->tcu_mdev = tcu_mdevice[0];
    }
    if (index == 1) {
        tcu = &tcu_dev[1];
        tcu->tcu_mdev = tcu_mdevice[1];
    }

    tcu->channel = kmalloc(sizeof(struct tcu_chn) * NR_TCU_CHNS, GFP_KERNEL);
    if (!tcu->channel) {
        printk(KERN_ERR "Failed to allocate channel struct\n");
        return -ENOMEM;
    }

    spin_lock_init(&tcu->lock);

    for (id = 0; id < NR_TCU_CHNS; id++) {
        tcu_set_channel_default_config(id, tcu);
        init_waitqueue_head(&tcu->channel[id].capture_waiter);
    }

    tcu->clk = clk_get(NULL, tcu->clk_name);
    clk_prepare_enable(tcu->clk);

    ret = request_irq(tcu->irq, tcu_interrupt,
            IRQF_SHARED | IRQF_TRIGGER_LOW, tcu->irq_name, tcu);
    if (ret) {
        printk(KERN_ERR "request_irq failed !! %d-\n",tcu->irq);
        return ret;
    }

    ret = misc_register(&tcu->tcu_mdev);
    if (ret < 0)
        panic(KERN_ERR "TCU: %s, tcu register misc dev error !\n", __func__);

    tcu->is_finish = 1;

    return 0;
}

static void tcu_deinit(int index)
{
    struct ingenic_tcu *tcu;
    if (index == 0) {
        tcu = &tcu_dev[0];
        tcu->tcu_mdev = tcu_mdevice[0];
    }
    if (index == 1) {
        tcu = &tcu_dev[1];
        tcu->tcu_mdev = tcu_mdevice[1];
    }

    clk_disable_unprepare(tcu->clk);
    free_irq(tcu->irq,tcu);

    misc_deregister(&tcu->tcu_mdev);
}

static int __init jz_tcu_init(void)
{
    if (tcu_dev[0].is_enable)
        tcu_init(0);

    if (tcu_dev[1].is_enable)
        tcu_init(1);

    return 0;
}

static void __exit jz_tcu_exit(void)
{
    if (tcu_dev[0].is_finish)
        tcu_deinit(0);

    if (tcu_dev[1].is_finish)
        tcu_deinit(1);
}

module_init(jz_tcu_init);
module_exit(jz_tcu_exit);

MODULE_DESCRIPTION("Ingenic X2600 TCU driver");
MODULE_LICENSE("GPL");
