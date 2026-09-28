#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>

#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <dt-bindings/dma/ingenic-pdma.h>

#include <soc/gpio.h>
#include <linux/gpio.h>
#include <utils/gpio.h>
#include <linux/delay.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <common.h>
#include <pwm.h>
#include "pwm_audio.h"

#define PWM_DUTY_MAX_COUNT   (0xFFFF)

#define BUFF_SIZE_MAX    (PAGE_SIZE * 16)
#define PRD_SIZE_MAX    PAGE_SIZE
#define MIN_PERIODS    5

enum data_thread_processing_status {
    STATUS_clear,
    STATUS_start,
    STATUS_stop,
};

struct snd_pwm_audio_chip {
    struct snd_card *card;
    struct snd_pcm *pcm;

    struct snd_pcm_substream *substream;

    unsigned int alsa_buf_pos;
    struct task_struct *data_processing_task;
    wait_queue_head_t data_processing_waitq;
    wait_queue_head_t data_processing_pause;

    bool amp_power_enable;

    struct work_struct amp_power_work;
    struct workqueue_struct *amp_power_workqueue;

    int pwm_id;
    struct pwm_audio_pdata *pdata;
    unsigned int buf_size;

    unsigned int pwm_full_num;

    struct pwm_data *pwm_data;
    struct pwm_data *pwm_mute_data;

    dma_addr_t start_dma_addr;
    dma_addr_t end_dma_addr;

    unsigned int dma_buf_size;

    enum data_thread_processing_status thread_status;
    int thread_release;
    struct mutex lock;
};



static inline void *m_dma_alloc_coherent(int size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(NULL, size, &dma_handle, GFP_KERNEL);
    assert(mem);

    return mem;
}

static inline void m_dma_free_coherent(void *mem, int size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    dma_free_coherent(NULL, size, (void *)CKSEG1ADDR(mem), dma_handle);
}

void start_pwm_audio_thread(struct snd_pwm_audio_chip *pwm_audio)
{
    if (pwm_audio->pdata->amp_power_gpio >= 0) {
        struct pwm_dma_data pwm_dma_data = {
            .data = pwm_audio->pwm_mute_data,
            .id = pwm_audio->pwm_id,
            .data_count = pwm_audio->buf_size,
            .dma_loop = 1,
        };

        int usleep_time = pwm_audio->pdata->amp_mute_up_time * 1000;
        pwm2_dma_update(pwm_dma_data.id, &pwm_dma_data);
        usleep_range(usleep_time, usleep_time);

        gpio_set_value(pwm_audio->pdata->amp_power_gpio, 1);
        pwm2_dma_disable_loop(pwm_audio->pwm_id);
    }

    pwm_audio->thread_status = STATUS_start;
    wake_up(&pwm_audio->data_processing_waitq);
}

void stop_pwm_audio_thread(struct snd_pwm_audio_chip *pwm_audio)
{
    pwm_audio->thread_status = STATUS_stop;
    wake_up(&pwm_audio->data_processing_waitq);

    wait_event_timeout(pwm_audio->data_processing_pause, pwm_audio->thread_status == STATUS_clear,
                        msecs_to_jiffies(pwm_audio->pdata->unit_time * 4));

    if (pwm_audio->pdata->amp_power_gpio >= 0) {

        int usleep_time = pwm_audio->pdata->amp_mute_down_time * 1000;

        gpio_set_value(pwm_audio->pdata->amp_power_gpio, 0);

        usleep_range(usleep_time, usleep_time);
    }


    pwm2_dma_disable_loop(pwm_audio->pwm_id);

}

static void amp_power_work_handler(struct work_struct *p_work)
{
    struct snd_pwm_audio_chip *pwm_audio = container_of(p_work, struct snd_pwm_audio_chip, amp_power_work);

    mutex_lock(&pwm_audio->lock);

    if (pwm_audio->amp_power_enable)
        start_pwm_audio_thread(pwm_audio);
    else
        stop_pwm_audio_thread(pwm_audio);

    mutex_unlock(&pwm_audio->lock);

}

