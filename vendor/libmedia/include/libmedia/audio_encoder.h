#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__

#include <libmedia/media_muxing.h>
#include <libmedia/audio_frame.h>
#include <libmedia/media_packet.h>
#include <libmedia/media_errno.h>

struct audio_encoder;
struct audio_encoder_param;

struct audio_encoder_cb {
    struct audio_encoder *(*open_encoder)(struct audio_encoder_param *param);
    void (*close_encoder)(struct audio_encoder *encoder);
    int (*write_frame)(struct audio_encoder *encoder, struct audio_frame *frame);
    int (*get_packet)(struct audio_encoder *encoder, struct media_packet **pkt);
    void (*init_muxing_param)(struct audio_encoder *encoder, struct media_muxing_audio_param *param);
};

struct audio_encoder_param {
    struct audio_encoder_cb *cb;
};

struct audio_encoder{
    struct audio_encoder_param param;
};

struct audio_encoder *audio_encoder_open(struct audio_encoder_param *param);
void audio_encoder_close(struct audio_encoder *encoder);
int audio_encoder_write_frame(struct audio_encoder *encoder, struct audio_frame *frame);
int audio_encoder_get_packet(struct audio_encoder *encoder, struct media_packet **pkt);
void audio_encoder_init_muxing_param(struct audio_encoder *encoder, struct media_muxing_audio_param *param);

#endif