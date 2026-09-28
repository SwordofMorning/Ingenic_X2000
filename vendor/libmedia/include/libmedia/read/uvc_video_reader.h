#ifndef _UVC_VIDEO_READER_H_
#define _UVC_VIDEO_READER_H_

#include <libhardware2/v4l2_camera.h>
#include <libmedia/video_reader.h>


struct uvc_video_reader_param {
    struct video_reader_param param;
    /**
     * uvc 设备节点,如 "/dev/video4"
     */
    const char *uvc_device;
    /**
     * uvc 对应的参数
     */
    struct v4l2_camera_format uvc_format;
    /**
     * uvc 使用的解码器参数
     * 若摄像头的出图格式为压缩格式，则需要配置相应的解码器，否则取图会失败;
     * 若想拿到原始压缩格式图像，可参考uvc_demuxing.h 或 main_uvc_jpeg_to_nv12_test.c
     */
    struct video_decoder_param *decoder_param;
};

void uvc_video_reader_init_param(struct uvc_video_reader_param *uvc_param);

void uvc_video_reader_init_default_param(struct uvc_video_reader_param *param, int width, int height);

#endif /* _UVC_VIDEO_READER_H_ */