unsigned int pwm_audio_get_writeable_size(struct snd_pwm_audio_chip *pwm_audio,  dma_addr_t write_addr)
{
    dma_addr_t now_dma_addr = pwm2_get_current_dma_addr(pwm_audio->pwm_id);

    unsigned int write_able_size = 0;
    if (now_dma_addr < pwm_audio->start_dma_addr || now_dma_addr > pwm_audio->end_dma_addr)
        return 0;


    if (write_addr > now_dma_addr) {
        write_able_size = (pwm_audio->end_dma_addr - write_addr) + (now_dma_addr - pwm_audio->start_dma_addr);
    } else {
        write_able_size = now_dma_addr - write_addr;
    }

    return write_able_size;
}

static void write_data_to_pwm_dma(struct snd_pwm_audio_chip *pwm_audio,
                                  void **src, void **dst, int size)
{
    int i, j;
    short diff;
    int data_mul;
    short *audio_data = *dst;
    struct pwm_data *pwm_data = *src;

    struct snd_pcm_runtime *runtime;
    struct snd_pcm_substream *substream;

    substream = pwm_audio->substream;
    runtime = substream->runtime;

    data_mul = pwm_audio->pdata->base_freq / runtime->rate;

    snd_pcm_stream_lock(substream);

    for (i = 0; i < size; i += data_mul) {
        diff = audio_data[0] * pwm_audio->pwm_full_num / PWM_DUTY_MAX_COUNT;
        pwm_data[i].high = pwm_audio->pwm_full_num / 2 + diff;
        if (pwm_data[i].high >= pwm_audio->pwm_full_num)
            pwm_data[i].high = pwm_audio->pwm_full_num - 1;
        if (pwm_data[i].high == 0)
            pwm_data[i].high = 1;
        pwm_data[i].low = pwm_audio->pwm_full_num - pwm_data[i].high;

        for (j = 1; j < data_mul; j++) {
            pwm_data[i + j] = pwm_data[i];
        }

        audio_data++;
        if ((void *)audio_data >= (void *)runtime->dma_area + runtime->dma_bytes)
            audio_data = (short *)runtime->dma_area;
    }

    snd_pcm_stream_unlock(substream);

    pwm_data = pwm_data + size;
    if ((void *)pwm_data >= (void *)pwm_audio->pwm_data + pwm_audio->dma_buf_size)
        pwm_data = pwm_audio->pwm_data;

    *src = (void *)pwm_data;
    *dst = (void *)audio_data;
}

void write_full_mute_data_to_pwm_dma(struct snd_pwm_audio_chip *pwm_audio, struct pwm_data *pwm_data)
{
    int i = 0;
    int write_able_size;

    struct snd_pcm_runtime *runtime = pwm_audio->substream->runtime;
    int data_size_mul = (pwm_audio->pdata->base_freq / runtime->rate);

    int dma_threshold_size = pwm_audio->dma_buf_size / 4;
    int usleep_time = pwm_audio->pdata->unit_time * 1000 / 4 / 2;

    int cnt = 4;

    while(cnt) {
        write_able_size = pwm_audio_get_writeable_size(pwm_audio, virt_to_phys(pwm_data));
        if (write_able_size < dma_threshold_size) {
            usleep_range(usleep_time, usleep_time);
            continue;
        }
        write_able_size = dma_threshold_size;

        for (i = 0; i < write_able_size; i += data_size_mul) {
            *pwm_data = pwm_audio->pwm_mute_data[0];
            pwm_data++;
            if ((void *)pwm_data >= (void *)pwm_audio->pwm_data + pwm_audio->dma_buf_size)
                pwm_data = pwm_audio->pwm_data;
        }

        cnt--;
    }
}

