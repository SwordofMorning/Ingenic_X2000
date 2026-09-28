#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <bit_field.h>
#include <utils/gpio.h>
#include <utils/clock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>

#include <common.h>
#include <pwm.h>

#define PWM_ENS                         0x0000
#define PWM_ENC                         0x0004
#define PWM_EN                          0x0008
#define PWM_UPT                         0x0010
#define PWM_BSY                         0x0014
#define PWM_FIS                         0x0018
#define PWM_FS                          0x001C
#define PWM_MS                          0x0020
#define PWM_INL                         0x0024
#define PWM_IDL                         0x0028
#define PWM_CC(n)                       (0x0040 + (n) * 4)
#define PWM_WC(n)                       (0x0080 + (n) * 4)

#define PWM_DR(n)                       (0x00C0 + (n) * 4)
#define PWM_DFN(n)                      (0x0100 + (n) * 4)
#define PWM_DRTN(n)                     (0x0140 + (n) * 4)
#define PWM_DRE                         0x0180
#define PWM_DRS                         0x0184
#define PWM_DFIE                        0x0188
#define PWM_DFIE1                       0x0198
#define PWM_DFS                         0x018C
#define PWM_DAFF                        0x0190
#define PWM_DCFF                        0x0194
#define PWM_SS                          0x0200
#define PWM_SIE                         0x0204
#define PWM_SIE1                        0x0208
#define PWM_SC(n)                       (0x0210 + (n) * 4)
#define PWM_SN(n)                       (0x0250 + (n) * 4)
#define PWM_ON(n)                       (0x0290 + (n) * 4)

#define PWM_CC_div                      0, 15
#define PWM_WAVEFORM_HIGH               16, 31
#define PWM_WAVEFORM_LOW                0, 15

#define PWM_NUMS                        16
#define PWM_DUTY_MAX_COUNT              (0xFFFF)
#define PWM_PRESCALE_MAX_COUNT          (0xFFFF)

#define PWM_MAGIC_NUMBER                'P'
#define PWM_REQUEST                     _IOW(PWM_MAGIC_NUMBER, 11, char)
#define PWM_RELEASE                     _IOW(PWM_MAGIC_NUMBER, 22, unsigned int)
#define PWM_CONFIG                      _IOW(PWM_MAGIC_NUMBER, 33, struct pwm_config_data)
#define PWM_SET_LEVEL                   _IOWR(PWM_MAGIC_NUMBER, 44, unsigned int)
#define PWM_DMA_INIT                    _IOW(PWM_MAGIC_NUMBER, 55, struct pwm_dma_config)
#define PWM_DMA_UPDATE                  _IOWR(PWM_MAGIC_NUMBER, 66, struct pwm_dma_data)
#define PWM_DMA_DISABLE_LOOP            _IOW(PWM_MAGIC_NUMBER, 77, char)
#define PWM_NOT_REALLY_ENABLE           _IOW(PWM_MAGIC_NUMBER, 88, unsigned int)
#define PWM_NOT_REALLY_DISABLE          _IOW(PWM_MAGIC_NUMBER, 89, unsigned int)
#define PWM_ENABLE_CHANNELS             _IOW(PWM_MAGIC_NUMBER, 98, unsigned int)
#define PWM_DISABLE_CHANNELS            _IOW(PWM_MAGIC_NUMBER, 99, unsigned int)

#define PWM_IOBASE                      0x13610000
#define PWM_REG_BASE                    KSEG1ADDR(PWM_IOBASE)
#define PWM_ADDR(reg)                   ((volatile unsigned long *)(PWM_REG_BASE + (reg)))

#define IRQ_PWM                         (IRQ_INTC_BASE + 6)

#define INGENIC_DMA_REQ_ASOC_PWM	    0x2c

#define PWM_CONFIG_REG_MODE             1
#define PWM_CONFIG_DMA_MODE             2

#define PWM_DMA_TRIGGER_NUM             32

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
    set_bit_field(PWM_ADDR(reg), bit, bit, value);
}

static inline unsigned int pwm_get_bit(unsigned int reg, int bit)
{
    return get_bit_field(PWM_ADDR(reg), bit, bit);
}

static inline void pwm_set_bits(unsigned int reg, int start, int end, unsigned int value)
{
    set_bit_field(PWM_ADDR(reg), start, end, value);
}

static inline unsigned int pwm_get_bits(unsigned int reg, int start, int end)
{
    return get_bit_field(PWM_ADDR(reg), start, end);
}

