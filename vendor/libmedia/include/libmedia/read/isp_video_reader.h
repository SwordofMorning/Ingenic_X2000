#ifndef _ISP_VIDEO_READER_H_
#define _ISP_VIDEO_READER_H_

#include <libisp/isp.h>
#include <libmedia/video_reader.h>

struct isp_video_reader_param {
    struct video_reader_param param;

    /**
     * isp 设备节点,如 "/dev/mscaler1-ch0"
     */
    const char *isp_device;

    /**
     * isp 对应的参数
     */
    struct frame_image_format isp_params;
};

void isp_video_reader_init_param(struct isp_video_reader_param *isp_param);

void isp_video_reader_init_default_param(struct isp_video_reader_param *param, int width, int height);

#endif /* _ISP_VIDEO_READER_H_ */
