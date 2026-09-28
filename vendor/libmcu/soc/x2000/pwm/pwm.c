#include <soc/base.h>
#include <driver/gpio.h>
#include <driver/irq.h>
#include <driver/clk.h>
#include <assert.h>
#include <driver/pwm.h>
#include <bit_field2.h>
#include <little_things.h>
#include <delay.h>
#include <cpu/io.h>
// #include <addrspace.h>

#define PWM_CCFG0       0x00 // need spin look
#define PWM_CCFG1       0x04 // need spin look
#define PWM_ENS         0x10
#define PWM_ENC         0x14
#define PWM_EN          0x18
#define PWM_UP          0x20
#define PWM_BUSY        0x24
#define PWM_INITR       0x30 // need spin look
#define PWM_WCFG(n)     (0xb0 + (n)*4)

#define PWM_DES         0x100
#define PWM_DEC         0x104
#define PWM_DE          0x108
#define PWM_DCR0        0x110 // need spin look
#define PWM_DCR1        0x114 // need spin look
#define PWM_DTRIG       0x120
#define PWM_DFER        0x124
#define PWM_DFSM        0x128
#define PWM_DSR         0x130
#define PWM_DSCR        0x134
#define PWM_DINTC       0x138 // need spin look
#define PWM_DAR(n)      (0x140 + (n)*4)
#define PWM_DTLR(n)     (0x190 + (n)*4)
#define PWM_OEN          0x300 // need spin look

#define PWM_CCFG0_div(id)       (id * 4), (id * 4 + 3)
#define PWM_CCFG1_div(id)       ((id - 8) * 4), ((id - 8) * 4 + 3)
#define PWM_WAVEFROM_HIGH   16, 31
#define PWM_WAVWFROM_LOW    0, 15

#define PWM_NUMS 16
#define PWM_DUTY_MAX_COUNT   (0xFFFF)

#define PWM_REG_BASE  PWM_IOBASE
#define PWM_ADDR(reg) ((volatile unsigned long *)(PWM_REG_BASE + (reg)))

#define PWM_CONFIG_REG_MODE 1
#define PWM_CONFIG_DMA_MODE 2

#define VIR_TO_PHY(x) (((x & 0x0000ffff) | 0x13420000) + 0x00002000)

static inline void pwm_write_reg(unsigned int reg, unsigned int value)
{
    *PWM_ADDR(reg) = value;
}

static inline unsigned int pwm_read_reg(unsigned int reg)
{
    return *PWM_ADDR(reg);
}

static inline void pwm_set_bit(unsigned int reg, int bit, unsigned int value)
{
    *PWM_ADDR(reg) = set_bit_field(*PWM_ADDR(reg), bit, bit, value);
}

static inline unsigned int pwm_get_bit(unsigned int reg, int bit)
{
    return get_bit_field(*PWM_ADDR(reg), bit, bit);
}

static inline void pwm_set_bits(unsigned int reg, int start, int end, unsigned int value)
{
    *PWM_ADDR(reg) = set_bit_field(*PWM_ADDR(reg), start, end, value);
}

static inline unsigned int pwm_get_bits(unsigned int reg, int start, int end)
{
    return get_bit_field(*PWM_ADDR(reg), start, end);
}

/**********************************************************/
static void pwm_error(const char *err_msg)
{
    printf("pwm: failed to %s\n",err_msg);
}

struct pwm_handle *pwmdatas[PWM_NUMS];
static unsigned long clk_src_rate;

