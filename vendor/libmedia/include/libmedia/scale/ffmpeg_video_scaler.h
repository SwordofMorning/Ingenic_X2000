#ifndef __FFMEPG_VIDEO_SCALER_H__
#define __FFMPEG_VIDEO_SCALER_H__

#include <libmedia/video_scaler.h>
#include <libavutil/pixfmt.h>
#include <stdint.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

struct ffmpeg_video_scaler_param {
    struct video_scaler_param param;

    int width;
    int height;
    enum video_frame_format fmt;
};

void ffmpeg_video_scaler_init_param(struct ffmpeg_video_scaler_param *scaler_param);

#endif