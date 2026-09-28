#ifndef _ALSA_SPEEX_AEC_CC_card_H_
#define _ALSA_SPEEX_AEC_CC_card_H_

#include <libhardware2/alsa.h>
#include <libmedia/audio_frame.h>

struct aec_cc_card;

struct speex_cc_params {
    // 算法处理单元的样本数，也是算法输出帧的样本数，帧的其余参数与录制音频帧的参数一致
    int samples;
    // 为播放与录音音频对齐，录音录过align_ms(与播放音频对齐)后才传入算法
    int capture_align_ms;
    // 算法对于播放与录音音频不对齐的帧数的容忍程度
    // 直接影响算法产生消除回声后第一帧的快慢
    // 单位为算法处理单元的样本数aec_samples
    int filter_count;

    int echo_volume;
    int capture_volume;
};

struct alsa_speex_aec_cc_card_param {
    // 对播放进行回采的alsa设备（常见为amic）
    const char *alsa_echo_device;
    const char *alsa_capture_device;

    const char *alsa_echo_ctl_device;
    const char *alsa_capture_ctl_device;

    const char *alsa_echo_ctl_name;
    const char *alsa_capture_ctl_name;

    // alsa 对应的参数(仅支持播放与录制设置相同)
    struct alsa_params alsa_params;
    struct speex_cc_params speex_params;
};

void alsa_aec_cc_card_init_default_param(struct alsa_speex_aec_cc_card_param *param, int channels, int rate);

struct aec_cc_card *aec_cc_card_open(struct alsa_speex_aec_cc_card_param *param);
void aec_cc_card_close(struct aec_cc_card *card);

int aec_cc_card_read_frame(struct aec_cc_card *card, struct audio_frame **frame);

int aec_cc_card_drop_capture(struct aec_cc_card *card);
int aec_cc_card_avail_capture(struct aec_cc_card *card);

#endif