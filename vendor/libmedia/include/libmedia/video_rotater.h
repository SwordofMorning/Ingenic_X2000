#ifndef _VIDEO_ROTATER_H_
#define _VIDEO_ROTATER_H_

#include <libmedia/video_frame.h>
#include <libmedia/media_alloter.h>

struct video_rotater;
struct video_rotater_param;

enum video_rotate_angle {
    rotate_0,
    rotate_90,
    rotate_180,
    rotate_270,
};

struct video_rotater_cb {
    struct video_rotater *(*open_rotater)(struct video_rotater_param *param);
    void (*close_rotater)(struct video_rotater *rotater);
    int (*convert_video_frame)(struct video_rotater *rotater, struct video_frame *src_frame, struct video_frame **dst_frame);
};

struct video_rotater_param {
    enum video_rotate_angle rotate_angle;
    int hflip;
    int vflip;
    struct video_rotater_cb *cb;
};

struct video_rotater {
    struct video_rotater_param param;
};

struct video_rotater *video_rotater_open(struct video_rotater_param *param);
void video_rotater_close(struct video_rotater *rotater);
int video_rotater_convert_video_frame(struct video_rotater *rotater, struct video_frame *src_frame, struct video_frame **dst_frame);
void video_rotater_set_rotate(struct video_rotater *rotater, enum video_rotate_angle angle, int hflip, int vflip);

#endif