#ifndef __MEDIA_PACKET_H__
#define __MEDIA_PACKET_H__

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

enum media_packet_type{
    AUDIO_pkt_aac,

    VIDEO_pkt_h264 = 1000,
    VIDEO_pkt_mjpeg,
    VIDEO_pkt_png,
};

enum media_packet_pdata_type {
    MEDIA_PACKET_PDATA_NULL,
    MEDIA_PACKET_PDATA_FFMPEG,
};

struct media_packet {
    uint8_t *data;
    int size;
    int flags;

    enum media_packet_type type;

    int ref_cnt;

    void *pdata;
    enum media_packet_pdata_type pdata_type;

    void *handle;
    void (*put_packet)(void *handle, struct media_packet *pkt);

    int stream_index;
    int64_t duration;
    int pos;
    int64_t pts;
    int is_key_frame;
};

struct media_packet *media_packet_alloc(void);
void media_packet_free(struct media_packet *packet);
void media_packet_get(struct media_packet *packet);
void media_packet_put(struct media_packet *packet);

#endif