#ifndef __VIC_VIDEO_READER_H__
#define __VIC_VIDEO_READER_H__


#include <libmedia/video_reader.h>

struct vic_video_reader_param {
    struct video_reader_param param;

    /**
     * vic 设备节点,如 "/dev/vic0"
     */
    const char *vic_device;
};

void vic_video_reader_init_param(struct vic_video_reader_param *vic_param);

void vic_video_reader_init_default_param(struct vic_video_reader_param *param);

#endif