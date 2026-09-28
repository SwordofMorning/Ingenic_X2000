#ifndef __VIDEO_SCALER_H__
#define __VIDEO_SCALER_H__

#include <libmedia/media_muxing.h>
#include <libmedia/media_packet.h>
#include <libmedia/video_frame.h>
#include <libmedia/media_errno.h>

struct video_scaler;
struct video_scaler_param;

struct video_scaler_cb {
    struct video_scaler *(*open_scaler)(struct video_scaler_param *param);
    int (*send_frame)(struct video_scaler *scaler, struct video_frame *frame);
    int (*get_frame)(struct video_scaler *scaler, struct video_frame **frame);
    void (*close_scaler)(struct video_scaler *scaler);
};

struct video_scaler_param {
    struct video_scaler_cb *cb;
    int dst_width;
    int dst_height;
};

struct video_scaler {
    struct video_scaler_param param;
};

struct video_scaler *video_scaler_open(struct video_scaler_param *param);
void video_scaler_close(struct video_scaler *scaler);
int video_scaler_send_frame(struct video_scaler *scaler, struct video_frame *frame);
int video_scaler_get_frame(struct video_scaler *scaler, struct video_frame **frame);

int video_frame_scale(struct video_scaler_param *param, struct video_frame *src_frame,
                      struct video_frame **dst_frame, int w, int h);

#endif