static int data_processing_thread(void *data)
{
    const short *audio_data;

    struct snd_pwm_audio_chip *pwm_audio = data;
    struct pwm_data *pwm_data = pwm_audio->pwm_data;
    struct pwm_dma_data dma_data;

    volatile int need_start = 1;

    unsigned int write_able_size = 0;
    unsigned int write_buf_size = pwm_audio->buf_size;

    int usleep_time = 0;
    int dma_threshold_size;

    struct snd_pcm_runtime *runtime = pwm_audio->substream->runtime;
    int data_size_mul = pwm_audio->pdata->base_freq / runtime->rate * sizeof(struct pwm_data);

    while (1) {
        wait_event(pwm_audio->data_processing_waitq,
                   pwm_audio->thread_status || pwm_audio->thread_release);

        if (pwm_audio->thread_status == STATUS_stop) {
            write_full_mute_data_to_pwm_dma(pwm_audio, pwm_data);

            pwm_audio->alsa_buf_pos = 0;
            snd_pcm_period_elapsed(pwm_audio->substream);

            need_start = 1;

            pwm_audio->thread_status = STATUS_clear;
            wake_up(&pwm_audio->data_processing_pause);
            continue;
        }

        if (pwm_audio->thread_release)
            break;

        if (need_start) {
            audio_data = (const short *)runtime->dma_area;
            write_data_to_pwm_dma(pwm_audio, (void **)&pwm_data, (void **)&audio_data, pwm_audio->buf_size);

            dma_data.data = pwm_audio->pwm_data;
            dma_data.data_count = pwm_audio->buf_size;
            dma_data.dma_loop = 1;
            dma_data.id = pwm_audio->pwm_id;
            pwm2_dma_update(pwm_audio->pwm_id, &dma_data);

            need_start = 0;

            dma_threshold_size = data_size_mul;
            usleep_time = 0;
        }

        write_able_size = pwm_audio_get_writeable_size(pwm_audio, virt_to_phys(pwm_data));

        if (write_able_size >= dma_threshold_size) {
            write_buf_size = dma_threshold_size / sizeof(struct pwm_data);

            write_data_to_pwm_dma(pwm_audio, (void **)&pwm_data, (void **)&audio_data, write_buf_size);
            pwm_audio->alsa_buf_pos = (void *)audio_data - (void *)runtime->dma_area;
            snd_pcm_period_elapsed(pwm_audio->substream);

            usleep_time = pwm_audio->pdata->unit_time * 1000 / 4;
            dma_threshold_size = pwm_audio->dma_buf_size / 4;
        } else {
            usleep_range(usleep_time / 2, usleep_time / 2);
        }
    }

    pwm_audio->alsa_buf_pos = 0;
    snd_pcm_period_elapsed(pwm_audio->substream);

    return 0;
}

static int pwm_audio_pcm_open(struct snd_pcm_substream *substream)
{
    struct snd_pwm_audio_chip *pwm_audio = substream->private_data;
    struct snd_pcm_runtime *runtime = substream->runtime;

    runtime->hw.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER
         | SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID;
    runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE;

    runtime->hw.rates = SNDRV_PCM_RATE_96000;
    runtime->hw.rate_min = 96000;
    runtime->hw.rate_max = 96000;

    runtime->hw.channels_min = 1;
    runtime->hw.channels_max = 1;
    runtime->hw.buffer_bytes_max = BUFF_SIZE_MAX,
    runtime->hw.period_bytes_min = pwm_audio->dma_buf_size / MIN_PERIODS,
    runtime->hw.period_bytes_max = PRD_SIZE_MAX,
    runtime->hw.periods_min = MIN_PERIODS,
    runtime->hw.periods_max = BUFF_SIZE_MAX / PRD_SIZE_MAX,
    runtime->hw.fifo_size = 0;

    pwm_audio->thread_status = STATUS_clear;
    pwm_audio->thread_release = 0;
    pwm_audio->alsa_buf_pos = 0;

    snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

    pwm_audio->substream = substream;
    pwm_audio->data_processing_task = kthread_run(data_processing_thread, pwm_audio, "pwm_audio_data");
    if (IS_ERR(pwm_audio->data_processing_task)) {
        printk(KERN_ERR "%s: Couldn't start thread\n", __func__);
        return PTR_ERR(pwm_audio->data_processing_task);
    }

    return 0;
}

