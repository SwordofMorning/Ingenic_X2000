#ifndef __HW_H264_ENCODER_H__
#define __HW_H264_ENCODER_H__

#include <libmedia/video_encoder.h>

#include <libhardware2/v4l2_h264_encode.h>

struct hw_h264_video_encoder_param {
    struct video_encoder_param param;
    int framerate;
    struct v4l2_h264_encoder_config config;
};

void hw_h264_video_encoder_init_param(struct hw_h264_video_encoder_param *param);

void hw_h264_encoder_init_default_param(
    struct hw_h264_video_encoder_param *param, int width, int height);

#endif