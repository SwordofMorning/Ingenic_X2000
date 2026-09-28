#ifndef __FFMPEG_AUDIO_ENCODER_H__
#define __FFMPEG_AUDIO_ENCODER_H__

#include <libmedia/audio_encoder.h>
#include <libavcodec/avcodec.h>

struct ffmpeg_audio_encoder_param {
    struct audio_encoder_param param;

    const char *encoder_name;
    int bit_rate;
    int sample_rate;
    int channels;
    enum AVSampleFormat sample_fmt;
};

void ffmpeg_audio_encoder_init_param(struct ffmpeg_audio_encoder_param *ffmpeg_param);

void ffmpeg_audio_encoder_init_default_param(
    struct ffmpeg_audio_encoder_param *param, int channels, int rate);

#endif