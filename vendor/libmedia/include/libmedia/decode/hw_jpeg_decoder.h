#ifndef __HW_JPEG_DECODER_H__
#define __HW_JPEG_DECODER_H__

#include <libmedia/video_decoder.h>
#include <libhardware2/v4l2_jpeg_decode.h>

struct hw_jpeg_video_decoder_param {
    struct video_decoder_param param;

    struct v4l2_jpeg_decoder_config config;
};

void hw_jpeg_video_decoder_init_param(struct hw_jpeg_video_decoder_param *decoder_param);

void hw_jpeg_decoder_init_default_param(
    struct hw_jpeg_video_decoder_param *decoder_param, int width, int height);

#endif /* __HW_JPEG_DECODER_H__ */