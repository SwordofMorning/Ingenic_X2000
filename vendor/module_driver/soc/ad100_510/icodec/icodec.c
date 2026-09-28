#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <asm/div64.h>
#include <sound/soc-dai.h>
#include <linux/spinlock.h>
#include <utils/gpio.h>
#include <linux/clk.h>

#define ICODEC_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
#define ICODEC_RATE (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_44100 | \
                        SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000)

struct icodec_data {
    int ref_count;
    int adc_is_enable;
    int dac_is_enable;

    spinlock_t lock;

    struct mutex mutex;
};

struct icodec_data icodec;

#define MAX_MIC_VOLUME 0xff

static unsigned int max_dac_volume = 0xff;
static unsigned int min_dac_volume = 0;
static int dac_dgain = 0xe2;  // 0db
static int adc_dgain = 0xc4;  // 0db
static int mic_in_gain = 3;   // 30db mic gain
static int alcl_gain = 6;     // 0db alcl gain
static int dac_mute = 0;
static int spk_gpio = -1; /* PE05 */
static int speaker_gpio_level = 1;
static int speaker_need_delay_ms = 20;
static int bias_enable = 1;
static int bias_level = 7;
static int hpl_gain = 0x1a;   // 0db

module_param(max_dac_volume, int, 0644);
module_param(min_dac_volume, int, 0644);

module_param_gpio_named(speaker_gpio, spk_gpio, 0644);
module_param(speaker_gpio_level, int, 0644);

module_param(speaker_need_delay_ms, int, 0644);

module_param(bias_enable, int, 0644);
module_param(bias_level, int, 0644);
module_param(hpl_gain, int, 0644);

module_param(mic_in_gain, int, 0644);
module_param(alcl_gain, int, 0644);

#include "icodec_hal.c"

static int spkeaker_gpio_request(void)
{
    int ret = 0;
    char buf[10];

    if (spk_gpio >= 0) {
        ret = gpio_request(spk_gpio, "spk_gpio");
        if (ret < 0)
            printk(KERN_ERR "ICODEC: speaker gpio failed to request %s.\n", gpio_to_str(spk_gpio, buf));
        gpio_direction_output(spk_gpio, !speaker_gpio_level);
    }

    return ret;
}

static void spkeaker_gpio_release(void)
{
    if (spk_gpio >= 0)
       gpio_free(spk_gpio);
}

static void spkeaker_enable(void)
{
    if (spk_gpio >= 0)
        gpio_direction_output(spk_gpio, speaker_gpio_level);

    if (speaker_need_delay_ms) {
        int us = speaker_need_delay_ms * 1000;
        usleep_range(us, us);
    }
}

static void spkeaker_disable(void)
{
    if (spk_gpio >= 0)
        gpio_direction_output(spk_gpio, !speaker_gpio_level);
}

static int to_data_bits(int format)
{
    if (format == SNDRV_PCM_FORMAT_S16_LE)
        return 16;
    if (format == SNDRV_PCM_FORMAT_S24_LE)
        return 24;
    if (format == SNDRV_PCM_FORMAT_S32_LE)
        return 32;
    return 16;
}

static int icodec_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
    int format = params_format(params);
    int data_bits = to_data_bits(format);
    int sample_rate = params_rate(params);
    int channel = params_channels(params);
    unsigned long flags;

    mutex_lock(&icodec.mutex);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        if (!icodec.dac_is_enable) {
            if (icodec.ref_count++ == 0) {
                icodec_power_on();
                usleep_range(10 * 1000, 10 * 1000);
            }

            spkeaker_enable();

            spin_lock_irqsave(&icodec.lock, flags);
            icodec_disable_dac();
            icodec_config_dac(data_bits, sample_rate);
            icodec_enable_dac(dac_mute, dac_dgain, hpl_gain);
            spin_unlock_irqrestore(&icodec.lock, flags);

            icodec.dac_is_enable = 1;
        }
    } else {
        if (!icodec.adc_is_enable) {
            if (icodec.ref_count++ == 0) {
                icodec_power_on();
                usleep_range(10 * 1000, 10 * 1000);
            }

            spin_lock_irqsave(&icodec.lock, flags);
            icodec_config_adc(data_bits, channel);
            icodec_enable_adc(bias_enable, bias_level);
            spin_unlock_irqrestore(&icodec.lock, flags);

            usleep_range(10 * 1000, 10 * 1000);

            spin_lock_irqsave(&icodec.lock, flags);
            icodec_enable_adc1(adc_dgain, mic_in_gain, alcl_gain);
            spin_unlock_irqrestore(&icodec.lock, flags);

            icodec.adc_is_enable = 1;
        }
    }
    mutex_unlock(&icodec.mutex);

    return 0;
}

