#ifndef __VIDEO_ENCODER_H__
#define __VIDEO_ENCODER_H__

#include <libmedia/media_muxing.h>
#include <libmedia/media_packet.h>
#include <libmedia/video_frame.h>
#include <libmedia/media_errno.h>

struct video_encoder;
struct video_encoder_param;

struct video_encoder_cb {
    struct video_encoder *(*open_encoder)(struct video_encoder_param *param);
    void (*set_next_keyframe)(struct video_encoder *encoder);
    int (*write_frame)(struct video_encoder *encoder, struct video_frame *frame);
    int (*get_pkt)(struct video_encoder *encoder, struct media_packet **pkt);
    void (*init_muxing_param)(struct video_encoder *encoder, struct media_muxing_video_param *param);
    void (*close_encoder)(struct video_encoder *encoder);
};

struct video_encoder_param {
    const char *encoder_name;
    struct video_encoder_cb *cb;
};

struct video_encoder {
    struct video_encoder_param param;

};

struct video_encoder *video_encoder_open(struct video_encoder_param *param);
void video_encoder_close(struct video_encoder *encoder);
void video_encoder_set_next_keyframe(struct video_encoder *encoder);
int video_encoder_write_frame(struct video_encoder *encoder, struct video_frame *frame);
int video_encoder_get_packet(struct video_encoder *encoder, struct media_packet **pkt);
void video_encoder_init_muxing_param(struct video_encoder *encoder, struct media_muxing_video_param *param);

#endif