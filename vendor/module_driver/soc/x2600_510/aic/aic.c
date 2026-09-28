#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <common.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <assert.h>
#include <utils/gpio.h>

#include <ingenic_asla_sound_card.h>

#include "aic_regs.h"

#define gpio_i2s_mclk   GPIO_PC(16)
#define gpio_i2s_bclk   GPIO_PC(15)
#define gpio_i2s_lrck   GPIO_PC(17)
#define gpio_i2s_dout   GPIO_PC(18)
#define gpio_i2s_din    GPIO_PC(19)

struct dma_param {
    int dma_type;
    struct dma_chan *dma_chan;
    dma_addr_t dma_addr;

    char *buf;
    int buf_len;
    int period_size;
    int buswidth;
    int maxburst;
};

struct aic_data {
    unsigned int dma_pos;
    unsigned int rw_pos;
    unsigned int read_pos;
    unsigned int data_size;
    unsigned int period_time;
    unsigned int unit_size;
    unsigned int period_size;
    unsigned int frame_size;
    unsigned int channels;
    unsigned int format;

    void *dma_buffer;
    void *real_buffer;
    unsigned int dma_buffer_size;
    unsigned int buffer_size;

    unsigned int is_running;
    unsigned int expect_state;

    struct dma_param dma;

    struct mutex mutex;
    struct task_struct *thread;
    unsigned int thread_stop;
    unsigned int thread_is_stop;

    struct work_struct work;
    struct work_struct start_work;
    struct work_struct stop_work;

    struct snd_pcm_substream *substream;
};

struct aic_cfg {
    int is_use;
    int as_master;
    int interface;

    int clk_id;
    int clk_dir;
    unsigned int set_clk_freq;
};

struct aic_dev {
    struct device *dev;
    struct platform_device *pdev;

    struct clk *div_clk;

    struct aic_data playback;
    struct aic_data capture;

    struct aic_cfg cfg[2];

    int as_master;
    int interface;

    int sample_rate;

    int clk_id;
    unsigned int clk_div;

    unsigned int is_init;
    unsigned int is_enabled;

    struct mutex mutex;
    struct workqueue_struct *workqueue;
};

struct aic_dev aic_dev;

static int gpio_status = 0;

#define PCM_INTERFACE_I2S_MSB 1
#define PCM_INTERFACE_I2S 0

#define PCM_ON  1
#define PCM_OFF 0

#define PLAYBACK_INIT   BIT(0)
#define CAPTURE_INIT    BIT(1)

#include "aic_hal.c"
#include "aic_dma.c"

#define BUFFER_ALIGN 32
#define PERIOD_BYTES_MIN 1024
#define PERIODS_MIN 1
#define PERIODS_MAX 128
#define MAX_DMA_BUFFERSIZE (128*1024)

#define ASOC_AIC_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
#define ASOC_AIC_RATE (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | \
        SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000)

static const struct snd_pcm_hardware aic_pcm_hardware = {
    .info = SNDRV_PCM_INFO_MMAP |
        SNDRV_PCM_INFO_PAUSE |
        SNDRV_PCM_INFO_RESUME |
        SNDRV_PCM_INFO_MMAP_VALID |
        SNDRV_PCM_INFO_INTERLEAVED |
        SNDRV_PCM_INFO_BLOCK_TRANSFER,
    .formats = ASOC_AIC_FORMATS,
    .rates = ASOC_AIC_RATE,
    .channels_min = 1,
    .channels_max = 2,
    .buffer_bytes_max = MAX_DMA_BUFFERSIZE,
    .period_bytes_min = PERIOD_BYTES_MIN,
    .period_bytes_max = MAX_DMA_BUFFERSIZE,
    .periods_min = PERIODS_MIN,
    .periods_max = PERIODS_MAX,
    .fifo_size = 0,
};

static int m_gpio_request(int gpio, char *name)
{
    char buf[20];
    int index;

    index = gpio % 32;

    if (gpio_status & (1 << index))
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        printk(KERN_ERR "AIC: failed to request %s gpio: %s\n", name, gpio_to_str(gpio, buf));
        return -EINVAL;
    }

    gpio_set_func(gpio, GPIO_FUNC_1);

    gpio_status |= (1 << index);

    return 0;
}

static void m_gpio_free(int gpio)
{
    int index;

    if (gpio < 0)
        return;

    index = gpio % 32;

    if (!(gpio_status & (1 << index)))//为0就表示这个gpio未申请或者已经被释放
        return;

    gpio_set_func(gpio, GPIO_OUTPUT1);
    gpio_free(gpio);

    gpio_status &= ~(1 << index);
}

