#ifndef _MEDIA_MUXING_H_
#define _MEDIA_MUXING_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <libmedia/video_frame.h>
#include <libmedia/audio_frame.h>
#include <libmedia/media_alloter.h>
#include <libmedia/media_packet.h>

struct media_muxing;
struct media_muxing_param;

struct media_muxing_cb {
    struct media_muxing *(*open_muxing)(struct media_muxing_param *param);
    void (*close_muxing)(struct media_muxing *muxing);
    int (*muxing_packet)(struct media_muxing *muxing, struct media_packet *pkt);
};


struct media_muxing_video_param {
    int enable;
    int bit_rate;
    enum video_frame_format fmt;
    enum media_packet_type type;
    int width;
    int height;
    int framerate;
    int gop_size;

    uint8_t *extradata;
    int extradata_size;
};

struct media_muxing_audio_param {
    int enable;
    int bit_rate;
    int sample_rate;
    int channels;
    enum audio_frame_format fmt;
    enum media_packet_type type;
};

struct media_muxing_param {
    const char *output_file;

    struct media_muxing_video_param video_param;
    struct media_muxing_audio_param audio_param;

    struct media_muxing_cb *cb;
};

struct media_muxing {
    struct media_muxing_param param;
};

struct media_muxing *media_muxing_open(struct media_muxing_param *param);

void media_muxing_close(struct media_muxing *muxing);

int media_muxing_one_pkt(struct media_muxing *muxing, struct media_packet *pkt);

#endif