#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include <driver/dma.h>
#include <cpu/uncache_mem.h>

#define PWM_ENS                         0x00
#define PWM_ENC                         0x04
#define PWM_EN                          0x08
#define PWM_UPT                         0x10
#define PWM_BSY                         0x14
#define PWM_FIS                         0x18
#define PWM_FS                          0x1C
#define PWM_MS                          0x20
#define PWM_INL                         0x24
#define PWM_IDL                         0x28
#define PWM_CC(n)                       (0x40 + (n) * 4)
#define PWM_WC(n)                       (0x80 + (n) * 4)

#define PWM_DR(n)                       (0xC0 + (n) * 4)
#define PWM_DFN(n)                      (0x100 + (n) * 4)
#define PWM_DRTN(n)                     (0x140 + (n) * 4)
#define PWM_DRE                         0x180
#define PWM_DRS                         0x184
#define PWM_DFIE                        0x188
#define PWM_DFIE1                       0x198
#define PWM_DFS                         0x18C
#define PWM_DAFF                        0x190
#define PWM_DCFF                        0x194
#define PWM_SS                          0x200
#define PWM_SIE                         0x204
#define PWM_SIE1                        0x208
#define PWM_SC(n)                       (0x210 + (n) * 4)
#define PWM_SN(n)                       (0x250 + (n) * 4)
#define PWM_ON(n)                       (0x290 + (n) * 4)

#define PWM_CC_div                      0, 15
#define PWM_WAVEFORM_HIGH               16, 31
#define PWM_WAVEFORM_LOW                0, 15

#define PWM_NUMS                        16
#define PWM_DUTY_MAX_COUNT              (0xFFFF)
#define PWM_PRESCALE_MAX_COUNT          (0xFFFF)

#define PWM_REG_BASE                    PWM_IOBASE
#define PWM_ADDR(reg)                   ((volatile unsigned long *)(PWM_REG_BASE + (reg)))

#define PWM_CONFIG_REG_MODE             1
#define PWM_CONFIG_DMA_MODE             2

#define PWM_DMA_TRIGGER_NUM             (APP_libmcu_x2600_pwm_dma_trig_num)

void pwm_dump_regs(int ch);

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

static unsigned long clk_src_rate;

static struct pwm_handle_gpio pwm_gpio_array[] = {
    { .id = 0 , .func = GPIO_FUNC_2, .gpio = GPIO_PB(12), },
    { .id = 1 , .func = GPIO_FUNC_2, .gpio = GPIO_PB(13), },
    { .id = 2 , .func = GPIO_FUNC_2, .gpio = GPIO_PB(14), },
    { .id = 3 , .func = GPIO_FUNC_2, .gpio = GPIO_PB(15), },
    { .id = 4 , .func = GPIO_FUNC_2, .gpio = GPIO_PB(16), },
    { .id = 5 , .func = GPIO_FUNC_2, .gpio = GPIO_PB(17), },
    { .id = 6 , .func = GPIO_FUNC_2, .gpio = GPIO_PB(18), },
    { .id = 7 , .func = GPIO_FUNC_2, .gpio = GPIO_PB(19), },
    { .id = 8 , .func = GPIO_FUNC_0, .gpio = GPIO_PC(7) , },
    { .id = 9 , .func = GPIO_FUNC_0, .gpio = GPIO_PC(8) , },
    { .id = 10, .func = GPIO_FUNC_0, .gpio = GPIO_PC(9) , },
    { .id = 10, .func = GPIO_FUNC_2, .gpio = GPIO_PE(2) , },
    { .id = 11, .func = GPIO_FUNC_0, .gpio = GPIO_PC(10), },
    { .id = 12, .func = GPIO_FUNC_0, .gpio = GPIO_PC(11), },
    { .id = 13, .func = GPIO_FUNC_0, .gpio = GPIO_PC(12), },
    { .id = 14, .func = GPIO_FUNC_0, .gpio = GPIO_PC(13), },
    { .id = 15, .func = GPIO_FUNC_0, .gpio = GPIO_PC(14), },
};

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

