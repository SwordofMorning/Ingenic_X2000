#ifndef _FFMPEG_VIDEO_FORMATTER_H_
#define _FFMPEG_VIDEO_FORMATTER_H_

#include <libmedia/media_muxing.h>

#include <libavformat/avformat.h>

void ffmpeg_muxing_init_param(struct media_muxing_param *param);

#endif