struct pwm_handle_gpio pwm_gpio_array[] = {
#ifdef  APP_libmcu_x2000_pwm0_PC
    { .id = 0, .func = GPIO_FUNC_0, .gpio = GPIO_PC(0), },
#endif
#ifdef  APP_libmcu_x2000_pwm1_PC
    { .id = 1, .func = GPIO_FUNC_0, .gpio = GPIO_PC(1), },
#endif
#ifdef  APP_libmcu_x2000_pwm2_PC
    { .id = 2, .func = GPIO_FUNC_0, .gpio = GPIO_PC(2), },
#endif
#ifdef  APP_libmcu_x2000_pwm3_PC
    { .id = 3, .func = GPIO_FUNC_0, .gpio = GPIO_PC(3), },
#endif
#ifdef  APP_libmcu_x2000_pwm4_PC
    { .id = 4, .func = GPIO_FUNC_0, .gpio = GPIO_PC(4), },
#endif
#ifdef  APP_libmcu_x2000_pwm5_PC
    { .id = 5, .func = GPIO_FUNC_0, .gpio = GPIO_PC(5), },
#endif
#ifdef  APP_libmcu_x2000_pwm6_PC
    { .id = 6, .func = GPIO_FUNC_0, .gpio = GPIO_PC(6), },
#endif
#ifdef  APP_libmcu_x2000_pwm7_PC
    { .id = 7, .func = GPIO_FUNC_0, .gpio = GPIO_PC(7), },
#endif
#ifdef  APP_libmcu_x2000_pwm8
    { .id = 8, .func = GPIO_FUNC_0, .gpio = GPIO_PC(8), },
#endif
#ifdef  APP_libmcu_x2000_pwm9
    { .id = 9, .func = GPIO_FUNC_0, .gpio = GPIO_PC(9), },
#endif
#ifdef  APP_libmcu_x2000_pwm10
    { .id = 10, .func = GPIO_FUNC_0, .gpio = GPIO_PC(10), },
#endif
#ifdef  APP_libmcu_x2000_pwm11
    { .id = 11, .func = GPIO_FUNC_0, .gpio = GPIO_PC(11), },
#endif
#ifdef  APP_libmcu_x2000_pwm12
    { .id = 12, .func = GPIO_FUNC_0, .gpio = GPIO_PC(12), },
#endif
#ifdef  APP_libmcu_x2000_pwm13
    { .id = 13, .func = GPIO_FUNC_0, .gpio = GPIO_PC(13), },
#endif
#ifdef  APP_libmcu_x2000_pwm14
    { .id = 14, .func = GPIO_FUNC_0, .gpio = GPIO_PC(14), },
#endif
#ifdef  APP_libmcu_x2000_pwm15
    { .id = 15, .func = GPIO_FUNC_0, .gpio = GPIO_PC(15), },
#endif

#ifdef  APP_libmcu_x2000_pwm0_PD
    { .id = 0, .func = GPIO_FUNC_2, .gpio = GPIO_PD(30), },
#endif
#ifdef  APP_libmcu_x2000_pwm1_PD
    { .id = 1, .func = GPIO_FUNC_2, .gpio = GPIO_PD(31), },
#endif
#ifdef  APP_libmcu_x2000_pwm2_PE
    { .id = 2, .func = GPIO_FUNC_1, .gpio = GPIO_PE(0), },
#endif
#ifdef  APP_libmcu_x2000_pwm3_PE
    { .id = 3, .func = GPIO_FUNC_1, .gpio = GPIO_PE(1), },
#endif
#ifdef  APP_libmcu_x2000_pwm4_PE
    { .id = 4, .func = GPIO_FUNC_1, .gpio = GPIO_PE(2), },
#endif
#ifdef  APP_libmcu_x2000_pwm5_PE
    { .id = 5, .func = GPIO_FUNC_1, .gpio = GPIO_PE(3), },
#endif
#ifdef  APP_libmcu_x2000_pwm6_PE
    { .id = 6, .func = GPIO_FUNC_1, .gpio = GPIO_PE(4), },
#endif
#ifdef  APP_libmcu_x2000_pwm7_PE
    { .id = 7, .func = GPIO_FUNC_1, .gpio = GPIO_PE(5), },
#endif
};

int pwm_dma_stop(struct pwm_handle *pwmdata);

static struct pwm_handle_gpio *pwm_get_gpio_func_def(int gpio)
{
    int i;
    struct pwm_handle_gpio *def;

    for (i = 0; i < ARRAY_SIZE(pwm_gpio_array); i++) {
        def = &pwm_gpio_array[i];
        if (def->gpio == gpio)
            return def;
    }

    return NULL;
}