static void get_pwm_source_rate_freq_first(struct pwm_handle *pwmdata, unsigned long freq)
{
    unsigned long rate;
    unsigned int duty_count;
    unsigned int prescale;

    rate = clk_src_rate;
    duty_count = rate / freq;
    if (duty_count > PWM_DUTY_MAX_COUNT) {
        prescale = (duty_count / PWM_PRESCALE_MAX_COUNT) + 1;
        rate = clk_src_rate / prescale;
    } else
        rate = clk_src_rate;

    pwmdata->rate = rate;
    pwmdata->full_num = rate / freq;
    if (pwmdata->full_num > PWM_DUTY_MAX_COUNT)
        pwmdata->full_num = PWM_DUTY_MAX_COUNT;

    pwmdata->real_rate = rate / pwmdata->full_num;
}

static void get_pwm_source_rate_levels_first(struct pwm_handle *pwmdata, unsigned long freq, unsigned long levels)
{
    unsigned long rate;
    unsigned long delta;
    unsigned int duty_count;
    unsigned int prescale;

    rate = clk_src_rate;
    duty_count = rate / freq;
    if (duty_count > PWM_DUTY_MAX_COUNT) {
        prescale = (duty_count / PWM_PRESCALE_MAX_COUNT) + 1;
        rate = clk_src_rate / prescale;
    } else
        rate = clk_src_rate;

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

static void pwm_set_clk_div(struct pwm_handle *pwmdata)
{
    unsigned long div, tmp;
    int id = pwmdata->gpio_def->id;

    tmp = clk_src_rate / pwmdata->rate;

    if (tmp <= PWM_PRESCALE_MAX_COUNT) {
        div = tmp - 1;
    } else {
        div = PWM_PRESCALE_MAX_COUNT;
        printf("PWM: clk div err, pwm ch %d !\n", id);
    }

    pwm_set_bits(PWM_CC(id), PWM_CC_div, div);
}

static void set_pwm_idle_level(struct pwm_handle *pwmdata, int level)
{
    int id = pwmdata->gpio_def->id;

    pwm_set_bit(PWM_IDL, id, !!level);
}

static void set_pwm_init_level(struct pwm_handle *pwmdata, int level)
{
    int id = pwmdata->gpio_def->id;

    pwm_set_bit(PWM_INL, id, !!level);
}

static void set_pwm_mode(struct pwm_handle *pwmdata, int dma_mode)
{
    int id = pwmdata->gpio_def->id;

    pwm_set_bit(PWM_MS, id, !!dma_mode);
}

static void pwm_enable(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    if (pwmdata->is_enable)
        return;

    gpio_set_func(pwmdata->gpio_def->gpio, pwmdata->gpio_def->func);

    /* enable pwm */
    if (!pwmdata->not_really_enable)
        pwm_write_reg(PWM_ENS, 1 << id);

    pwmdata->is_enable = 1;
}

static void pwm_disable(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    if (!pwmdata->is_enable)
        return;

    /* disable pwm */
    pwm_write_reg(PWM_ENC, 1 << id);

    pwmdata->is_enable = 0;
}

static void set_pwm_dma_req(struct pwm_handle *pwmdata, int enable)
{
    int id = pwmdata->gpio_def->id;

    pwm_set_bit(PWM_DRE, id, !!enable);
}

static void pwm_dma_fifo_flush(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    pwm_set_bit(PWM_DCFF, id, 1);
}

static int pwm_update(struct pwm_handle *pwmdata)
{
    unsigned long timeout;
    int id = pwmdata->gpio_def->id;
    int retry = 2;

    /* wait 0~2 cycle to update pwm*/
    do {
        if (!pwm_get_bit(PWM_BSY, id))
            break;

        if (retry-- == 0) {
            printf("PWM: pwm updata waveform config timeout\n");
            return -1;
        }

        timeout = 1*1000*1000 / pwmdata->real_rate;
        if (timeout == 0)
            timeout = 1;

        udelay(timeout);
    } while (pwm_get_bit(PWM_BSY, id));

    pwm_write_reg(PWM_UPT, 1 << id);

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
    pwm_write_reg(PWM_ENS, channels);
}

void pwm_disable_channels(unsigned int channels)
{
    pwm_write_reg(PWM_ENC, channels);
}

int pwm_request(struct pwm_handle *pwmdata, int gpio)
{
    int id;
    struct pwm_handle_gpio *gpio_def;

    gpio_def = pwm_get_gpio_func_def(gpio);
    if (!gpio_def) {
        printf("PWM: gpio not support as pwm\n");
        return -1;
    }

    id = gpio_def->id;

    if (pwmdata->is_request) {
        printf("PWM: pwm ch %d, pwm already request\n", id);
        return -1;
    }

    pwmdata->gpio_def = gpio_def;
    pwmdata->rate = clk_src_rate;
    pwmdata->is_request = 1;

    return 0;
}

int pwm_config(struct pwm_handle *pwmdata, struct pwm_config_data *config)
{
    unsigned long freq = config->freq;
    unsigned long levels = config->levels;
    unsigned long idle_level = config->idle_level;
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        printf("PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    if (!freq) {
        printf("PWM: pwm ch %d only support frequency high than 0 Hz\n", id);
        return -1;
    }

    if (freq > clk_src_rate) {
        printf("PWM: pwm only support frequency low than %lu\n", clk_src_rate);
        return -1;
    }

    if (levels >= PWM_DUTY_MAX_COUNT) {
        printf("PWM: pwm ch %d levels need less than %d\n", id, PWM_DUTY_MAX_COUNT);
        return -1;
    }

    if (!pwmdata->is_request) {
        printf("PWM: pwm ch %d is not request!\n", id);
        return -1;
    }

    if (pwmdata->is_enable) {
        printf("PWM: pwm ch %d Cannot configure at working\n", id);
        return -1;
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
    set_pwm_idle_level(pwmdata, pwmdata->idle_level);
    set_pwm_init_level(pwmdata, !pwmdata->idle_level);

    /* updata mode */
    set_pwm_mode(pwmdata, 0);
    set_pwm_dma_req(pwmdata, 0);

    pwmdata->is_config = PWM_CONFIG_REG_MODE;

    return 0;
}

void pwm_release(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        printf("PWM: No support pwm ch %d !\n", id);
        return;
    }

    if (pwmdata->is_config == PWM_CONFIG_DMA_MODE && pwmdata->dma_loop)
        pwm_dma_stop(pwmdata);
    else if (pwmdata->is_config == PWM_CONFIG_DMA_MODE && pwmdata->dma_queue)
        pwm_dma_queue_stop(pwmdata);

    pwm_disable(pwmdata);

    if (pwmdata->pwm_dma) {
        pwm_dma_fifo_flush(pwmdata);
        dma_release(pwmdata->pwm_dma);
        pwmdata->pwm_dma = NULL;
    }

    if (pwmdata->is_request)
        pwmdata->is_request = 0;

    pwmdata->is_config = 0;
}

int pwm_set_level(struct pwm_handle *pwmdata, unsigned long level)
{
    unsigned long pwm_level = 0;
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        printf("PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    if (pwmdata->is_config != PWM_CONFIG_REG_MODE) {
        printf("PWM: pwm ch%d is not config!\n", id);
        return -1;
    }

    if (level > pwmdata->levels) {
        printf("PWM: pwm ch%d set level more than max level!\n", id);
        return -1;
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
        pwm_level = set_bit_field(pwm_level, PWM_WAVEFORM_LOW, 1);
        pwm_level = set_bit_field(pwm_level, PWM_WAVEFORM_HIGH, pwmdata->full_num - 1);
        pwm_write_reg(PWM_WC(id), pwm_level);
        // pwm_write_reg(PWM_UPT, 1 << id);

        if (!pwmdata->not_really_disable)
            pwm_disable(pwmdata);
        return 0;
    } else if (pwmdata->high_levels == 0) {
        pwmdata->func_is_clear = 1;
        gpio_set_func(pwmdata->gpio_def->gpio, GPIO_OUTPUT0);
        pwm_level = set_bit_field(pwm_level, PWM_WAVEFORM_LOW, pwmdata->full_num - 1);
        pwm_level = set_bit_field(pwm_level, PWM_WAVEFORM_HIGH, 1);
        pwm_write_reg(PWM_WC(id), pwm_level);
        // pwm_write_reg(PWM_UPT, 1 << id);

        if (!pwmdata->not_really_disable)
            pwm_disable(pwmdata);

        return 0;
    }
    if (pwmdata->func_is_clear) {
        pwmdata->func_is_clear = 0;
        gpio_set_func(pwmdata->gpio_def->gpio, pwmdata->gpio_def->func);
    }

    pwm_level = set_bit_field(pwm_level, PWM_WAVEFORM_LOW, pwmdata->low_levels);
    pwm_level = set_bit_field(pwm_level, PWM_WAVEFORM_HIGH, pwmdata->high_levels);
    pwm_write_reg(PWM_WC(id), pwm_level);

    /* set pwm updata */
    if (pwm_update(pwmdata) < 0)
        return -1;

    pwm_enable(pwmdata);

    return 0;
}

unsigned long pwm_get_freq(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        printf("PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    return pwmdata->real_rate;
}

int pwm_get_id(struct pwm_handle *pwmdata)
{
    return pwmdata->gpio_def->id;
}


static void dma_tx_callback(void *data)
{
    struct pwm_handle *pwmdata = (void *)data;

    if (!pwmdata->dma_loop) {
        pwmdata->dma_complete = 1;
    }
}

int pwm_dma_init(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data)
{
    int id = pwmdata->gpio_def->id;
    void *irq_cb = NULL;

    if (id < 0 || id >= PWM_NUMS) {
        printf("PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    if (!pwmdata->is_request) {
        printf("PWM: pwm ch %d is not request!\n", id);
        return -1;
    }

    if (pwmdata->is_enable) {
        printf("PWM: pwm ch %d Cannot configure at working!\n", id);
        return -1;
    }

    /* set pwm clk div, count clk must not less than AHB2 clk*/
    pwmdata->rate = dma_data->src_rate;
    pwm_set_clk_div(pwmdata);

    /* set pwm idle and init level */
    pwmdata->init_level = !!dma_data->start_level;
    pwmdata->idle_level = !!dma_data->idle_level;
    set_pwm_init_level(pwmdata, pwmdata->init_level);
    set_pwm_idle_level(pwmdata, pwmdata->idle_level);

    /* updata dma mode */
    set_pwm_mode(pwmdata, 1);

    /* set dma request trigger number */
    pwm_write_reg(PWM_DRTN(id), PWM_DMA_TRIGGER_NUM);

    if (dma_data->dma_complete_cb)
        irq_cb = dma_data->dma_complete_cb;
    else
        irq_cb = dma_tx_callback;

    struct dma *dma = dma_request(DMA_RQ_PWM0_TX + id, irq_cb, pwmdata, DMA_bus_32bit, 4);

    /* enable dma */
    set_pwm_dma_req(pwmdata, 1);

    pwmdata->is_config = PWM_CONFIG_DMA_MODE;
    pwmdata->pwm_dma = dma;

    return 0;
}

static int pwm_dma_check_update(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data)
{
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        printf("PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    if (!dma_data || !dma_data->data || !dma_data->data_count) {
        printf("PWM: pwm ch%d not have dma data!\n", id);
        return -1;
    }

#ifdef PWM_CHECK_DMA_DATA
    for (int i = 0; i < dma_data->data_count; i++) {
        if (!dma_data->data[i].high || !dma_data->data[i].low) {
            printf("PWM: pwm ch%d dma mode, data cannot be zero\n", id);
            return -1;
        }
    }
#endif

    if (pwmdata->is_config != PWM_CONFIG_DMA_MODE) {
        printf("PWM: pwm ch%d is not config dma mode!\n", id);
        return -1;
    }

    if (pwmdata->dma_loop) {
        printf("PWM: pwm ch%d work in dma loop mode !\n", id);
        return -1;
    }

    return 0;
}

int pwm_dma_update(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data)
{
    int id = pwmdata->gpio_def->id;
    int data_len;
    int data_count;

    if (pwm_dma_check_update(pwmdata, dma_data) < 0)
        return -1;

    data_count = dma_data->data_count / PWM_DMA_TRIGGER_NUM;
    if (dma_data->data_count % PWM_DMA_TRIGGER_NUM)
        data_count++;

    data_len = dma_data->data_count * sizeof(struct pwm_data);

    if (data_len % data_count) {
        printf("PWM: pwm ch%d dma data length not support !\n", id);
        return -1;
    }

    assert(!pwmdata->dma_data);
    pwmdata->dma_data = dma_data->data;

    /* update dma loop mode */
    pwmdata->dma_loop = !!dma_data->dma_loop;
    pwmdata->dma_complete = 0;

    if (dma_data->dma_loop)
        dma_start_cyclic(pwmdata->pwm_dma, pwmdata->dma_data, (void *)PWM_ADDR(PWM_DR(id)), data_len, data_count);
    else
        dma_start(pwmdata->pwm_dma, pwmdata->dma_data, (void *)PWM_ADDR(PWM_DR(id)), data_len);

    pwm_enable(pwmdata);

    return 0;
}

int pwm_dma_start(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data)
{
    int ret = pwm_dma_init(pwmdata, dma_data);
    if (ret < 0)
        return ret;

    ret = pwm_dma_update(pwmdata, dma_data);

    return ret;
}

void pwm_dma_wait_end(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data)
{
    /* normal mode wait dma complete */
    if (!pwmdata->dma_loop && !dma_data->dma_complete_cb) {
        while (!pwmdata->dma_complete);

        /* disable dma (!dma_loop not need use dma_stop) */
        // dma_stop(pwmdata->pwm_dma);

        pwmdata->dma_data = NULL;
    }
}

int pwm_dma_stop(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        printf("PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    if (pwmdata->is_config != PWM_CONFIG_DMA_MODE) {
        printf("PWM: pwm ch%d is not config dma mode!\n", id);
        return -1;
    }

    if (!pwmdata->is_enable) {
        printf("PWM: pwm ch%d dma is not enable\n", id);
        return -1;
    }

    if (!pwmdata->dma_loop) {
        printf("PWM: pwm ch%d dma is not loop\n", id);
        return -1;
    }

    /* disable dma request */
    set_pwm_dma_req(pwmdata, 0);

    /* disable dma */
    dma_stop(pwmdata->pwm_dma);

    pwmdata->is_enable = 0;

    pwmdata->dma_data = NULL;
    pwmdata->dma_loop = 0;
    pwmdata->dma_queue = 0;

    return 0;
}

unsigned long pwm_dma_get_freq(struct pwm_handle *pwmdata)
{
    return pwmdata->rate;
}

int pwm_dma_queue_init(struct pwm_handle *pwmdata, struct pwm_dma_data *dma_data)
{
    int id = pwmdata->gpio_def->id;

    pwm_dma_init(pwmdata, dma_data);

    if (pwm_dma_check_update(pwmdata, dma_data) < 0)
        return -1;

    if (pwmdata->dma_queue) {
        printf("PWM: pwm ch%d work in dma queue mode !\n", id);
        return -1;
    }

    /* update dma loop mode */
    pwmdata->dma_complete = 0;
    pwmdata->dma_queue = 1;

    dma_queue_desc_init(pwmdata->pwm_dma, (void *)PWM_ADDR(PWM_DR(id)), 4, 1);

    dma_queue_init(pwmdata->pwm_dma);

    return 0;
}

int pwm_dma_queue_add(struct pwm_handle *pwmdata, void *buf, unsigned int count)
{
    return dma_queue_add(pwmdata->pwm_dma, buf, count * sizeof(struct pwm_data));
}

void pwm_dma_queue_start(struct pwm_handle *pwmdata)
{
    dma_queue_start(pwmdata->pwm_dma);

    pwm_enable(pwmdata);
}

int pwm_dma_queue_get_available_size(struct pwm_handle *pwmdata)
{
    return dma_queue_get_available_size(pwmdata->pwm_dma);
}

int pwm_dma_queue_stop(struct pwm_handle *pwmdata)
{
    int id = pwmdata->gpio_def->id;

    if (id < 0 || id >= PWM_NUMS) {
        printf("PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    if (pwmdata->is_config != PWM_CONFIG_DMA_MODE) {
        printf("PWM: pwm ch%d is not config dma mode!\n", id);
        return -1;
    }

    if (!pwmdata->is_enable) {
        printf("PWM: pwm ch%d dma is not enable\n", id);
        return -1;
    }

    if (!pwmdata->dma_queue) {
        printf("PWM: pwm ch%d dma is not dma queue\n", id);
        return -1;
    }

    /* disable dma request */
    set_pwm_dma_req(pwmdata, 0);

    /* disable dma queue */
    dma_queue_deinit(pwmdata->pwm_dma);

    pwmdata->dma_queue = 0;

    return 0;
}

int pwm_init(void)
{
    clk_src_rate = clk_div_get_rate(CLK_DIV_PWM);
    clk_div_set_rate(CLK_DIV_PWM, clk_src_rate);

    clk_div_enable(CLK_DIV_PWM);
    clk_gate_enable(CLK_GATE_PWM);

    return 0;
}

void pwm_exit(void)
{
    clk_gate_disable(CLK_GATE_PWM);
    clk_div_disable(CLK_DIV_PWM);
}

void pwm_dump_regs(int ch)
{
    printf("PWM_ENS:0x%08x\n", pwm_read_reg(PWM_ENS));
    printf("PWM_ENC:0x%08x\n", pwm_read_reg(PWM_ENC));
    printf("PWM_EN:0x%08x\n", pwm_read_reg(PWM_EN));
    printf("PWM_UPT:0x%08x\n", pwm_read_reg(PWM_UPT));
    printf("PWM_BSY:0x%08x\n", pwm_read_reg(PWM_BSY));
    printf("PWM_FIS:0x%08x\n", pwm_read_reg(PWM_FIS));
    printf("PWM_FS:0x%08x\n", pwm_read_reg(PWM_FS));
    printf("PWM_MS:0x%08x\n", pwm_read_reg(PWM_MS));
    printf("PWM_INL:0x%08x\n", pwm_read_reg(PWM_INL));
    printf("PWM_IDL:0x%08x\n", pwm_read_reg(PWM_IDL));
    printf("PWM_CC(%d):0x%08x\n", ch, pwm_read_reg(PWM_CC(ch)));
    printf("PWM_WC(%d):0x%08x\n", ch, pwm_read_reg(PWM_WC(ch)));

    printf("PWM_DR(%d):0x%08x\n", ch, pwm_read_reg(PWM_DR(ch)));
    printf("PWM_DFN(%d):0x%08x\n", ch, pwm_read_reg(PWM_DFN(ch)));
    printf("PWM_DRTN(%d):0x%08x\n", ch, pwm_read_reg(PWM_DRTN(ch)));
    printf("PWM_DRE:0x%08x\n", pwm_read_reg(PWM_DRE));
    printf("PWM_DRS:0x%08x\n", pwm_read_reg(PWM_DRS));
    printf("PWM_DFIE:0x%08x\n", pwm_read_reg(PWM_DFIE));
    printf("PWM_DFIE1:0x%08x\n", pwm_read_reg(PWM_DFIE1));
    printf("PWM_DFS:0x%08x\n", pwm_read_reg(PWM_DFS));
    printf("PWM_DAFF:0x%08x\n", pwm_read_reg(PWM_DAFF));
    printf("PWM_DCFF:0x%08x\n", pwm_read_reg(PWM_DCFF));
    printf("PWM_SS:0x%08x\n", pwm_read_reg(PWM_SS));
    printf("PWM_SIE:0x%08x\n", pwm_read_reg(PWM_SIE));
    printf("PWM_SIE1:0x%08x\n", pwm_read_reg(PWM_SIE1));
    printf("PWM_SC(%d):0x%08x\n", ch, pwm_read_reg(PWM_SC(ch)));
    printf("PWM_SN(%d):0x%08x\n", ch, pwm_read_reg(PWM_SN(ch)));
    printf("PWM_ON(%d):0x%08x\n", ch, pwm_read_reg(PWM_ON(ch)));
}
