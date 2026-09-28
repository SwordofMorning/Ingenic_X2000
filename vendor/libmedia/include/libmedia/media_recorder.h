#ifndef __MEDIA_RECORDER_H__
#define __MEDIA_RECORDER_H__

#include <libmedia/media_muxing.h>
#include <libmedia/media_previewer.h>
#include <libmedia/audio_encoder.h>
#include <libmedia/video_encoder.h>

struct media_recorder;

struct media_recorder_param {
    struct media_muxing_param *muxing_param;
    struct media_previewer_param *previewer_param;
    struct audio_encoder_param *audio_encoder_param;
    struct video_encoder_param *video_encoder_param;
};

struct media_recorder *media_recorder_open(struct media_recorder_param *param);
void media_recorder_close(struct media_recorder *recorder);

int media_recorder_preview_one_frame(struct media_recorder *recorder);
int media_recorder_previewer_and_encode_one_frame(struct media_recorder *recorder);

void media_recorder_stop(struct media_recorder *recorder);
int media_recorder_start(struct media_recorder *recorder, const char *output_file);



#endif