static void pwm_set_clk_div(struct pwm_handle *pwmdata)
{
    unsigned long div, tmp;
    int id = pwmdata->gpio_def->id;

    tmp = clk_src_rate / pwmdata->rate;

    switch (tmp) {
        case 1:div = 0;break;
        case 2:div = 1;break;
        case 4:div = 2;break;
        case 8:div = 3;break;
        case 16:div = 4;break;
        case 32:div = 5;break;
        case 64:div = 6;break;
        case 128:div = 7;break;
        default:panic("PWM: %s, pwm ch %d clk div must be [1 2 4 8 16 32 64 128]!\n", __func__, id);
    }

    if (id < 8)
        pwm_set_bits(PWM_CCFG0, PWM_CCFG0_div(id), div);
    else
        pwm_set_bits(PWM_CCFG1, PWM_CCFG1_div(id), div);
}


static void pwm_disable(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    if (!pwmdata->is_enable)
        return;

    /* disable pwm */
    pwm_set_bit(PWM_OEN, id, 0);

    pwm_write_reg(PWM_ENC, 1 << id);

    udelay(2);

    pwmdata->is_enable = 0;
}

static void pwm_enable(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    if (pwmdata->is_enable)
        return;

    gpio_set_func(pwmdata->gpio_def->gpio, pwmdata->gpio_def->func);

    /* enable pwm */
    if (!pwmdata->not_really_enable) {
        pwm_write_reg(PWM_ENS, 1 << id);

        pwm_set_bit(PWM_OEN, id, 1);

        udelay(2);
    }

    pwmdata->is_enable = 1;
}

static void set_pwm_mode(int id, int dma_mode)
{
    pwm_set_bit(PWM_DCR0, id, !!dma_mode);
}

static void set_pwm_dma_loop(int id, int dma_loop)
{
    pwm_set_bit(PWM_DCR1, id, !!dma_loop);
}

static void set_pwm_init_level(int id, int level)
{
    pwm_set_bit(PWM_INITR, id, !!level);
}

static void set_pwm_idle_level(int id, int level)
{
    pwm_set_bit(PWM_INITR, id + 16, !!level);
}

static void set_pwm_irq_mask(int id, int mask)
{
    /* clear irq flag */
    pwm_write_reg(PWM_DSCR, 1 << id);

    pwm_set_bit(PWM_DINTC, id, !!mask);
}

static void get_pwm_source_rate_freq_first(struct pwm_handle *pwmdata, unsigned long freq)
{
    int i;
    unsigned long rate;

    i = 0;
    rate = clk_src_rate;
    while((rate / freq > PWM_DUTY_MAX_COUNT) && (i < 7)) {
        rate = rate / 2;
        i++;
    }

    pwmdata->rate = rate;
    pwmdata->full_num = rate / freq;
    if (pwmdata->full_num > PWM_DUTY_MAX_COUNT)
        pwmdata->full_num = PWM_DUTY_MAX_COUNT;

    pwmdata->real_rate = rate / pwmdata->full_num;
}

static void get_pwm_source_rate_levels_first(struct pwm_handle *pwmdata, unsigned long freq, unsigned long levels)
{
    int i;
    unsigned long rate;
    unsigned long delta;

    i = 0;
    rate = clk_src_rate;
    while((rate / freq > PWM_DUTY_MAX_COUNT) && (i < 7)) {
        rate = rate / 2;
        i++;
    }

    pwmdata->rate = rate;

    if (rate / freq < levels) {
        pwmdata->full_num = levels;
    } else {
        delta = (rate / freq) % levels;
        pwmdata->full_num = rate / freq - delta;
    }

    if (pwmdata->full_num > PWM_DUTY_MAX_COUNT)
        pwmdata->full_num = PWM_DUTY_MAX_COUNT;

    pwmdata->real_rate = rate / pwmdata->full_num;
}

static int pwm_update(struct pwm_handle *pwmdata)
{
    unsigned long timeout;
    int id = pwmdata->gpio_def->id;

    /* wait 1.5 cycle to update pwm*/
    if (pwm_get_bit(PWM_BUSY, id)) {
        timeout = 3*1000*1000 / pwmdata->real_rate / 2;
        if (timeout == 0)
            timeout = 1;

        udelay(timeout);

        if (pwm_get_bit(PWM_BUSY, id)) {
            pwm_error("updata wavwfrom config timeout");
            return -1;
        }
    }

    pwm_write_reg(PWM_UP, 1 << id);

    return 0;
}


