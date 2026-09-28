#ifndef __FFMPEG_VIDEO_ENCODER_H__
#define __FFMPEG_VIDEO_ENCODER_H__

#include <libmedia/video_encoder.h>
#include <libavcodec/avcodec.h>

struct ffmpeg_video_encoder_param {
    struct video_encoder_param param;

    const char *encoder_name;
    int framerate;
    int bit_rate;
    int width;
    int height;
    int gop_size;
    int profile;
    enum AVPixelFormat pix_fmt;
};

void ffmpeg_video_encoder_init_param(struct ffmpeg_video_encoder_param *ffmpeg_param);

void ffmpeg_h264_encoder_init_default_param(
    struct ffmpeg_video_encoder_param *param, int width, int height);

#endif