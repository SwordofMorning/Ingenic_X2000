#ifndef _TURBO_JPEG_VIDEO_DECODER_H_
#define _TURBO_JPEG_VIDEO_DECODER_H_

#include <libmedia/video_decoder.h>

struct turbo_jpeg_video_decoder_param {
    struct video_decoder_param param;
};

void turbo_jpeg_video_decoder_init_param(
    struct turbo_jpeg_video_decoder_param *param);

#endif /* _TURBO_JPEG_VIDEO_DECODER_H_ */
