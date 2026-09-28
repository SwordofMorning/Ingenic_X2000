#ifndef __AUDIO_RESAMPLER_H__
#define __AUDIO_RESAMPLER_H__

#include <libmedia/audio_frame.h>
#include <libmedia/media_errno.h>

struct audio_resampler;
struct audio_resampler_param;

struct audio_resampler_cb {
    struct audio_resampler *(*create_resampler)(struct audio_resampler_param *param);
    int (*convert_audio)(struct audio_resampler *resampler, struct audio_frame *frame_in, struct audio_frame **frame_out);
    void (*delete_resampler)(struct audio_resampler *resampler);
};

struct audio_resampler_param {
    enum audio_frame_format src_format;
    int src_rate;
    int src_channels;

    enum audio_frame_format dst_format;
    int dst_rate;
    int dst_channels;

    struct audio_resampler_cb *cb;
};

struct audio_resampler  {
    struct audio_resampler_param param;
};


struct audio_resampler *audio_resampler_create(struct audio_resampler_param *param);
int audio_resampler_convert(struct audio_resampler *resampler, struct audio_frame *frame_in, struct audio_frame **frame_out);
void audio_resampler_delete(struct audio_resampler *resampler);

#endif