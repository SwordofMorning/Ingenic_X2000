#ifndef __VIDEO_OVERLAYER_H__
#define __VIDEO_OVERLAYER_H__

#include <libmedia/media_muxing.h>
#include <libmedia/media_packet.h>
#include <libmedia/video_frame.h>
#include <libmedia/media_errno.h>

struct video_overlayer;
struct video_overlayer_param;

struct video_overlayer_cb {
    struct video_overlayer *(*open_overlayer)(struct video_overlayer_param *bg_param, struct video_overlayer_param *fg_param, int fg_len);
    int (*send_bg_frame)(struct video_overlayer *overlayer, struct video_frame *frame);
    int (*send_fg_frame)(struct video_overlayer *overlayer, struct video_frame *frame, int index);
    int (*get_frame)(struct video_overlayer *overlayer, struct video_frame **frame);
    void (*close_overlayer)(struct video_overlayer *overlayer);
};

struct video_overlayer_layer_info {
    enum video_frame_format format;
    int xpos;
    int ypos;
    int width;
    int height;
};

struct video_overlayer_param {
    struct video_overlayer_cb *cb;

    struct video_frame *frame;
    struct video_overlayer_layer_info info;
};

struct video_overlayer {
    struct video_overlayer_param param;
};

struct video_overlayer *video_overlayer_open(struct video_overlayer_param *bg_param, struct video_overlayer_param *fg_param, int fg_len);
void video_overlayer_close(struct video_overlayer *overlayer);
int video_overlayer_send_bg_frame(struct video_overlayer *overlayer, struct video_frame *frame);
int video_overlayer_send_fg_frame(struct video_overlayer *overlayer, struct video_frame *frame, int index);
int video_overlayer_get_frame(struct video_overlayer *overlayer, struct video_frame **frame);

#endif