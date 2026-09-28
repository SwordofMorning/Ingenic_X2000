#ifndef _ALSA_AUDIO_READER_H_
#define _ALSA_AUDIO_READER_H_

#include <libhardware2/alsa.h>
#include <libmedia/audio_reader.h>

struct alsa_audio_reader_param {
    struct audio_reader_param param;

    /**
     * alsa 设备名字, 如 "plughw:1,0"
     */
    const char *alsa_capture_device;

    /**
     * alsa 对应的参数
     */
    struct alsa_params alsa_params;
};

void alsa_audio_reader_init_param(struct alsa_audio_reader_param *alsa_param);

void alsa_audio_reader_init_default_param(
    struct alsa_audio_reader_param *param, int channles, int rate);

#endif /* _ALSA_AUDIO_READER_H_ */