static int aic_ensure_dac_gpio_request(void)
{
    int ret = 0;

    ret = m_gpio_request(gpio_i2s_mclk, "i2s-mclk");
    if (ret < 0)
        return -EINVAL;

    ret = m_gpio_request(gpio_i2s_bclk, "i2s-bclk");
    if (ret < 0) {
        ret = -EINVAL;
        goto request_tx_bclk_gpio_err;
    }

    ret = m_gpio_request(gpio_i2s_lrck, "i2s-lrck");
    if (ret < 0) {
        ret = -EINVAL;
        goto request_tx_lr_clk_gpio_err;
    }

    ret = m_gpio_request(gpio_i2s_dout, "i2s-dout");
    if (ret < 0) {
        ret = -EINVAL;
        goto request_data_out_gpio_err;
    }

    return 0;

request_data_out_gpio_err:
    m_gpio_free(gpio_i2s_lrck);
request_tx_lr_clk_gpio_err:
    m_gpio_free(gpio_i2s_bclk);
request_tx_bclk_gpio_err:
    m_gpio_free(gpio_i2s_mclk);

    return ret;
}

static int aic_ensure_adc_gpio_request(void)
{
    int ret = 0;

    ret = m_gpio_request(gpio_i2s_mclk, "i2s-mclk");
    if (ret < 0)
        return -EINVAL;

    ret = m_gpio_request(gpio_i2s_bclk, "i2s-bclk");
    if (ret < 0) {
        ret = -EINVAL;
        goto request_bclk_gpio_err;
    }

    ret = m_gpio_request(gpio_i2s_lrck, "i2s-lrck");
    if (ret < 0) {
        ret = -EINVAL;
        goto request_lr_clk_gpio_err;
    }

    ret = m_gpio_request(gpio_i2s_din, "i2s-din");
    if (ret < 0) {
        ret = -EINVAL;
        goto request_data_gpio_err;
    }

    return 0;

request_data_gpio_err:
    m_gpio_free(gpio_i2s_lrck);
request_lr_clk_gpio_err:
    m_gpio_free(gpio_i2s_bclk);
request_bclk_gpio_err:
    m_gpio_free(gpio_i2s_mclk);

    return ret;
}

static void aic_gpio_release(void)
{
    m_gpio_free(gpio_i2s_mclk);
    m_gpio_free(gpio_i2s_bclk);
    m_gpio_free(gpio_i2s_lrck);
    m_gpio_free(gpio_i2s_dout);
    m_gpio_free(gpio_i2s_din);

    gpio_status = 0;
}