struct jz_pwm_gpio {
    const char *name;
    int id;
    int gpio;
    int func;
};

struct jz_pwm {
    const char *name;
    unsigned int is_config;
    unsigned int is_request;
    unsigned int is_enable;
    unsigned int func_is_clear;
    unsigned int not_really_disable;
    unsigned int not_really_enable;
    unsigned long rate;
    unsigned long real_rate;
    unsigned long half_num;
    unsigned long full_num;
    unsigned int levels;
    unsigned int idle_level;
    unsigned int high_levels;
    unsigned int low_levels;
    struct jz_pwm_gpio *gpio_def;

    unsigned int init_level;
    unsigned int dma_loop;
    void *dma_data;
    unsigned int dma_data_len;
    dma_addr_t dma_handle;

    struct mutex lock;
    wait_queue_head_t dma_wait;
    unsigned int dma_complete;
    struct dma_chan *dma_chan;
};

static DEFINE_SPINLOCK(pwm_lock);
struct jz_pwm pwmdata[PWM_NUMS];

static struct clk *pwm_gate_clk;
static struct clk *pwm_div_clk;
static struct clk *pwm_mux_clk;
static unsigned long clk_src_rate;
static struct device *platform_dev;

struct jz_pwm_gpio pwm_gpio_array[] = {
    { .name = "pwm0", .id = 0, .func = GPIO_FUNC_2, .gpio = GPIO_PB(12), },
    { .name = "pwm1", .id = 1, .func = GPIO_FUNC_2, .gpio = GPIO_PB(13), },
    { .name = "pwm2", .id = 2, .func = GPIO_FUNC_2, .gpio = GPIO_PB(14), },
    { .name = "pwm3", .id = 3, .func = GPIO_FUNC_2, .gpio = GPIO_PB(15), },
    { .name = "pwm4", .id = 4, .func = GPIO_FUNC_2, .gpio = GPIO_PB(16), },
    { .name = "pwm5", .id = 5, .func = GPIO_FUNC_2, .gpio = GPIO_PB(17), },
    { .name = "pwm6", .id = 6, .func = GPIO_FUNC_2, .gpio = GPIO_PB(18), },
    { .name = "pwm7", .id = 7, .func = GPIO_FUNC_2, .gpio = GPIO_PB(19), },
    { .name = "pwm8", .id = 8, .func = GPIO_FUNC_0, .gpio = GPIO_PC(7), },
    { .name = "pwm9", .id = 9, .func = GPIO_FUNC_0, .gpio = GPIO_PC(8), },
    { .name = "pwm10", .id = 10, .func = GPIO_FUNC_0, .gpio = GPIO_PC(9), },
    { .name = "pwm10", .id = 10, .func = GPIO_FUNC_2, .gpio = GPIO_PE(2), },
    { .name = "pwm11", .id = 11, .func = GPIO_FUNC_0, .gpio = GPIO_PC(10), },
    { .name = "pwm12", .id = 12, .func = GPIO_FUNC_0, .gpio = GPIO_PC(11), },
    { .name = "pwm13", .id = 13, .func = GPIO_FUNC_0, .gpio = GPIO_PC(12), },
    { .name = "pwm14", .id = 14, .func = GPIO_FUNC_0, .gpio = GPIO_PC(13), },
    { .name = "pwm15", .id = 15, .func = GPIO_FUNC_0, .gpio = GPIO_PC(14), },

};

static struct jz_pwm_gpio *pwm_get_gpio_func_def(int gpio)
{
    int i;
    struct jz_pwm_gpio *def;

    for (i = 0; i < ARRAY_SIZE(pwm_gpio_array); i++) {
        def = &pwm_gpio_array[i];
        if (def->gpio == gpio)
            return def;
    }

    return NULL;
}

static void pwm_set_clk_div(int id)
{
    unsigned long div, tmp;
    unsigned long flags;

    tmp = clk_src_rate / pwmdata[id].rate;

    if (tmp <= PWM_PRESCALE_MAX_COUNT) {
        div = tmp - 1;
    } else {
        div = PWM_PRESCALE_MAX_COUNT;
        printk(KERN_ERR "PWM: clk div err, pwm ch %d !\n", id);
    }

    spin_lock_irqsave(&pwm_lock, flags);

    pwm_set_bits(PWM_CC(id), PWM_CC_div, div);

    spin_unlock_irqrestore(&pwm_lock, flags);
}

