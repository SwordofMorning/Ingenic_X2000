#ifndef _ASYNC_AUDIO_PLAYER_H_
#define _ASYNC_AUDIO_PLAYER_H_

#include <libmedia/audio_player.h>

struct async_audio_player_param {
    struct audio_player_param param;
    struct audio_player_param *device_param;
    const char *device_node;
};

void async_audio_player_init_param(
    struct async_audio_player_param *param,
    const char *device_node, struct audio_player_param *device_param);

#endif /* _ASYNC_AUDIO_PLAYER_H_ */