void pwm_set_not_really_disable(struct pwm_handle *pwmdata, int enable)
{
    pwmdata->not_really_disable = !!enable;
}

void pwm_set_not_really_enable(struct pwm_handle *pwmdata, int enable)
{
    pwmdata->not_really_enable = !!enable;
}

void pwm_enable_channels(unsigned int channels)
{
    unsigned int pwm_oen;

    pwm_write_reg(PWM_ENS, channels);
    pwm_oen = pwm_read_reg(PWM_OEN);
    pwm_write_reg(PWM_OEN, channels | pwm_oen);
}

void pwm_disable_channels(unsigned int channels)
{
    unsigned int pwm_oen;

    pwm_oen = pwm_read_reg(PWM_OEN);
    pwm_write_reg(PWM_OEN, pwm_oen & (~channels));
    pwm_write_reg(PWM_ENC, channels);
}

int pwm_config(struct pwm_handle *pwmdata, struct pwm_config_data *config)
{
    int ret = 0;
    int id = pwmdata->gpio_def->id;
    unsigned long freq = config->freq;
    unsigned long levels = config->levels;
    unsigned int idle_level = config->idle_level;

    if (id < 0 || id >= PWM_NUMS) {
        pwm_error("pwm channel not support");
        return -1;
    }

    if (freq == 0) {
        pwm_error("freq not support");
        return -1;
    }

    if (freq > clk_src_rate) {
        pwm_error("freq not support");
        return -1;
    }

    if (levels >= PWM_DUTY_MAX_COUNT) {
        pwm_error("levels not support");
        return -1;
    }

    if (!pwmdata->is_request) {
        pwm_error("pwm channel not request");
        ret = -1;
        goto unlock;
    }

    if (pwmdata->is_enable) {
        pwm_error("pwm channel Cannot configure at working");
        ret = -1;
        goto unlock;
    }

    pwmdata->levels = levels;
    pwmdata->idle_level = !!idle_level;

    if (config->accuracy_priority == PWM_accuracy_freq_first)
        get_pwm_source_rate_freq_first(pwmdata, freq);
    else
        get_pwm_source_rate_levels_first(pwmdata, freq, levels);

    /* set pwm clk div */
    pwm_set_clk_div(pwmdata);

    /* set pwm level */
    set_pwm_idle_level(id, pwmdata->idle_level);
    set_pwm_init_level(id, !pwmdata->idle_level);

    /* updata mode */
    set_pwm_mode(id, 0);

    pwmdata->is_config = PWM_CONFIG_REG_MODE;

unlock:


    return ret;
}

int pwm_request(struct pwm_handle *pwmdata, int gpio)
{
    int id;
    struct pwm_handle_gpio *gpio_def;

    gpio_def = pwm_get_gpio_func_def(gpio);
    if (gpio_def == NULL) {
        pwm_error("gpio not support pwm");
        return -1;
    }

    id = gpio_def->id;

    if (pwmdata->is_request) {
        pwm_error("pwm already request");
        goto unlock;
    }

    pwmdata->gpio_def = gpio_def;
    pwmdata->rate = clk_src_rate;
    pwmdata->is_request = 1;

    pwmdatas[id] = pwmdata;
unlock:

    return 0;
}

