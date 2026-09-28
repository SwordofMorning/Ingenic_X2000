#ifndef __FFMPEG_AUDIO_DECODER_H__
#define __FFMPEG_AUDIO_DECODER_H__

#include <libavcodec/avcodec.h>
#include <libmedia/audio_decoder.h>


struct ffmpeg_audio_decoder_param {
    struct audio_decoder_param param;

    int channels;
    int sample_rate;
    const char *decoder_name;
};

void ffmpeg_audio_decoder_init_param(struct ffmpeg_audio_decoder_param *ffmpeg_param);


#endif