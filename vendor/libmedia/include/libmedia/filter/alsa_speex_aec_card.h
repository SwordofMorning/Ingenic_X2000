#ifndef _ALSA_SPEEX_AEC_CARD_H_
#define _ALSA_SPEEX_AEC_CARD_H_

#include <libhardware2/alsa.h>
#include <libmedia/audio_frame.h>

struct aec_card;

struct speex_params {
    // 算法处理单元的样本数，也是算法输出帧的样本数，帧的其余参数与录制音频帧的参数一致
    int aec_samples;
    // 为播放与录音音频对齐，录音录过align_ms(与播放音频对齐)后才传入算法
    int capture_align_ms;
    // 算法对于播放与录音音频不对齐的帧数的容忍程度
    // 直接影响算法产生消除回声后第一帧的快慢
    // 单位为算法处理单元的样本数aec_samples
    int filter_count;

    // 在喇叭静音时，录制设备的音量大小
    int mic_volume_m;
    // 喇叭播放时，录音设备的音量大小（一般设置的比前者小，防止录到的喇叭声过大影响录制效果）
    int mic_volume_p;
    int spk_volume;
};

struct alsa_speex_aec_card_param {
    // alsa 设备名字, 如 "plughw:1,0"
    const char *alsa_capture_device;
    const char *alsa_playback_device;

    const char *alsa_capture_ctl_device;
    const char *alsa_playback_ctl_device;

    const char *alsa_capture_ctl_name;
    const char *alsa_playback_ctl_name;

    // alsa 对应的参数(仅支持播放与录制设置相同)
    struct alsa_params alsa_params;
    struct speex_params speex_params;
};

void alsa_aec_card_init_default_param(struct alsa_speex_aec_card_param *param, int channels, int rate);

struct aec_card *aec_card_open(struct alsa_speex_aec_card_param *param);
void aec_card_close(struct aec_card *aec_card);

int aec_card_display_frame(struct aec_card *aec_card, struct audio_frame *frame);
int aec_card_read_frame(struct aec_card *aec_card, struct audio_frame **frame);

int aec_card_pause_player(struct aec_card *aec_card);
int aec_card_resume_player(struct aec_card *aec_card);
int aec_card_drop_capture(struct aec_card *aec_card);
int aec_card_avail_capture(struct aec_card *aec_card);

#endif