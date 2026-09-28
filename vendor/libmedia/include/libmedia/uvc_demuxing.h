#include <stdio.h>
#include <assert.h>

#include <libmedia/media_demuxing.h>
#include <libhardware2/v4l2_camera.h>

struct uvc_demuxing_param {
    struct media_demuxing_param param;
    /**
     * uvc 设备节点,如 "/dev/video4"
     */
    const char *uvc_device;
    /**
     * uvc 对应的参数
     */
    struct v4l2_camera_format uvc_format;
};

void uvc_demuxing_init_param(struct media_demuxing_param *param);