#ifndef __AUDIO_PLAYER_H__
#define __AUDIO_PLAYER_H__

#include <libmedia/audio_frame.h>

struct audio_player;
struct audio_player_param;

struct audio_player_cb {
    struct audio_player *(*open_player)(struct audio_player_param *param);
    void (*close_player)(struct audio_player *player);
    int (*display_audio)(struct audio_player *player, struct audio_frame *frame);
};

struct audio_player_param {
    int rate;
    int channels;
    enum audio_frame_format format;

    struct audio_player_cb *cb;
};

struct audio_player {
    struct audio_player_param param;
};


struct audio_player *audio_player_open(struct audio_player_param *param);
int audio_player_display_audio(struct audio_player *player, struct audio_frame *frame);
void audio_player_close(struct audio_player *player);

#endif