int pwm_audio_pcm_close(struct snd_pcm_substream *substream)
{
    struct snd_pwm_audio_chip *pwm_audio = substream->private_data;
    pwm_audio->thread_release = 1;
    wake_up(&pwm_audio->data_processing_waitq);

    kthread_stop(pwm_audio->data_processing_task);
    return 0;
}


static int pwm_audio_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
    struct snd_pwm_audio_chip *pwm_audio = substream->private_data;
    int err = 0;

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    if (pwm_audio->amp_power_enable)
        break;

        pwm_audio->amp_power_enable = true;
        queue_work(pwm_audio->amp_power_workqueue, &pwm_audio->amp_power_work);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    if (!pwm_audio->amp_power_enable)
        break;

        pwm_audio->amp_power_enable = false;
        queue_work(pwm_audio->amp_power_workqueue, &pwm_audio->amp_power_work);
        break;
    default:
        err = -EINVAL;
    }

    return err;
}

static snd_pcm_uframes_t pwm_audio_pcm_pointer(struct snd_pcm_substream *substream)
{
    struct snd_pwm_audio_chip *pwm_audio = substream->private_data;
    return bytes_to_frames(substream->runtime, pwm_audio->alsa_buf_pos);
}

static int pwm_audio_pcm_null(struct snd_pcm_substream *substream)
{
    return 0;
}

static int pwm_audio_pcm_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *hw_params)
{
    int ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
    if (ret < 0) {
        printk(KERN_EMERG "failed to allocate pages: %d\n", ret);
        return ret;
    }

    return 0;
}

static int pwm_audio_pcm_hw_free(struct snd_pcm_substream *substream)
{
    return snd_pcm_lib_free_pages(substream);
}

static const struct snd_pcm_ops pwm_audio_pcm_ops = {
    .open = pwm_audio_pcm_open,
    .close = pwm_audio_pcm_close,
    .trigger = pwm_audio_pcm_trigger,
    .pointer = pwm_audio_pcm_pointer,
    .prepare = pwm_audio_pcm_null,
    .ioctl = snd_pcm_lib_ioctl,
    .hw_params = pwm_audio_pcm_hw_params,
    .hw_free = pwm_audio_pcm_hw_free,
};