int pwm_set_level(struct pwm_handle *pwmdata, unsigned long level)
{
    int ret = 0;
    int id = pwmdata->gpio_def->id;
    unsigned long pwm_level = 0;

    if (id < 0 || id >= PWM_NUMS) {
        pwm_error("pwm channel not support");
        return -1;
    }

    if (pwmdata->is_config != PWM_CONFIG_REG_MODE) {
        pwm_error("pwm channel not config");
        ret = -1;
        goto unlock;
    }

    if (level > pwmdata->levels) {
        pwm_error("set level more than max level");
        ret = -1;
        goto unlock;
    }

    if (level != 0) {
        level = level * pwmdata->full_num / pwmdata->levels;
        if (level == 0)
            level = 1;
    }

    pwmdata->half_num = level;

    /* set pwm waveform config */
    if (pwmdata->idle_level) {
        pwmdata->high_levels = pwmdata->full_num - level;
        pwmdata->low_levels = level;
    } else {
        pwmdata->low_levels = pwmdata->full_num - level;
        pwmdata->high_levels = level;
    }

    /* set pwm init level */
    if (pwmdata->low_levels == 0) {
        pwmdata->func_is_clear = 1;
        gpio_set_func(pwmdata->gpio_def->gpio, GPIO_OUTPUT1);
        pwm_level = set_bit_field(pwm_level, PWM_WAVWFROM_LOW, 1);
        pwm_level = set_bit_field(pwm_level, PWM_WAVEFROM_HIGH, pwmdata->full_num - 1);
        pwm_write_reg(PWM_WCFG(id), pwm_level);
        // pwm_write_reg(PWM_UP, 1 << id);

        if (!pwmdata->not_really_disable)
            pwm_disable(pwmdata);
        goto unlock;
    } else if (pwmdata->high_levels == 0) {
        pwmdata->func_is_clear = 1;
        gpio_set_func(pwmdata->gpio_def->gpio, GPIO_OUTPUT0);
        pwm_level = set_bit_field(pwm_level, PWM_WAVWFROM_LOW, pwmdata->full_num - 1);
        pwm_level = set_bit_field(pwm_level, PWM_WAVEFROM_HIGH, 1);
        pwm_write_reg(PWM_WCFG(id), pwm_level);
        // pwm_write_reg(PWM_UP, 1 << id);

        if (!pwmdata->not_really_disable)
            pwm_disable(pwmdata);
        goto unlock;
    }
    if (pwmdata->func_is_clear) {
        pwmdata->func_is_clear = 0;
        gpio_set_func(pwmdata->gpio_def->gpio, pwmdata->gpio_def->func);
    }

    pwm_level = set_bit_field(pwm_level, PWM_WAVWFROM_LOW, pwmdata->low_levels);
    pwm_level = set_bit_field(pwm_level, PWM_WAVEFROM_HIGH, pwmdata->high_levels);
    pwm_write_reg(PWM_WCFG(id), pwm_level);

    /* set pwm updata */
    ret = pwm_update(pwmdata);
    if (ret < 0)
        goto unlock;

    pwm_enable(pwmdata);

unlock:

    return ret;
}

void pwm_release(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        pwm_error("pwm channel not support");
        return;
    }

    if (pwmdata->is_config == PWM_CONFIG_DMA_MODE && pwmdata->dma_loop)
        pwm_dma_stop(pwmdata);

    pwm_disable(pwmdata);

    if (pwmdata->is_request) {

        pwmdata->is_request = 0;
    }

    pwmdata->is_config = 0;

}

unsigned long pwm_get_freq(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        pwm_error("pwm channel not support");
        return -1;
    }

    return pwmdata->real_rate;
}

int pwm_get_id(struct pwm_handle *pwmdata)
{
    return pwmdata->gpio_def->id;
}


int pwm_dma_start(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data)
{
    int ret = 0;
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        pwm_error("pwm channel not support");
        return -1;
    }

    if (!pwmdata->is_request) {
        pwm_error("pwm channel not request");
        ret = -1;
        goto unlock;
    }

    if (pwmdata->is_enable) {
        pwm_error("pwm channel Cannot configure at working");
        ret = -1;
        goto unlock;
    }

    /* set pwm clk div, count clk must not less than AHB2 clk*/
    pwmdata->rate = clk_src_rate;
    pwm_set_clk_div(pwmdata);

    /* updata dma mode */
    set_pwm_mode(id, 1);

    /* set pwm idle and init level */
    pwmdata->init_level = !!dma_data->start_level;
    pwmdata->idle_level = !!dma_data->idle_level;
    set_pwm_init_level(id, pwmdata->init_level);
    set_pwm_idle_level(id, pwmdata->idle_level);

    pwmdata->is_config = PWM_CONFIG_DMA_MODE;

    if (dma_data == NULL || dma_data->data == NULL || dma_data->data_count == 0) {
        pwm_error("not have dma data");
        return -1;
    }

    if (dma_data->dma_loop && dma_data->data_count % 4) {
        pwm_error("dma data length must 4 word align");
        return -1;
    }

