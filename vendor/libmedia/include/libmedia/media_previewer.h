#ifndef __MEDIA_PREVIEWER_H__
#define __MEDIA_PREVIEWER_H__

#include <semaphore.h>
#include <pthread.h>

#include <libmedia/video_player.h>
#include <libmedia/video_reader.h>
#include <libmedia/audio_reader.h>
#include <libmedia/rotate.h>

struct media_previewer_param {
    struct audio_reader_param *audio_reader_param;
    struct video_reader_param *video_reader_param;
    struct video_player_param *video_player_param;
    struct video_rotater_param *player_rotater_param;
};

struct media_previewer {
    struct audio_reader *audio;
    struct video_reader *video;
    struct video_player *player;
    struct media_previewer_param param;

    struct video_rotater *player_rotater;

    pthread_t video_display_thread;
    sem_t video_frame_ok;
    sem_t video_display_done;
    volatile int is_stop;
};

struct media_previewer *media_previewer_open(struct media_previewer_param *param);

void media_previewer_close(struct media_previewer *previewer);

int media_previewer_display_video_frame(
    struct media_previewer *previewer, struct video_frame *frame, int timeout_ms);


#endif