static int pwm_audio_probe(struct platform_device *pdev)
{
    int i;
    int err;
    int rate;
    struct snd_pcm *pcm;
    struct snd_card *card;
    struct snd_pwm_audio_chip *pwm_audio;
    struct pwm_dma_config dma_config;
    struct pwm_audio_pdata *pdata = dev_get_platdata(&pdev->dev);

    if (!pdata) {
        printk(KERN_ERR "%s: pwm_audio_pdata is NULL\n", __func__);
        return -EINVAL;
    }

    err = snd_card_new(&pdev->dev, -1, NULL, THIS_MODULE,
               sizeof(struct snd_pwm_audio_chip), &card);
    if (err < 0) {
        printk(KERN_ERR "%s: snd_card_new fail\n", __func__);
        return err;
    }

    pwm_audio = card->private_data;
    pwm_audio->card = card;
    pwm_audio->pdata = pdata;

    err = snd_pcm_new(card, "pwm_audio_pcm", 0, 1, 0, &pcm);
    if (err < 0) {
        printk(KERN_ERR "%s: snd_pcm_new fail\n", __func__);
        goto err_card_free;
    }

    pwm_audio->pcm = pcm;
    pcm->private_data = pwm_audio;

    strcpy(pcm->name, "pwm_audio_pcm");
    snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &pwm_audio_pcm_ops);

    snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
                       NULL, BUFF_SIZE_MAX, BUFF_SIZE_MAX);

    if (gpio_is_valid(pdata->amp_power_gpio)) {
        err = gpio_request_one(pdata->amp_power_gpio, GPIOF_DIR_OUT|GPIOF_INIT_LOW, "pwm_audio_amp_power");
        if (err < 0) {
            printk(KERN_ERR "%s: amp_power_gpio gpio_request fail\n", __func__);
            err = -EBUSY;
            goto err_card_free;
        }
    }

    INIT_WORK(&pwm_audio->amp_power_work, amp_power_work_handler);
    pwm_audio->amp_power_workqueue = create_singlethread_workqueue("amp_power_work");

    pwm_audio->buf_size = pdata->base_freq * pdata->unit_time / 1000;
    pwm_audio->pwm_id = pwm2_request(pdata->pwm_gpio, "pwm_audio");
    if (pwm_audio->pwm_id < 0) {
        printk(KERN_ERR "%s: pwm2_request fail\n", __func__);
        err = -EBUSY;
        goto err_destroy_workqueue;
    }

    dma_config.id = pwm_audio->pwm_id;
    dma_config.idle_level = PWM_idle_low;
    dma_config.start_level = PWM_start_high;
    rate = pwm2_dma_init(pwm_audio->pwm_id, &dma_config);
    if (rate < 0) {
        printk(KERN_ERR "%s: pwm%d pwm2_dma_init fail\n", __func__, pwm_audio->pwm_id);
        err = -EBUSY;
        goto err_pwm_release;
    }

    if (rate % pdata->base_freq) {
        printk(KERN_ERR "%s: pwm%d not support base freq %d, rate %d\n", __func__, pwm_audio->pwm_id, pdata->base_freq, rate);
        err = -EINVAL;
        goto err_pwm_release;
    }

    pwm_audio->pwm_full_num = rate / pdata->base_freq;
    pwm_audio->dma_buf_size = pwm_audio->buf_size * sizeof(struct pwm_data);
    pwm_audio->pwm_data = m_dma_alloc_coherent(pwm_audio->dma_buf_size);

    pwm_audio->start_dma_addr = virt_to_phys(pwm_audio->pwm_data);
    pwm_audio->end_dma_addr = pwm_audio->start_dma_addr + pwm_audio->dma_buf_size;
    if (!pwm_audio->pwm_data) {
        printk(KERN_ERR "%s: pwm%d malloc pwm buf fail\n", __func__, pwm_audio->pwm_id);
        err = -ENOMEM;
        goto err_pwm_release;
    }

    pwm_audio->pwm_mute_data = m_dma_alloc_coherent(pwm_audio->dma_buf_size);
    if (!pwm_audio->pwm_mute_data) {
        printk(KERN_ERR "%s: pwm%d malloc pwm mute buf fail\n", __func__, pwm_audio->pwm_id);
        err = -ENOMEM;
        goto err_free_pwm_data;
    }

    /* init mute data */
    pwm_audio->pwm_mute_data[0].high = pwm_audio->pwm_full_num / 2;
    pwm_audio->pwm_mute_data[0].low = pwm_audio->pwm_full_num - pwm_audio->pwm_mute_data[0].high;
    for (i = 1; i < pwm_audio->buf_size; i++) {
        pwm_audio->pwm_mute_data[i] = pwm_audio->pwm_mute_data[0];
    }

    init_waitqueue_head(&pwm_audio->data_processing_waitq);
    init_waitqueue_head(&pwm_audio->data_processing_pause);
    mutex_init(&pwm_audio->lock);

    strcpy(card->driver, "pwm_audio");
    strcpy(card->shortname, "pwm_audio");
    strcpy(card->longname, "pwm_audio");
    err = snd_card_register(card);
    if (err < 0) {
        printk(KERN_ERR "%s: snd_card_register fail\n", __func__);
        goto err_free_pwm_mute_data;
    }

    platform_set_drvdata(pdev, card);
    return 0;

err_free_pwm_mute_data:
    m_dma_free_coherent(pwm_audio->pwm_mute_data, pwm_audio->dma_buf_size);
err_free_pwm_data:
    m_dma_free_coherent(pwm_audio->pwm_data, pwm_audio->dma_buf_size);
err_pwm_release:
    pwm2_release(pwm_audio->pwm_id);