static inline int icodec_hw_free(struct snd_pcm_substream *substream,
        struct snd_soc_dai *dai)
{
    mutex_lock(&icodec.mutex);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        if (icodec.dac_is_enable) {
            icodec_disable_dac();
            usleep_range(30*1000, 30*1000);

            spkeaker_disable();

            icodec.dac_is_enable = 0;

            if (--icodec.ref_count == 0)
                icodec_power_off();
        }
    } else {
        if (icodec.adc_is_enable) {
            icodec_disable_adc();
            icodec.adc_is_enable = 0;

            if (--icodec.ref_count == 0)
                icodec_power_off();
        }
    }

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int icodec_trigger(struct snd_pcm_substream * stream, int cmd,
        struct snd_soc_dai *dai)
{
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        break;
    }
    return 0;
}

static int icodec_probe(struct snd_soc_component *component)
{
    int ret = 0;

    if (max_dac_volume > 0xff)
        max_dac_volume = 0xff;

    if (min_dac_volume > max_dac_volume)
        min_dac_volume = max_dac_volume;

    if (!dac_dgain) {
        dac_dgain = min_dac_volume +
            (max_dac_volume - min_dac_volume) * 7 / 10;
    }

    mutex_init(&icodec.mutex);

    spin_lock_init(&icodec.lock);

    ret = spkeaker_gpio_request();
    if (ret < 0)
        return ret;

    return 0;
}

static void icodec_remove(struct snd_soc_component *component)
{
    spkeaker_gpio_release();
}

static int spk_info_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_info *uinfo)
{
    uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
    uinfo->count = 1;
    uinfo->value.integer.min = min_dac_volume;
    uinfo->value.integer.max = max_dac_volume;
    return 0;
}

static int spk_get_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    mutex_lock(&icodec.mutex);

    if (icodec.dac_is_enable)
        uctl->value.integer.value[0] = icodec_get_dac_gain();
    else
        uctl->value.integer.value[0] = dac_dgain;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int spk_put_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    int vol;

    mutex_lock(&icodec.mutex);

    vol = uctl->value.integer.value[0];
    if (vol < min_dac_volume)
        vol = min_dac_volume;
    if (vol > max_dac_volume)
        vol = max_dac_volume;

    if (vol == min_dac_volume)
        dac_mute = 1;
    else
        dac_mute = 0;

    if (icodec.dac_is_enable) {
        icodec_set_dac_mute(dac_mute);
        icodec_set_dac_gain(vol);
    }

    dac_dgain = vol;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int mic_info_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_info *uinfo)
{
    uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
    uinfo->count = 1;
    uinfo->value.integer.min = 0;
    uinfo->value.integer.max = MAX_MIC_VOLUME;
    return 0;
}

static int mic_get_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    mutex_lock(&icodec.mutex);

    if (icodec.adc_is_enable)
        uctl->value.integer.value[0] = icodec_get_adc_gain();
    else
        uctl->value.integer.value[0] = adc_dgain;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int mic_put_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    int vol;

    mutex_lock(&icodec.mutex);

    vol = uctl->value.integer.value[0];
    if (vol < 0)
        vol = 0;
    if (vol > MAX_MIC_VOLUME)
        vol = MAX_MIC_VOLUME;
    if (icodec.adc_is_enable)
        icodec_set_adc_gain(vol);

    adc_dgain = vol;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int mute_info(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_info *uinfo)
{
    uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
    uinfo->count = 1;
    uinfo->value.integer.min = 0;
    uinfo->value.integer.max = 1;
    return 0;
}