static void pwm_enable(int id)
{
    if (pwmdata[id].is_enable)
        return;

    gpio_set_func(pwmdata[id].gpio_def->gpio, pwmdata[id].gpio_def->func);

    /* enable pwm */
    if (!pwmdata[id].not_really_enable)
        pwm_write_reg(PWM_ENS, 1 << id);

    pwmdata[id].is_enable = 1;
}

static void pwm_disable(int id)
{
    if (!pwmdata[id].is_enable)
        return;

    /* disable pwm */
    pwm_write_reg(PWM_ENC, 1 << id);

    pwmdata[id].is_enable = 0;
}

static void set_pwm_init_level(int id, int level)
{
    unsigned long flags;

    spin_lock_irqsave(&pwm_lock, flags);
    pwm_set_bit(PWM_INL, id, !!level);
    spin_unlock_irqrestore(&pwm_lock, flags);
}

static void set_pwm_idle_level(int id, int level)
{
    unsigned long flags;

    spin_lock_irqsave(&pwm_lock, flags);
    pwm_set_bit(PWM_IDL, id, !!level);
    spin_unlock_irqrestore(&pwm_lock, flags);
}

static void set_pwm_mode(int id, int dma_mode)
{
    unsigned long flags;

    spin_lock_irqsave(&pwm_lock, flags);
    pwm_set_bit(PWM_MS, id, !!dma_mode);
    spin_unlock_irqrestore(&pwm_lock, flags);
}

static void get_pwm_source_rate_freq_first(int id, unsigned long freq)
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

    pwmdata[id].rate = rate;
    pwmdata[id].full_num = rate / freq;
    if (pwmdata[id].full_num > PWM_DUTY_MAX_COUNT)
        pwmdata[id].full_num = PWM_DUTY_MAX_COUNT;

    pwmdata[id].real_rate = rate / pwmdata[id].full_num;
}

static void get_pwm_source_rate_levels_first(int id, unsigned long freq, unsigned long levels)
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

    pwmdata[id].rate = rate;

    if (rate / freq < levels) {
        pwmdata[id].full_num = levels;
    } else {
        delta = (rate / freq) % levels;
        pwmdata[id].full_num = rate / freq - delta;
    }

    if (pwmdata[id].full_num > PWM_DUTY_MAX_COUNT)
        pwmdata[id].full_num = PWM_DUTY_MAX_COUNT;

    pwmdata[id].real_rate = rate / pwmdata[id].full_num;
}

static void set_pwm_dma_req(int id, int enable)
{
    unsigned long flags;

    spin_lock_irqsave(&pwm_lock, flags);
    pwm_set_bit(PWM_DRE, id, !!enable);
    spin_unlock_irqrestore(&pwm_lock, flags);
}

void pwm_set_not_really_disable(int id, int enable)
{
    pwmdata[id].not_really_disable = !!enable;
}

void pwm_set_not_really_enable(int id, int enable)
{
    pwmdata[id].not_really_enable = !!enable;
}

void pwm_enable_channels(unsigned int channels)
{
    pwm_write_reg(PWM_ENS, channels);
}

void pwm_disable_channels(unsigned int channels)
{
    pwm_write_reg(PWM_ENC, channels);
}


unsigned long soc_pwm_get_level(int id)
{
    if (id < 0 || id >= PWM_NUMS) {
        printk(KERN_ERR "PWM: %s, No support pwm ch %d !\n", __func__, id);
        return -1;
    }

    int save_level;
    if (pwmdata[id].idle_level) {
        save_level = pwmdata[id].low_levels;
    } else {
        save_level = pwmdata[id].high_levels;
    }

    int level = save_level;
    if ((save_level != 0 && save_level != pwmdata[id].full_num) && pwmdata[id].is_enable) {
        if (pwmdata[id].idle_level) {
            level = pwm_get_bits(PWM_WC(id), PWM_WAVEFORM_LOW);
        } else {
            level = pwm_get_bits(PWM_WC(id), PWM_WAVEFORM_HIGH);
        }
    }

    return level * pwmdata[id].levels / pwmdata[id].full_num;
}
EXPORT_SYMBOL_GPL(soc_pwm_get_level);