err_destroy_workqueue:
    if (gpio_is_valid(pdata->amp_power_gpio))
        gpio_free(pdata->amp_power_gpio);
err_card_free:
    snd_card_free(card);
    return err;
}

static int pwm_audio_remove(struct platform_device *pdev)
{
    struct snd_card *card = platform_get_drvdata(pdev);
    struct snd_pwm_audio_chip *pwm_audio = card->private_data;
    int amp_power_gpio = pwm_audio->pdata->amp_power_gpio;
    int pwm_id = pwm_audio->pwm_id;
    void *pwm_data = pwm_audio->pwm_data;
    void *pwm_mute_data = pwm_audio->pwm_mute_data;

    m_dma_free_coherent(pwm_mute_data, pwm_audio->dma_buf_size);
    m_dma_free_coherent(pwm_data, pwm_audio->dma_buf_size);

    snd_card_free(card);
    pwm2_release(pwm_id);
    if (gpio_is_valid(amp_power_gpio)) {
        gpio_free(amp_power_gpio);
    }

    return 0;
}

static struct platform_driver pwm_audio_driver = {
    .probe = pwm_audio_probe,
    .remove = pwm_audio_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "pwm-audio",
    },
};

/* stop no dev release warning */
static void pwm_audio_dev_release(struct device *dev){}

struct pwm_audio_pdata pwm_audio_pdata_1 = {
    .pwm_gpio = -1,
    .amp_power_gpio = -1,
    .base_freq = 96000,
    .unit_time = 50,
};

struct platform_device pwm_audio_device_1 = {
    .name = "pwm-audio",
    .dev  = {
        .release = pwm_audio_dev_release,
        .platform_data = &pwm_audio_pdata_1,
    },
};

struct pwm_audio_pdata pwm_audio_pdata_2 = {
    .pwm_gpio = -1,
    .amp_power_gpio = -1,
    .base_freq = 96000,
    .unit_time = 50,
};

struct platform_device pwm_audio_device_2 = {
    .name = "pwm-audio",
    .dev  = {
        .release = pwm_audio_dev_release,
        .platform_data = &pwm_audio_pdata_2,
    },
};

module_param_gpio_named(pwm_gpio_1, pwm_audio_pdata_1.pwm_gpio, 0644);
module_param_gpio_named(amp_power_gpio_1, pwm_audio_pdata_1.amp_power_gpio, 0644);
module_param_named(amp_mute_up_time_1, pwm_audio_pdata_1.amp_mute_up_time, int, 0644);
module_param_named(amp_mute_down_time_1, pwm_audio_pdata_1.amp_mute_down_time, int,  0644);

module_param_gpio_named(pwm_gpio_2, pwm_audio_pdata_2.pwm_gpio, 0644);
module_param_gpio_named(amp_power_gpio_2, pwm_audio_pdata_2.amp_power_gpio, 0644);
module_param_named(amp_mute_up_time_2, pwm_audio_pdata_2.amp_mute_up_time, int, 0644);
module_param_named(amp_mute_down_time_2, pwm_audio_pdata_2.amp_mute_down_time, int, 0644);

static int __init pwm_audio_init(void)
{
    if (gpio_is_valid(pwm_audio_pdata_1.pwm_gpio))
        platform_device_register(&pwm_audio_device_1);

    if (gpio_is_valid(pwm_audio_pdata_2.pwm_gpio))
        platform_device_register(&pwm_audio_device_2);

    return platform_driver_register(&pwm_audio_driver);
}
module_init(pwm_audio_init);

static void __exit pwm_audio_exit(void)
{
    if (gpio_is_valid(pwm_audio_pdata_1.pwm_gpio))
        platform_device_unregister(&pwm_audio_device_1);

    if (gpio_is_valid(pwm_audio_pdata_2.pwm_gpio))
        platform_device_unregister(&pwm_audio_device_2);

    platform_driver_unregister(&pwm_audio_driver);
}
module_exit(pwm_audio_exit);

MODULE_DESCRIPTION("pwm audio driver");
MODULE_LICENSE("GPL");