#ifdef PWM_CHECK_DMA_DATA
    for (int i = 0; i < dma_data->data_count; i++) {
        if (dma_data->data[i].high == 0 || dma_data->data[i].low == 0) {
            pwm_error("data not be null in dma mode")
            return -1;
        }
    }
#endif

    if (pwmdata->dma_loop) {
        pwm_error("work in dma loop mode");
        ret = -1;
        goto unlock;
    }

    assert(!pwmdata->dma_data);

    pwmdata->dma_data = dma_data->data;

    /* update dma loop mode */
    pwmdata->dma_loop = !!dma_data->dma_loop;
    set_pwm_dma_loop(id, pwmdata->dma_loop);

    /* set dma data_addr and data_len */
    pwm_write_reg(PWM_DAR(id), VIR_TO_PHY((unsigned int)pwmdata->dma_data));
    pwm_write_reg(PWM_DTLR(id), dma_data->data_count);

    /* enable irq */
    set_pwm_irq_mask(id, pwmdata->dma_loop);

    /* enable dma */
    pwm_write_reg(PWM_DES, 1 << id);
    /* trigger dma */
    pwm_write_reg(PWM_DTRIG, 1 << id);

    pwm_enable(pwmdata);

unlock:
    return ret;
}

void pwm_dma_wait_end(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data)
{
    int id = pwmdata->gpio_def->id;

    /* normal mode wait dma complete */
    if (!dma_data->dma_loop) {
        /* wait dma_end */
        while (!pwmdata->dma_wait) {}
        pwmdata->dma_wait = 0;

        /* disable irq */
        set_pwm_irq_mask(id, 1);

        /* wait fifo data is empty */
        while (!pwm_get_bit(PWM_DFER, id));

        /* disable dma */
        pwm_write_reg(PWM_DEC, 1 << id);

        pwmdata->dma_data = NULL;
    }
}

unsigned long pwm_dma_get_freq(struct pwm_handle *pwmdata)
{
    pwmdata->rate = clk_src_rate;

    return pwmdata->rate;
}

int pwm_dma_stop(struct pwm_handle *pwmdata)
{
    int ret = 0;
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        pwm_error("pwm channel not support");
        return -1;
    }

    if (pwmdata->is_config != PWM_CONFIG_DMA_MODE) {
        pwm_error("not config dma mode");
        ret = -1;
        goto unlock;
    }

    if (!pwmdata->is_enable) {
        pwm_error("dma is not enable");
        ret = -1;
        goto unlock;
    }

    if (!pwmdata->dma_loop) {
        pwm_error("dma is not loop");
        ret = -1;
        goto unlock;
    }

    /* disable dma */
    pwm_write_reg(PWM_DEC, 1 << id);

    /* wait fifo data is empty */
    while (!pwm_get_bit(PWM_DFER, id));

    pwmdata->dma_data = NULL;
    pwmdata->dma_loop = 0;

unlock:
    return ret;
}

static void pwm_irq_handler(int irq, void *data)
{
    int i;
    unsigned int end_status_reg;

    end_status_reg = pwm_read_reg(PWM_DSR);
    for (i = 0; i < PWM_NUMS; i++) {
        if ((end_status_reg >> i) & 0x1)
            pwmdatas[i]->dma_wait = 1;
    }
    pwm_write_reg(PWM_DSCR, end_status_reg);
}

int pwm_init(void)
{
    int ret;

    clk_src_rate = cpccr_get_rate(CLK_ID_H2CLK);
    ret = cgu_set_parent(CLK_ID_CGU_PWM, CLK_ID_MPLL);
    if (ret)
        pwm_error("cgu_set_parent fail");

    cgu_set_rate(CLK_ID_CGU_PWM, clk_src_rate);
    cgu_enable(CLK_ID_CGU_PWM, 1);

    clk_enable(CLK_GATE_PWM, 1);

    request_irq(IRQ_PWM, 0, pwm_irq_handler, "pwm", NULL);

    return 0;
}

void pwm_exit(void)
{
    release_irq(IRQ_PWM);
    clk_enable(CLK_GATE_PWM, 0);
    cgu_enable(CLK_ID_CGU_PWM, 0);
}