int pwm2_request(int gpio, const char *name)
{
    int id;
    char buf[8];
    struct jz_pwm_gpio *gpio_def;

    gpio_def = pwm_get_gpio_func_def(gpio);
    if (!gpio_def) {
        printk(KERN_ERR "PWM: %s not support as pwm.\n", gpio_to_str(gpio, buf));
        return -1;
    }

    id = gpio_def->id;

    mutex_lock(&pwmdata[id].lock);

    if (pwmdata[id].is_request)
        goto unlock;

    if (gpio_request(gpio, name)) {
        printk(KERN_ERR "PWM: pwm ch %d, request IO %s error !\n", id, gpio_to_str(gpio, buf));
        id = -1;
        goto unlock;
    }

    pwmdata[id].name = name;
    pwmdata[id].gpio_def = gpio_def;
    pwmdata[id].rate = clk_src_rate;
    pwmdata[id].is_request = 1;

unlock:
    mutex_unlock(&pwmdata[id].lock);

    return id;
}
EXPORT_SYMBOL_GPL(pwm2_request);

int pwm2_release(int id)
{
    if (id < 0 || id >= PWM_NUMS) {
        printk(KERN_ERR "PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    mutex_lock(&pwmdata[id].lock);

    if (pwmdata[id].is_config == PWM_CONFIG_DMA_MODE && pwmdata[id].dma_loop)
        pwm2_dma_disable_loop(id);

    if (pwmdata[id].dma_chan) {
        dma_release_channel(pwmdata[id].dma_chan);
        pwmdata[id].dma_chan = NULL;
    }

    pwm_disable(id);

    if (pwmdata[id].is_request) {
        gpio_free(pwmdata[id].gpio_def->gpio);
        pwmdata[id].is_request = 0;
    }

    pwmdata[id].is_config = 0;

    mutex_unlock(&pwmdata[id].lock);

    return 0;
}
EXPORT_SYMBOL_GPL(pwm2_release);

int pwm2_config(int id, struct pwm_config_data *config)
{
    int ret = 0;
    unsigned long freq = config->freq;
    unsigned long levels = config->levels;
    unsigned long idle_level = config->idle_level;

    if (id < 0 || id >= PWM_NUMS) {
        printk(KERN_ERR "PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    if (!freq) {
        printk(KERN_ERR "PWM: pwm ch %d only support frequency high than 0 Hz\n", id);
        return -1;
    }

    if (freq > clk_src_rate) {
        printk(KERN_ERR "PWM: pwm only support frequency low than %lu\n", clk_src_rate);
        return -1;
    }

    if (levels >= PWM_DUTY_MAX_COUNT) {
        printk(KERN_ERR "PWM: pwm ch %d levels need less than %d\n", id, PWM_DUTY_MAX_COUNT);
        return -1;
    }

    mutex_lock(&pwmdata[id].lock);

    if (!pwmdata[id].is_request) {
        printk(KERN_ERR "PWM: pwm ch %d is not request!\n", id);
        ret = -1;
        goto unlock;
    }

    if (pwmdata[id].is_enable) {
        printk(KERN_ERR "PWM: pwm ch %d Cannot configure at working\n", id);
        ret = -1;
        goto unlock;
    }

    pwmdata[id].levels = levels;
    pwmdata[id].idle_level = !!idle_level;

    if (config->accuracy_priority == PWM_accuracy_freq_first)
        get_pwm_source_rate_freq_first(id, freq);
    else
        get_pwm_source_rate_levels_first(id, freq, levels);

    /* set pwm clk div */
    pwm_set_clk_div(id);

    /* set pwm level */
    set_pwm_idle_level(id, pwmdata[id].idle_level);
    set_pwm_init_level(id, !pwmdata[id].idle_level);

    /* updata mode */
    set_pwm_mode(id, 0);

    pwmdata[id].is_config = PWM_CONFIG_REG_MODE;

unlock:
    mutex_unlock(&pwmdata[id].lock);

    return ret;
}
EXPORT_SYMBOL_GPL(pwm2_config);

static int pwm2_update(int id)
{
    unsigned long timeout;
    int retry = 2;
    /* wait 0~2 cycle to update pwm*/
    do {
        if (!pwm_get_bit(PWM_BSY, id))
            break;

        if (retry-- == 0) {
            printk(KERN_ERR "PWM: pwm updata waveform config timeout\n");
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

int pwm2_set_level(int id, unsigned long level)
{
    int ret = 0;
    unsigned long pwm_level = 0;

    if (id < 0 || id >= PWM_NUMS) {
        printk(KERN_ERR "PWM: No support pwm ch %d !\n", id);
        return -1;;
    }

    mutex_lock(&pwmdata[id].lock);

    if (pwmdata[id].is_config != PWM_CONFIG_REG_MODE) {
        printk(KERN_ERR "PWM: pwm ch%d is not config!\n", id);
        ret = -1;
        goto unlock;
    }

    if (level > pwmdata[id].levels) {
        printk(KERN_ERR "PWM: pwm ch%d set level more than max level!\n", id);
        ret = -1;
        goto unlock;
    }

    if (level != 0) {
        level = level * pwmdata[id].full_num / pwmdata[id].levels;
        if (level == 0)
            level = 1;
    }

    pwmdata[id].half_num = level;

    /* set pwm waveform config */
    if (pwmdata[id].idle_level) {
        pwmdata[id].high_levels = pwmdata[id].full_num - level;
        pwmdata[id].low_levels = level;
    } else {
        pwmdata[id].low_levels = pwmdata[id].full_num - level;
        pwmdata[id].high_levels = level;
    }

    /* set pwm init level */
    if (pwmdata[id].low_levels == 0) {
        pwmdata[id].func_is_clear = 1;
        gpio_set_func(pwmdata[id].gpio_def->gpio, GPIO_OUTPUT1);
        set_bit_field(&pwm_level, PWM_WAVEFORM_LOW, 1);
        set_bit_field(&pwm_level, PWM_WAVEFORM_HIGH, pwmdata[id].full_num - 1);
        pwm_write_reg(PWM_WC(id), pwm_level);
        // pwm_write_reg(PWM_UPT, 1 << id);

        if (!pwmdata[id].not_really_disable)
            pwm_disable(id);
        goto unlock;
    } else if (pwmdata[id].high_levels == 0) {
        pwmdata[id].func_is_clear = 1;
        gpio_set_func(pwmdata[id].gpio_def->gpio, GPIO_OUTPUT0);
        set_bit_field(&pwm_level, PWM_WAVEFORM_LOW, pwmdata[id].full_num - 1);
        set_bit_field(&pwm_level, PWM_WAVEFORM_HIGH, 1);
        pwm_write_reg(PWM_WC(id), pwm_level);
        // pwm_write_reg(PWM_UPT, 1 << id);

        if (!pwmdata[id].not_really_disable)
            pwm_disable(id);
        goto unlock;
    }

    if (pwmdata[id].func_is_clear) {
        pwmdata[id].func_is_clear = 0;
        gpio_set_func(pwmdata[id].gpio_def->gpio, pwmdata[id].gpio_def->func);
    }

    set_bit_field(&pwm_level, PWM_WAVEFORM_LOW, pwmdata[id].low_levels);
    set_bit_field(&pwm_level, PWM_WAVEFORM_HIGH, pwmdata[id].high_levels);
    pwm_write_reg(PWM_WC(id), pwm_level);

    /* set pwm updata */
    if (pwm2_update(id) < 0)
        return -1;

    pwm_enable(id);

unlock:
    mutex_unlock(&pwmdata[id].lock);

    return ret;
}
EXPORT_SYMBOL_GPL(pwm2_set_level);

static void dma_tx_callback(void *data)
{
    int id = (int)data;

    if (!pwmdata[id].dma_loop) {
        pwmdata[id].dma_complete = 1;
        wake_up(&pwmdata[id].dma_wait);
    }
}

int pwm2_dma_init(int id, struct pwm_dma_config *dma_config)
{
    int ret = 0;
    dma_cap_mask_t mask;
    struct dma_chan *txchan;
    struct dma_slave_config tx_config;

    if (id < 0 || id >= PWM_NUMS) {
        printk(KERN_ERR "PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    mutex_lock(&pwmdata[id].lock);

    if (!pwmdata[id].is_request) {
        printk(KERN_ERR "PWM: pwm ch %d is not request!\n", id);
        ret = -1;
        goto unlock;
    }

    if (pwmdata[id].is_enable) {
        printk(KERN_ERR "PWM: pwm ch %d Cannot configure at working!\n", id);
        ret = -1;
        goto unlock;
    }

    if (pwmdata[id].dma_chan) {
        printk(KERN_ERR "PWM: pwm ch %d is busy!\n", id);
        ret = -1;
        goto unlock;
    }

    pwmdata[id].rate = clk_src_rate;
    pwm_set_clk_div(id);

    /* set pwm idle and init level */
    pwmdata[id].init_level = !!dma_config->start_level;
    pwmdata[id].idle_level = !!dma_config->idle_level;
    set_pwm_init_level(id, pwmdata[id].init_level);
    set_pwm_idle_level(id, pwmdata[id].idle_level);

    /* updata dma mode */
    set_pwm_mode(id, 1);

    /* set dma request trigger number */
    pwm_write_reg(PWM_DRTN(id), PWM_DMA_TRIGGER_NUM);

    dma_cap_zero(mask);
    dma_cap_set(DMA_SLAVE, mask);
    dma_cap_set(DMA_CYCLIC, mask);
    txchan = dma_request_channel(mask, NULL, NULL);
    if (!txchan) {
        printk(KERN_ERR "PWM%d request dma tx channel failed\n", id);
        ret = -1;
        goto unlock;
    }
    
    tx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    tx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    tx_config.dst_maxburst = 4;
    tx_config.src_maxburst = 4;
    tx_config.dst_addr = (dma_addr_t)(PWM_IOBASE + PWM_DR(id));
    tx_config.slave_id = INGENIC_DMA_REQ_ASOC_PWM + id;
    tx_config.direction = DMA_MEM_TO_DEV;
    dmaengine_slave_config(txchan, &tx_config);

    pwmdata[id].dma_chan = txchan;
    pwmdata[id].is_config = PWM_CONFIG_DMA_MODE;
    ret = pwmdata[id].rate;

unlock:
    mutex_unlock(&pwmdata[id].lock);

    return ret;
}
EXPORT_SYMBOL_GPL(pwm2_dma_init);

int pwm2_dma_update(int id, struct pwm_dma_data *dma_data)
{
    int ret = 0;
    struct dma_async_tx_descriptor *txdesc;

    if (id < 0 || id >= PWM_NUMS) {
        printk(KERN_ERR "PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    if (!dma_data || !dma_data->data || !dma_data->data_count) {
        printk(KERN_ERR "PWM: pwm ch%d not have dma data!\n", id);
        return -1;
    }

#ifdef MD_AD100_PWM_CHECK_DMA_DATA
    for (int i = 0; i < dma_data->data_count; i++) {
        if (!dma_data->data[i].high || !dma_data->data[i].low) {
            printk(KERN_ERR "PWM: pwm ch%d dma mode, data cannot be zero\n", id);
            return -1;
        }
    }
#endif

    mutex_lock(&pwmdata[id].lock);

    if (pwmdata[id].is_config != PWM_CONFIG_DMA_MODE) {
        printk(KERN_ERR "PWM: pwm ch%d is not config dma mode!\n", id);
        ret = -1;
        goto unlock;
    }

    if (pwmdata[id].dma_loop) {
        printk(KERN_ERR "PWM: pwm ch%d work in dma loop mode\n", id);
        ret = -1;
        goto unlock;
    }

    assert(!pwmdata[id].dma_data);
    pwmdata[id].dma_data_len = dma_data->data_count * sizeof(struct pwm_data);
    pwmdata[id].dma_data = dma_alloc_coherent(platform_dev, pwmdata[id].dma_data_len, &pwmdata[id].dma_handle, GFP_KERNEL);
    if (!pwmdata[id].dma_data) {
        printk(KERN_ERR "PWM: pwm ch%d dma_alloc_coherent fail\n", id);
        ret = -1;
        goto unlock;
    }

    memcpy(pwmdata[id].dma_data, dma_data->data, pwmdata[id].dma_data_len);

    if (dma_data->dma_loop) {
        txdesc = dmaengine_prep_dma_cyclic(pwmdata[id].dma_chan,
                pwmdata[id].dma_handle,
                pwmdata[id].dma_data_len,
                pwmdata[id].dma_data_len,
                DMA_MEM_TO_DEV,
                DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
    } else {
        txdesc = dmaengine_prep_slave_single(pwmdata[id].dma_chan,
                pwmdata[id].dma_handle,
                pwmdata[id].dma_data_len,
                DMA_MEM_TO_DEV,
                DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
    }

    if (!txdesc) {
        printk(KERN_ERR "PWM request dma desc failed!\n");
        dma_free_coherent(platform_dev, pwmdata[id].dma_data_len, pwmdata[id].dma_data, pwmdata[id].dma_handle);
        pwmdata[id].dma_data = NULL;
        ret = -1;
        goto unlock;
    }

    /* update dma loop mode */
    pwmdata[id].dma_loop = !!dma_data->dma_loop;
    txdesc->callback = dma_tx_callback;
    txdesc->callback_param = (void *)id;

    dmaengine_submit(txdesc);
    pwmdata[id].dma_complete = 0;

    /* enable dma request */
    set_pwm_dma_req(id, 1);
    pwm_enable(id);
    dma_async_issue_pending(pwmdata[id].dma_chan);
    /* normal mode wait dma complete */
    if (!dma_data->dma_loop) {
        /* wait dma_end */
        wait_event(pwmdata[id].dma_wait, pwmdata[id].dma_complete);

        /* disable dma request */
        set_pwm_dma_req(id, 0);

        dmaengine_terminate_all(pwmdata[id].dma_chan);

        dma_free_coherent(platform_dev, pwmdata[id].dma_data_len, pwmdata[id].dma_data, pwmdata[id].dma_handle);
        pwmdata[id].dma_data = NULL;
    }

unlock:
    mutex_unlock(&pwmdata[id].lock);

    return ret;
}
EXPORT_SYMBOL_GPL(pwm2_dma_update);

int pwm2_dma_disable_loop(int id)
{
    int ret = 0;

    if (id < 0 || id >= PWM_NUMS) {
        printk(KERN_ERR "PWM: No support pwm ch %d !\n", id);
        return -1;
    }

    mutex_lock(&pwmdata[id].lock);

    if (pwmdata[id].is_config != PWM_CONFIG_DMA_MODE) {
        printk(KERN_ERR "PWM: pwm ch%d is not config dma mode!\n", id);
        ret = -1;
        goto unlock;
    }

    if (!pwmdata[id].is_enable) {
        printk(KERN_ERR "PWM: pwm ch%d dma is not enable\n", id);
        ret = -1;
        goto unlock;
    }

    if (!pwmdata[id].dma_loop) {
        printk(KERN_ERR "PWM: pwm ch%d dma is not loop\n", id);
        ret = -1;
        goto unlock;
    }

    /* disable dma request */
    set_pwm_dma_req(id, 0);

    /* disable dma */
    dmaengine_terminate_all(pwmdata[id].dma_chan);

    dma_free_coherent(platform_dev, pwmdata[id].dma_data_len, pwmdata[id].dma_data, pwmdata[id].dma_handle);

    pwmdata[id].dma_data = NULL;
    pwmdata[id].dma_loop = 0;

unlock:
    mutex_unlock(&pwmdata[id].lock);
    return ret;
}
EXPORT_SYMBOL_GPL(pwm2_dma_disable_loop);


static int pwm_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int pwm_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static long pwm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd) {
        case PWM_NOT_REALLY_ENABLE: {
            unsigned int id;
            if (copy_from_user(&id, (unsigned int *)arg, sizeof(unsigned int))) {
                printk(KERN_ERR "PWM: copy_from_user err!\n");
                return -1;
            }
            pwm_set_not_really_enable(id, 1);
            return 0;
        }

        case PWM_NOT_REALLY_DISABLE: {
            int id;
            if (copy_from_user(&id, (unsigned int *)arg, sizeof(int))) {
                printk(KERN_ERR "PWM: copy_from_user err!\n");
                return -1;
            }
            pwm_set_not_really_disable(id, 1);
            return 0;
        }

        case PWM_ENABLE_CHANNELS: {
            unsigned int channels = 0;
            if (copy_from_user(&channels, (unsigned int *)arg, sizeof(unsigned int))) {
                printk(KERN_ERR "PWM: copy_from_user err!\n");
                return -1;
            }
            pwm_enable_channels(channels);
            return 0;
        }

        case PWM_DISABLE_CHANNELS: {
            unsigned int channels = 0;
            if (copy_from_user(&channels, (unsigned int *)arg, sizeof(unsigned int))) {
                printk(KERN_ERR "PWM: copy_from_user err!\n");
                return -1;
            }
            pwm_disable_channels(channels);
            return 0;
        }

        case PWM_REQUEST: {
            char gpio_name[12] = {0};
            if (copy_from_user(gpio_name, (char *)arg, 12)) {
                printk(KERN_ERR "PWM: copy_from_user err!\n");
                return -1;
            }

            int gpio = str_to_gpio(gpio_name);
            if (gpio < 0) {
                printk(KERN_ERR "PWM: gpio is invalid:%s !\n", gpio_name);
                return -1;
            }

            struct jz_pwm_gpio *gpio_def = pwm_get_gpio_func_def(gpio);
            if (!gpio_def) {
                printk(KERN_ERR "PWM: gpio is not pwm:%s !\n", gpio_name);
                return -1;
            }

            ret = pwm2_request(gpio_def->gpio, gpio_def->name);

            return ret;
        }

        case PWM_RELEASE: {
            ret = pwm2_release(arg);
            return ret;
        }

        case PWM_CONFIG: {
            struct pwm_config_data config;
            if (copy_from_user(&config, (void *)arg, sizeof(config))) {
                printk(KERN_ERR "PWM: copy_from_user err!\n");
                return -1;
            }

            ret = pwm2_config(config.id, &config);
            return ret;
        }

        case PWM_SET_LEVEL: {
            unsigned int tmp[2];
            if (copy_from_user(tmp, (void *)arg, sizeof(tmp))) {
                printk(KERN_ERR "PWM: copy_from_user err!\n");
                return -1;
            }

            ret = pwm2_set_level(tmp[0], tmp[1]);
            return ret;
        }

        case PWM_DMA_INIT: {
            struct pwm_dma_config dma_config;
            if (copy_from_user(&dma_config, (void *)arg, sizeof(dma_config))) {
                printk(KERN_ERR "PWM: copy_from_user err!\n");
                return -1;
            }

            ret = pwm2_dma_init(dma_config.id, &dma_config);
            return ret;
        }

        case PWM_DMA_UPDATE: {
            void *data;
            struct pwm_dma_data dma_data;
            if (copy_from_user(&dma_data, (void *)arg, sizeof(dma_data))) {
                printk(KERN_ERR "PWM: copy_from_user err!\n");
                return -1;
            }

            data = kmalloc(dma_data.data_count * sizeof(struct pwm_data), GFP_KERNEL);
            if (!data) {
                printk(KERN_ERR "PWM: pwm ch%d malloc dma data err\n", dma_data.id);
                return -1;
            }

            if (copy_from_user(data, (void *)dma_data.data, dma_data.data_count * sizeof(struct pwm_data))) {
                printk(KERN_ERR "PWM: copy_from_user err!\n");
                return -1;
            }

            dma_data.data = data;
            ret = pwm2_dma_update(dma_data.id, &dma_data);
            kfree(data);
            return ret;
        }

        case PWM_DMA_DISABLE_LOOP: {
            ret = pwm2_dma_disable_loop(arg);
            return ret;
        }

        default:
            return -1;
    }

    return ret;
}

static struct file_operations pwm_fops = {
    .owner      = THIS_MODULE,
    .open       = pwm_open,
    .release    = pwm_close,
    .unlocked_ioctl = pwm_ioctl,
};

struct miscdevice pwm_mdevice = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "jz_pwm",
    .fops = &pwm_fops,
};

static int ingenic_pwm_probe(struct platform_device *pdev)
{
    int ret;
    int i;

    platform_dev = &pdev->dev;

    pwm_div_clk = clk_get(NULL, "div_pwm");
    clk_src_rate = clk_get_rate(pwm_div_clk);
    BUG_ON(IS_ERR(pwm_div_clk));
    clk_set_rate(pwm_div_clk, clk_src_rate);
    clk_prepare_enable(pwm_div_clk);

    pwm_gate_clk = clk_get(NULL, "gate_pwm");
    BUG_ON(IS_ERR(pwm_gate_clk));
    clk_prepare_enable(pwm_gate_clk);

    for (i = 0; i < PWM_NUMS; i++) {
        mutex_init(&pwmdata[i].lock);
        init_waitqueue_head(&pwmdata[i].dma_wait);
    }

    ret = misc_register(&pwm_mdevice);
    BUG_ON(ret < 0);

    return 0;
}

static int ingenic_pwm_remove(struct platform_device *pdev)
{
    misc_deregister(&pwm_mdevice);

    clk_disable_unprepare(pwm_gate_clk);
    clk_disable_unprepare(pwm_div_clk);
    clk_disable_unprepare(pwm_mux_clk);

    clk_put(pwm_gate_clk);
    clk_put(pwm_div_clk);
    clk_put(pwm_mux_clk);

    return 0;
}

static struct platform_driver ingenic_pwm_driver = {
    .probe = ingenic_pwm_probe,
    .remove = ingenic_pwm_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ingenic-pwm",
    },
};

/* stop no dev release warning */
static void jz_pwm_dev_release(struct device *dev){}

struct platform_device ingenic_pwm_device = {
    .name = "ingenic-pwm",
    .dev = {
        .release = jz_pwm_dev_release,
    },
};

static int __init jz_pwm_init(void)
{
    int ret = platform_device_register(&ingenic_pwm_device);
    if (ret)
        return ret;
    
    return platform_driver_register(&ingenic_pwm_driver);
}
module_init(jz_pwm_init);

static void __exit jz_pwm_exit(void)
{
    platform_device_unregister(&ingenic_pwm_device);

    platform_driver_unregister(&ingenic_pwm_driver);
}
module_exit(jz_pwm_exit);

MODULE_DESCRIPTION("Ingenic SoC PWM driver");
MODULE_LICENSE("GPL");