static inline void *m_dma_alloc_coherent(int size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(aic_dev.dev, size, &dma_handle, GFP_KERNEL);
    assert(mem);

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(int size)
{
    void *dma_buffer = aic_dev.capture.dma_buffer;
    if (dma_buffer) {
        dma_addr_t dma_handle = virt_to_phys(dma_buffer);
        dma_free_coherent(aic_dev.dev, size, (void *)CKSEG1ADDR(dma_buffer), dma_handle);
        aic_dev.capture.dma_buffer = NULL;
    }
}

static unsigned int aic_get_writed_pos(void)
{
    int old_dma_pos = aic_dev.playback.dma_pos;
    struct dma_param *dma = &aic_dev.playback.dma;
    unsigned int buffer_size = aic_dev.playback.buffer_size;
    int buffer_addr = virt_to_phys(aic_dev.playback.dma_buffer);

    dma_addr_t cur_addr = get_dma_addr(dma->dma_chan, buffer_addr, buffer_size, DMA_MEM_TO_DEV);
    unsigned int pos = cur_addr - buffer_addr;

    if (pos == old_dma_pos || cur_addr == 0)
        return 0;

    aic_dev.playback.dma_pos = pos;

    return pos;
}

static unsigned int aic_get_readable_size(void)
{
    int ret = 0;
    struct dma_param *dma = &aic_dev.capture.dma;
    struct dma_chan *dma_chan = dma->dma_chan;
    unsigned int buffer_size = aic_dev.capture.dma_buffer_size;
    int buffer_addr = virt_to_phys(aic_dev.capture.dma_buffer);

    dma_addr_t cur_addr = get_dma_addr(dma_chan, buffer_addr, buffer_size, DMA_DEV_TO_MEM);
    unsigned int pos = cur_addr - buffer_addr;
    unsigned int size = sub_pos(buffer_size, pos, aic_dev.capture.dma_pos);
    unsigned int unit_size = aic_dev.capture.unit_size;

    aic_dev.capture.dma_pos = pos;
    aic_dev.capture.data_size += size;

    if (aic_dev.capture.data_size > (buffer_size - unit_size)) {
        unsigned int align_pos = ALIGN(pos, unit_size);
        aic_dev.capture.rw_pos = add_pos(buffer_size, align_pos, unit_size);
        aic_dev.capture.data_size = buffer_size - sub_pos(buffer_size, aic_dev.capture.rw_pos, pos);
    }

    ret = aic_dev.capture.data_size > unit_size ? aic_dev.capture.data_size - unit_size : 0;

    return ret;
}

static void do_memcpy(void *dst, void *src, int bytes)
{
    dma_cache_inv((unsigned long)src, bytes);
    memcpy(dst, src, bytes);
}

static int aic_do_read_buffer(void *mem, unsigned int bytes)
{
    unsigned int buffer_size = aic_dev.capture.dma_buffer_size;
    unsigned int pos = aic_dev.capture.rw_pos;
    void *src = aic_dev.capture.dma_buffer;

    if (pos + bytes <= buffer_size) {
        do_memcpy(mem, src + pos, bytes);
    } else {
        unsigned int size1 = buffer_size - pos;
        do_memcpy(mem, src + pos, size1);
        do_memcpy(mem + size1, src, bytes - size1);
    }

    dma_cache_wback((unsigned long)mem, bytes);

    aic_dev.capture.rw_pos = add_pos(buffer_size, pos, bytes);

    aic_dev.capture.data_size -= bytes;

    return 0;
}

static int aic_read_bytes(struct aic_data *data, void *mem, int bytes)
{
    unsigned int len = 0;

    while (bytes) {
        unsigned int n = aic_get_readable_size();
        if (!n)
            break;

        if (n > bytes)
            n = bytes;

        aic_do_read_buffer(mem, n);
        len += n;
        mem += n;
        bytes -= n;
    }

    return len;
}

static int capture_dma_copy_thread(void *data_)
{
    int n = 0;
    int len = 0;
    struct aic_data *data = data_;
    int buffer_size = data->buffer_size;
    unsigned int period_time_us = data->period_time / 1000;

    data->read_pos = 0;
    data->thread_is_stop = 0;

    while (1) {
        if (data->thread_stop)
            break;

        mutex_lock(&data->mutex);

        len = buffer_size - data->read_pos;
        if (len > 512)
            len = 512;

        n = aic_read_bytes(data, data->real_buffer + data->read_pos, len);
        data->read_pos += n;
        if (data->read_pos >= buffer_size)
            data->read_pos = 0;

        if (n)
            snd_pcm_period_elapsed(data->substream);

        mutex_unlock(&data->mutex);

        if (n != len)
            usleep_range(period_time_us, period_time_us);
    }
    data->thread_is_stop = 1;

    return 0;
}

static void aic_write_bytes(void)
{
    void *dma_buf = aic_dev.playback.dma_buffer;
    int dma_buffer_size = aic_dev.playback.dma_buffer_size;
    int old_dma_pos = aic_dev.playback.dma_pos;

    int pos = aic_get_writed_pos();
    if (pos) {
        if (pos > old_dma_pos) {
            memset(dma_buf + old_dma_pos, 0, pos - old_dma_pos);
            dma_cache_wback((unsigned long)(dma_buf + old_dma_pos), pos - old_dma_pos);
        } else {
            int size = dma_buffer_size - old_dma_pos;
            memset(dma_buf + old_dma_pos, 0, size);
            dma_cache_wback((unsigned long)(dma_buf + old_dma_pos), size);
            memset(dma_buf, 0, pos);
            dma_cache_wback((unsigned long)dma_buf, pos);
        }
        snd_pcm_period_elapsed(aic_dev.playback.substream);
    }
}

static int playback_dma_copy_thread(void *data_)
{
    struct aic_data *data = data_;
    unsigned int period_time_us = data->period_time / 1000;

    data->thread_is_stop = 0;

    while (1) {
        if (data->thread_stop)
            break;

        mutex_lock(&data->mutex);
        aic_write_bytes();
        mutex_unlock(&data->mutex);

        usleep_range(period_time_us, period_time_us);
    }

    data->thread_is_stop = 1;

    return 0;
}

static void aic_start_playback(void)
{
    mutex_lock(&aic_dev.mutex);
    aic_hal_start_playback();
    mutex_unlock(&aic_dev.mutex);
}

static void aic_start_capture(void)
{
    mutex_lock(&aic_dev.mutex);
    aic_hal_start_capture();
    mutex_unlock(&aic_dev.mutex);
}

static void aic_reset_dma_pos(void)
{
    aic_dev.playback.dma_pos = 0;
    snd_pcm_period_elapsed(aic_dev.playback.substream);
}

static void aic_stop_playback(void)
{
    mutex_lock(&aic_dev.mutex);
    aic_hal_stop_playback();
    aic_reset_dma_pos();
    mutex_unlock(&aic_dev.mutex);
}

static void aic_stop_capture(void)
{
    mutex_lock(&aic_dev.mutex);
    aic_hal_stop_capture();
    mutex_unlock(&aic_dev.mutex);
}

static void aic_init_common_setting(struct aic_data *data)
{
    if (aic_dev.is_init == 0)
        aic_hal_init_common_setting(data);

    if (data->substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        aic_dev.is_init |= PLAYBACK_INIT;
    else
        aic_dev.is_init |= CAPTURE_INIT;
}

static int aic_set_clk_rate(struct aic_data *data, int is_playback, int clk_freq)
{
    int div = 0;
    unsigned int freq;

    int sample_rate = aic_dev.sample_rate;
    int set_clk_freq = clk_freq;

    if (aic_dev.clk_id == SELECT_INNER_CODEC) {
        freq = 24576000;
        div = freq / sample_rate;
        if (set_clk_freq != 0 && freq != set_clk_freq) {
            printk(KERN_ERR "AIC: no support clk freq %d\n", set_clk_freq);
            return -EINVAL;
        }
    } else {
        if (set_clk_freq == 0) {
            if (sample_rate <= 16000)
                div = 768;
            else if (sample_rate <= 24000)
                div = 512;
            else if (sample_rate <= 32000)
                div = 384;
            else if (sample_rate <= 48000)
                div = 256;
            else
                div = 256;
            freq = sample_rate * div;
        } else {
            freq = set_clk_freq;
            div = freq / sample_rate;
        }
    }

    aic_dev.clk_div = div;

    clk_set_rate(aic_dev.div_clk, freq);
    clk_prepare_enable(aic_dev.div_clk);

    return 0;
}

static void start_work(struct work_struct *work)
{
    struct aic_data *data = container_of(work, struct aic_data, start_work);

    if (data->expect_state == PCM_OFF || data->is_running == 1)
        return;

    mutex_lock(&data->mutex);
    data->is_running = 1;

    if (data->substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        aic_start_playback();
        aic_dma_submit_cyclic(&aic_dev.playback, DMA_MEM_TO_DEV);
        aic_dev.playback.thread_stop = 0;
        aic_dev.playback.thread = kthread_create(playback_dma_copy_thread, &aic_dev.playback, "playback_dma_copy");
        if (!IS_ERR_OR_NULL(aic_dev.playback.thread))
            wake_up_process(aic_dev.playback.thread);
    } else {
        aic_start_capture();
        aic_dma_submit_cyclic(&aic_dev.capture, DMA_DEV_TO_MEM);
        aic_dev.capture.thread_stop = 0;
        aic_dev.capture.thread = kthread_create(capture_dma_copy_thread, &aic_dev.capture, "capture_dma_copy");
        if (!IS_ERR_OR_NULL(aic_dev.capture.thread))
            wake_up_process(aic_dev.capture.thread);
    }
    mutex_unlock(&data->mutex);
}

static void stop_work(struct work_struct *work)
{
    struct aic_data *data = container_of(work, struct aic_data, stop_work);

    if (data->expect_state == PCM_ON || data->is_running == 0)
        return;

    mutex_lock(&data->mutex);
    if (data->substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        data->thread_stop = 1;
        while (!aic_dev.playback.thread_is_stop)
            usleep_range(300, 300);
        aic_dev.playback.thread = NULL;

        aic_dma_terminate(&aic_dev.playback, DMA_MEM_TO_DEV);
        aic_stop_playback();
    } else {
        data->thread_stop = 1;
        while (!aic_dev.capture.thread_is_stop)
            usleep_range(300, 300);
        aic_dev.capture.thread = NULL;

        aic_dma_terminate(&aic_dev.capture, DMA_DEV_TO_MEM);
        aic_stop_capture();
    }

    data->is_running = 0;
    mutex_unlock(&data->mutex);
}

static void aic_pcm_start(struct aic_data *data)
{
    data->expect_state = PCM_ON;
    queue_work(aic_dev.workqueue, &data->start_work);
}

static void aic_pcm_stop(struct aic_data *data)
{
    data->expect_state = PCM_OFF;
    queue_work(aic_dev.workqueue, &data->stop_work);
}

static int aic_trigger(struct snd_pcm_substream *substream, int cmd,
        struct snd_soc_dai *dai)
{
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            aic_pcm_start(&aic_dev.playback);
        else
            aic_pcm_start(&aic_dev.capture);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            aic_pcm_stop(&aic_dev.playback);
        else
            aic_pcm_stop(&aic_dev.capture);
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static int aic_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
    struct aic_data *data;
    int sample_rate;
    int ret = 0;

    mutex_lock(&aic_dev.mutex);

    int id = dai->id;

    if (aic_dev.is_init && aic_dev.cfg[!id].is_use) {
        printk(KERN_ERR "AIC: failed, only support ecodec or icodec at the same time.\n");
        ret = -EBUSY;
        goto unlock_mutex;
    }

    int is_playback = substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? 1 : 0;

    if (is_playback) {
        if (aic_dev.is_init & PLAYBACK_INIT) {
            printk(KERN_ERR "AIC: playback is already initialized.\n");
            ret = -EBUSY;
            goto unlock_mutex;
        }
        data = &aic_dev.playback;
    } else {
        if (aic_dev.is_init & CAPTURE_INIT) {
            printk(KERN_ERR "AIC: caputer is already initialized.\n");
            ret = -EBUSY;
            goto unlock_mutex;
        }
        data = &aic_dev.capture;
    }

    sample_rate = params_rate(hw_params);
    if (aic_dev.is_init) {
        if (sample_rate != aic_dev.sample_rate) {
            printk(KERN_ERR "AIC: capture and playback sample rate %d are different!\n", aic_dev.sample_rate);
            ret = -EINVAL;
            goto unlock_mutex;
        }
    }

    data->channels = params_channels(hw_params);
    data->format = params_format(hw_params);
    data->substream = substream;
    aic_dev.sample_rate = sample_rate;

    aic_dev.cfg[id].is_use++;
    aic_dev.as_master = aic_dev.cfg[id].as_master;
    aic_dev.interface = aic_dev.cfg[id].interface;
    aic_dev.clk_id = aic_dev.cfg[id].clk_id;

    ret = aic_set_clk_rate(data, is_playback, aic_dev.cfg[id].set_clk_freq);
    if (ret < 0)
        goto unlock_mutex;

    aic_init_common_setting(data);
    if (is_playback) {
        if (aic_dev.clk_id == SELECT_EXT_CODEC) {
            ret = aic_ensure_dac_gpio_request();
            if (ret < 0)
                goto unlock_mutex;
        }
        aic_init_playback_setting(data);
    } else {
        if (aic_dev.clk_id == SELECT_EXT_CODEC) {
            ret = aic_ensure_adc_gpio_request();
            if (ret < 0)
                goto unlock_mutex;
        }
        aic_init_capture_setting(data);
    }

unlock_mutex:
    mutex_unlock(&aic_dev.mutex);

    return ret;
}

static int aic_free(struct snd_pcm_substream *substream,
            struct snd_soc_dai *dai)
{
    mutex_lock(&aic_dev.mutex);

    if (!aic_dev.cfg[dai->id].is_use)
        goto unlock_mutex;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        aic_dev.is_init &= ~PLAYBACK_INIT;
    else
        aic_dev.is_init &= ~CAPTURE_INIT;

    aic_dev.cfg[dai->id].is_use--;

    clk_disable_unprepare(aic_dev.div_clk);

    aic_gpio_release();

unlock_mutex:
    mutex_unlock(&aic_dev.mutex);

    return 0;
}

static int aic_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    unsigned int interface = fmt & SND_SOC_DAIFMT_FORMAT_MASK;
    unsigned int clk_dir = fmt & SND_SOC_DAIFMT_MASTER_MASK;

    switch (interface) {
    case SND_SOC_DAIFMT_I2S:
        aic_dev.cfg[dai->id].interface = PCM_INTERFACE_I2S;
        break;
    case SND_SOC_DAIFMT_MSB:
        aic_dev.cfg[dai->id].interface = PCM_INTERFACE_I2S_MSB;
        break;
    default:
        printk(KERN_ERR "AIC: fmt error: %x", interface);
        return -EINVAL;
    }

    switch (clk_dir) {
    case SND_SOC_DAIFMT_CBM_CFM:
        aic_dev.cfg[dai->id].as_master = 0; /* codec as master */
        break;
    case SND_SOC_DAIFMT_CBS_CFS:
        aic_dev.cfg[dai->id].as_master = 1; /* codec as slave */
        break;
    default:
        printk(KERN_ERR "AIC: clk dir error: %x", clk_dir);
        return -EINVAL;
    }

    return 0;
}

static int aic_set_sysclk(struct snd_soc_dai *dai, int clk_id,
        unsigned int freq, int dir)
{
    int id = dai->id;

    aic_dev.cfg[id].clk_id = clk_id;

    aic_dev.cfg[id].set_clk_freq = freq;

    aic_dev.cfg[id].clk_dir = dir;

    return 0;
}

static struct snd_soc_dai_ops aic_dai_ops = {
    .trigger    = aic_trigger,
    .hw_params  = aic_hw_params,
    .hw_free    = aic_free,
    .set_fmt    = aic_set_dai_fmt,
    .set_sysclk = aic_set_sysclk,
};

static int aic_dai_probe(struct snd_soc_dai *dai)
{
    return 0;
}

static int aic_dai_remove(struct snd_soc_dai *dai)
{
    return 0;
}

static int aic_dma_pcm_open(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;

    int ret = snd_soc_set_runtime_hwparams(substream, &aic_pcm_hardware);
    if (ret) {
        printk(KERN_ERR "AIC: snd_soc_set_runtime_hwparams failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, BUFFER_ALIGN);
    if (ret) {
        printk(KERN_ERR "AIC: align hw_param buffer failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, BUFFER_ALIGN);
    if (ret) {
        printk(KERN_ERR "AIC: align hw_param period failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
    if (ret < 0) {
        printk(KERN_ERR "AIC: snd_pcm_hw_constraint_integer failed ret = %d\n", ret);
        return ret;
    }

    return 0;
}

static int aic_dma_pcm_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
    return 0;
}

static int aic_dma_pcm_prepare(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
    return 0;
}

static int aic_dma_playback_mmap(struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
    unsigned long start;
    unsigned long off;
    void *dma_buffer;

    off = vma->vm_pgoff << PAGE_SHIFT;

    dma_buffer = aic_dev.playback.dma_buffer;

    start = virt_to_phys(dma_buffer);
    start &= PAGE_MASK;
    off += start;

    vma->vm_pgoff = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    /* 0: cachable,write through (cache + cacheline对齐写穿)
    * 1: uncachable,write Acceleration (uncache + 硬件写加速)
    * 2: uncachable
    * 3: cachable
    */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= 0;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                        vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    return 0;
}

static int aic_dma_pcm_mmap(struct snd_soc_component *component, struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        return aic_dma_playback_mmap(substream, vma);
    else
        return snd_pcm_lib_default_mmap(substream, vma);
}

static int aic_dma_pcm_hw_params(struct snd_soc_component *component, struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *hw_params)
{
    struct aic_data *data;
    int ret, channels, period_ms, buf_size, unit_size, format;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        data = &aic_dev.playback;
    else
        data = &aic_dev.capture;

    buf_size = params_buffer_bytes(hw_params);
    data->period_size = params_period_size(hw_params);
    channels = data->channels;

    format = data->format;
    if (format == SNDRV_PCM_FORMAT_S24_LE) {
        data->dma.buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
        data->dma.maxburst = 32;
    } else if (format == SNDRV_PCM_FORMAT_S16_LE) {
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
            if (channels == 1) {
                data->dma.buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
                data->dma.maxburst = 16;
            } else {
                data->dma.buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
                data->dma.maxburst = 32;
            }
        } else {
            data->dma.buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
            data->dma.maxburst = 16;
        }
    }

    unit_size = data->dma.maxburst;

    if (buf_size % unit_size) {
        printk(KERN_ERR "AIC: ERROR buf_size UNALIGN %d.\n", buf_size);
        return -EINVAL;
    }

    data->unit_size = unit_size;
    data->buffer_size = buf_size;
    data->dma_buffer_size = buf_size;
    data->frame_size = snd_pcm_format_physical_width(params_format(hw_params)) * channels / 8;/* 帧＝采样位数*通道 */

    substream->dma_buffer.dev.type = SNDRV_DMA_TYPE_DEV;
    ret = snd_pcm_lib_malloc_pages(substream, buf_size);
    if (ret < 0)
        return ret;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        data->dma_buffer = (unsigned long *)CKSEG0ADDR(substream->dma_buffer.area);
    } else {
        data->real_buffer = (unsigned long *)CKSEG0ADDR(substream->dma_buffer.area);
        data->dma_buffer = m_dma_alloc_coherent(data->dma_buffer_size);
    }

    if (data->dma_buffer == NULL) {
        printk("data->dma_buffer is null\n");
        return -EINVAL;
    }

    period_ms = 1000 * data->period_size / aic_dev.sample_rate / data->frame_size;
    data->period_time = period_ms * 1000 * 1000;

    data->dma_pos = 0;
    data->rw_pos = 0;
    data->data_size = 0;
    data->thread_stop = 0;

    data->substream = substream;

    return 0;
}

static snd_pcm_uframes_t aic_dma_pcm_pointer(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
    int pos;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        pos = aic_dev.playback.dma_pos;
    else
        pos = aic_dev.capture.read_pos;

    return bytes_to_frames(substream->runtime, pos);
}

static int aic_dma_pcm_hw_free(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        while (aic_dev.playback.is_running)
            usleep_range(600, 600);
    } else {
        while (aic_dev.capture.is_running)
            usleep_range(600, 600);
        m_dma_free_coherent(aic_dev.capture.dma_buffer_size);
    }

    return snd_pcm_lib_free_pages(substream);
}

static void aic_dma_pcm_free(struct snd_soc_component *component, struct snd_pcm *pcm)
{
    snd_pcm_lib_preallocate_free_for_all(pcm);

    return;
}

static int aic_dma_pcm_new(struct snd_soc_component *component, struct snd_soc_pcm_runtime *rtd)
{
    struct snd_pcm *pcm = rtd->pcm;

    snd_pcm_lib_preallocate_pages_for_all(pcm,
                    SNDRV_DMA_TYPE_DEV,
                    &aic_dev.pdev->dev,
                    MAX_DMA_BUFFERSIZE,
                    MAX_DMA_BUFFERSIZE);

    return 0;
}

static struct snd_soc_component_driver aic_components[2];

static struct snd_soc_dai_driver dai_drivers[2];

static void aic_data_init(int id, struct snd_soc_component_driver *aic_component,
        struct snd_soc_dai_driver *dai_driver)
{
    static char *component_name[] = {
        "ingenic-aic-component.0", "ingenic-aic-component.1",
    };

    static char *dai_name[] = {
        "ingenic-aic.0", "ingenic-aic.1",
    };

    aic_component->name = component_name[id];
    aic_component->open = aic_dma_pcm_open;
    aic_component->close = aic_dma_pcm_close;
    aic_component->prepare = aic_dma_pcm_prepare;
    aic_component->hw_params = aic_dma_pcm_hw_params;
    aic_component->hw_free = aic_dma_pcm_hw_free;
    aic_component->pointer = aic_dma_pcm_pointer;
    aic_component->mmap = aic_dma_pcm_mmap;
    aic_component->pcm_construct = aic_dma_pcm_new;
    aic_component->pcm_destruct = aic_dma_pcm_free;

    dai_driver->id = id;
    dai_driver->probe = aic_dai_probe;
    dai_driver->remove = aic_dai_remove;
    dai_driver->name = dai_name[id];
    dai_driver->ops = &aic_dai_ops;
    dai_driver->symmetric_rates = 1;
    dai_driver->symmetric_channels = 1;
    dai_driver->symmetric_samplebits = 1;

    dai_driver->capture.stream_name = "icodec capture";
    dai_driver->capture.channels_min = 1;
    dai_driver->capture.channels_max = 2;
    dai_driver->capture.rates = ASOC_AIC_RATE;
    dai_driver->capture.formats = ASOC_AIC_FORMATS;

    dai_driver->playback.stream_name = "i2s playback";
    dai_driver->playback.channels_min = 1;
    dai_driver->playback.channels_max = 2;
    dai_driver->playback.rates = ASOC_AIC_RATE;
    dai_driver->playback.formats = ASOC_AIC_FORMATS;
}

static int aic_probe(struct platform_device *pdev)
{
    int ret;
    int id = pdev->dev.id;

    aic_data_init(id, &aic_components[id], &dai_drivers[id]);

    ret = snd_soc_register_component(&pdev->dev, &aic_components[id], &dai_drivers[id], 1);
    if (ret)
        panic("AIC: aic snd_soc_register_component failed ret = %d!\n", ret);

    return 0;
}

static int aic_remove(struct platform_device *pdev)
{
    snd_soc_unregister_component(&pdev->dev);

    return 0;
}

static struct platform_driver aic_driver = {
    .probe = aic_probe,
    .remove = aic_remove,
    .driver = {
        .name = "ingenic-aic",
        .owner = THIS_MODULE,
    },
};

/* stop no dev release warning */
static void aic_device_release(struct device *dev){}

static struct platform_device aic_pdevs[2];

static int aic_init(void)
{
    struct clk *clk = clk_get(NULL, "ce_i2st");
    BUG_ON(IS_ERR(clk));
    clk_prepare_enable(clk);
    clk_put(clk);

    struct clk *gate_aic_clk = clk_get(NULL, "gate_aic");
    BUG_ON(IS_ERR(gate_aic_clk));
    clk_prepare_enable(gate_aic_clk);
    clk_put(gate_aic_clk);

    struct clk *gate_i2s_clk = clk_get(NULL, "gate_i2s");
    BUG_ON(IS_ERR(gate_i2s_clk));
    clk_prepare_enable(gate_i2s_clk);
    clk_put(gate_i2s_clk);

    struct clk *mux_clk = clk_get(NULL, "mux_i2scs");
    BUG_ON(IS_ERR(mux_clk));

    struct clk *parent = clk_get(NULL, "epll");
    clk_set_parent(mux_clk, parent);
    clk_put(parent);
    clk_put(mux_clk);

    struct device_node *np;
    np = of_find_compatible_node(NULL, NULL, "ingenic,x2600-aic");
    if (!np)
        return -1;

    aic_dev.div_clk = clk_get(NULL, "div_i2s_mn");
    BUG_ON(IS_ERR(aic_dev.div_clk));

    aic_dev.dev = &aic_pdevs[0].dev;
    dev_set_drvdata(aic_dev.dev, NULL);
    aic_dev.pdev = aic_pdevs;

    dma_param_init(&aic_dev.playback.dma, DMA_MEM_TO_DEV);
    dma_param_init(&aic_dev.capture.dma, DMA_DEV_TO_MEM);

    INIT_WORK(&aic_dev.playback.start_work, start_work);
    INIT_WORK(&aic_dev.capture.start_work, start_work);
    INIT_WORK(&aic_dev.playback.stop_work, stop_work);
    INIT_WORK(&aic_dev.capture.stop_work, stop_work);

    aic_dev.workqueue = create_singlethread_workqueue("aic_work");

    mutex_init(&aic_dev.mutex);
    mutex_init(&aic_dev.capture.mutex);
    mutex_init(&aic_dev.playback.mutex);

    int ret, i;
    for (i = 0; i < 2; i++) {
        aic_pdevs[i].id = i;
        aic_pdevs[i].name = "ingenic-aic";

        aic_pdevs[i].dev.id = i;
        aic_pdevs[i].dev.of_node = np;
        aic_pdevs[i].dev.release = aic_device_release;

        ret = platform_device_register(&aic_pdevs[i]);
        if (ret) {
            printk(KERN_ERR "AIC: Failed to register aic dev: %d\n", ret);
            return ret;
        }
    }

    struct dma_param *tx_dma = &aic_dev.playback.dma;
    struct dma_param *rx_dma = &aic_dev.capture.dma;

    tx_dma->dma_chan = dma_request_slave_channel(aic_dev.dev, "tx");
    if (!tx_dma->dma_chan)
        printk("AIC: aic dma tx_chan requested failed.\n");

    rx_dma->dma_chan = dma_request_slave_channel(aic_dev.dev, "rx");
    if (!rx_dma->dma_chan)
        printk("AIC: aic dma rx_chan requested failed.\n");

    return platform_driver_register(&aic_driver);
}
module_init(aic_init);

static void aic_exit(void)
{
    int i;

    dma_release_channel(aic_dev.playback.dma.dma_chan);
    dma_release_channel(aic_dev.capture.dma.dma_chan);

    destroy_workqueue(aic_dev.workqueue);

    clk_put(aic_dev.div_clk);

    for (i = 0; i < 2; i++)
        platform_device_unregister(&aic_pdevs[i]);

    platform_driver_unregister(&aic_driver);
}
module_exit(aic_exit);
MODULE_LICENSE("GPL");
