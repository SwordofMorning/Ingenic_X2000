#ifndef __HW_H264_DECODER_H__
#define __HW_H264_DECODER_H__

#include <libmedia/video_decoder.h>
#include <libhardware2/v4l2_h264_decode.h>

struct hw_h264_video_decoder_param {
    struct video_decoder_param param;

    struct v4l2_h264_decoder_config config;
};

void hw_h264_video_decoder_init_param(struct hw_h264_video_decoder_param *decoder_param);

void hw_h264_decoder_init_default_param(
    struct hw_h264_video_decoder_param *decoder_param, int width, int height);

#endif /* __HW_H264_DECODER_H__ */