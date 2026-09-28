#ifndef _FFMPEG_VIDEO_DECODER_H_
#define _FFMPEG_VIDEO_DECODER_H_

#include <libmedia/video_decoder.h>
#include <libmedia/utils/ffmpeg_utils.h>

struct ffmpeg_video_decoder_param {
    struct video_decoder_param param;
    const char *decoder_name;
};

void ffmpeg_video_decoder_init_param(struct ffmpeg_video_decoder_param *param);

#endif