#ifndef __ALSA_AUDIO_PLAYER_H__
#define __ALSA_AUDIO_PLAYER_H__

#include <libhardware2/alsa.h>
#include <libmedia/audio_player.h>

struct alsa_audio_player_param {
    struct audio_player_param param;

    /**
     * alsa 设备名字, 如 "plughw:1,0"
     */
    const char *alsa_playback_device;

    /**
     * alsa 对应的参数
     */
    struct alsa_params alsa_params;
};

void alsa_audio_player_init_param(struct alsa_audio_player_param *alsa_param);

void alsa_audio_player_init_default_param(struct alsa_audio_player_param *param, int channles, int rate);

#endif