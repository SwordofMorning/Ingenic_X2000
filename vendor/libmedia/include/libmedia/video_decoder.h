#ifndef _VIDEO_DECODER_H_
#define _VIDEO_DECODER_H_

#include <libmedia/video_frame.h>
#include <libmedia/media_packet.h>
#include <libmedia/media_errno.h>

struct video_decoder;
struct video_decoder_param;

struct media_packet;

struct video_decoder_cb {
    struct video_decoder *(*open_decoder)(struct video_decoder_param *param);
    void (*close_decoder)(struct video_decoder *decoder);
    int (*send_pkt)(struct video_decoder *decoder, struct media_packet *pkt);
    int (*get_frame)(struct video_decoder *decoder, struct video_frame **dst_frame);
};

struct video_decoder_param {
    int width;
    int height;

    char *codec_name;

    int bit_rate;

    enum video_frame_format fmt;
    uint8_t *extradata;
    int extradata_size;

    struct video_decoder_cb *cb;
    int fps_num;
    int fps_den;
    int timebase_num;
    int timebase_den;
};

struct video_decoder {
    struct video_decoder_param param;
};

struct video_decoder *video_decoder_open(struct video_decoder_param *param);

void video_decoder_close(struct video_decoder *decoder);

int video_decoder_send_pkt(struct video_decoder *decoder, struct media_packet *pkt);

int video_decoder_get_frame(struct video_decoder *decoder, struct video_frame **dst_frame);

#endif