static int mute_get(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    mutex_lock(&icodec.mutex);

    if (icodec.dac_is_enable)
        uctl->value.integer.value[0] = icodec_get_dac_mute();
    else
        uctl->value.integer.value[0] = dac_mute;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int mute_set(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    int mute;

    mutex_lock(&icodec.mutex);

    mute = uctl->value.integer.value[0];

    dac_mute = mute;

    if (icodec.dac_is_enable)
        icodec_set_dac_mute(mute);

    mutex_unlock(&icodec.mutex);

    return 0;
}

static struct snd_soc_dai_ops icodec_dai_ops = {
    .hw_params = icodec_hw_params,
    .hw_free = icodec_hw_free,
    .trigger = icodec_trigger,
};

static const DECLARE_TLV_DB_SCALE(dac_dgain_tlv, -12100, 50, 0);
static const DECLARE_TLV_DB_SCALE(adc_dgain_tlv, -9550, 50, 0);

static struct snd_kcontrol_new icodec_controls[] = {
    [0] = {
        .iface  = SNDRV_CTL_ELEM_IFACE_MIXER,
        .name   = "Master Playback Volume",
        .access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | SNDRV_CTL_ELEM_ACCESS_READWRITE,
        .info   = spk_info_volume,
        .get    = spk_get_volume,
        .put    = spk_put_volume,
        .tlv.p  = dac_dgain_tlv,
    },
    [1] = {
        .iface  = SNDRV_CTL_ELEM_IFACE_MIXER,
        .name   = "Mic Volume",
        .access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | SNDRV_CTL_ELEM_ACCESS_READWRITE,
        .info   = mic_info_volume,
        .get    = mic_get_volume,
        .put    = mic_put_volume,
        .tlv.p  = adc_dgain_tlv,
    },
    [2] = {
        .iface  = SNDRV_CTL_ELEM_IFACE_MIXER,
        .name   = "Playback Mute",
        .access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
        .info   = mute_info,
        .get    = mute_get,
        .put    = mute_set,
    },
};

static struct snd_soc_component_driver soc_component_dev_icodec = {
    .probe = icodec_probe,
    .remove = icodec_remove,
    .controls = icodec_controls,
    .num_controls = ARRAY_SIZE(icodec_controls),
};

static struct snd_soc_dai_driver icodec_dai = {
    .name = "internal-codec",
    .playback = {
        .stream_name = "playback",
        .channels_min = 1,
        .channels_max = 1,
        .rates = ICODEC_RATE,
        .formats = ICODEC_FORMATS,
    },
    .capture = {
        .stream_name = "Capture",
        .channels_min = 1,
        .channels_max = 1,
        .rates = ICODEC_RATE,
        .formats = ICODEC_FORMATS,
    },
    .symmetric_rates = 1,
    .ops = &icodec_dai_ops,
};

static int icodec_plat_probe(struct platform_device *pdev)
{
    int ret;

    ret = snd_soc_register_component(&pdev->dev, &soc_component_dev_icodec, &icodec_dai, 1);
    if (ret)
        panic("ICODEC: asoc register icodec failed ret = %d\n", ret);

    return 0;
}

static int icodec_plat_remove(struct platform_device *pdev)
{
    snd_soc_unregister_component(&pdev->dev);

    return 0;
}

/* stop no dev release warning */
static void asoc_icodec_dev_release(struct device *dev){}

static struct platform_device icodec_device = {
    .id   = -1,
    .name = "ingenic-icodec",
    .dev  = {
        .release = asoc_icodec_dev_release,
    },
};

static struct platform_driver icodec_driver = {
    .driver = {
        .name = "ingenic-icodec",
        .owner = THIS_MODULE,
    },
    .probe = icodec_plat_probe,
    .remove = icodec_plat_remove,
};

static int icodec_init(void)
{
    int ret;

    ret = platform_device_register(&icodec_device);
    if (ret)
        return ret;

    return platform_driver_register(&icodec_driver);
}
module_init(icodec_init);

static void icodec_exit(void)
{
    platform_device_unregister(&icodec_device);
    platform_driver_unregister(&icodec_driver);
}
module_exit(icodec_exit);
MODULE_LICENSE("GPL");
