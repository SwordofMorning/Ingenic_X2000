#ifndef __FFMEPG_AUDIO_RESAMPLER_H__
#define __FFMPEG_AUDIO_RESAMPLER_H__

#include <libmedia/audio_resampler.h>
#include <libavutil/pixfmt.h>
#include <stdint.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

void ffmpeg_audio_resampler_init_param(struct audio_resampler_param *param);

void ffmpeg_audio_resample_init_default_param(
    struct audio_resampler_param *param, int channels, int rate, 
    enum audio_frame_format src_fmt, enum audio_frame_format dst_fmt);

#endif