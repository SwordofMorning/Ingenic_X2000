#ifndef __FFMEPG_VIDEO_OVERLAYER_H__
#define __FFMPEG_VIDEO_OVERLAYER_H__

#include <libmedia/video_overlayer.h>
#include <libavutil/pixfmt.h>
#include <stdint.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

void ffmpeg_video_overlayer_init_param(struct video_overlayer_param *overlayer_param);

#endif