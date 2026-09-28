#ifndef _MEDIA_DEMUXING_H_
#define _MEDIA_DEMUXING_H_

#include <libmedia/video_frame.h>
#include <libmedia/media_alloter.h>
#include <libmedia/media_packet.h>

#include <libmedia/video_decoder.h>
#include <libmedia/audio_decoder.h>
#include <libmedia/media_errno.h>

struct media_demuxing;
struct media_demuxing_param;
struct media_metadata;

struct media_demuxing_cb {
    struct media_demuxing *(*open_demuxing)(struct media_demuxing_param *param);
    void (*close_demuxing)(struct media_demuxing *demuxing);
    int (*get_video_param)(struct media_demuxing *demuxing, struct video_decoder_param *param);
    int (*get_audio_param)(struct media_demuxing *demuxing, struct audio_decoder_param *param);
    int (*demuxing_packet)(struct media_demuxing *demuxing, struct media_packet **pkt);
    uint64_t (*get_pts)(struct media_demuxing *media_demuxing, struct media_packet *pkt);
    uint64_t (*get_duration)(struct media_demuxing *media_demuxing);
    int (*get_metadata)(struct media_demuxing *demuxing, struct media_metadata *metadata);
    int (*seek_forward)(struct media_demuxing *media_demuxing, int64_t abs_us);
    int (*seek_backward)(struct media_demuxing *media_demuxing, int64_t abs_us);
};

struct media_metadata {
    char *key;
    void *value;
};

struct media_demuxing_param {
    struct media_demuxing_cb *cb;
    const char *input_file;
    int is_server;                  /*仅对网络ip 地址有效，如tcp://xxx, rtsp://xxx, udp://xxx，目前只有ffmpeg demuxing 能使用ip地址*/
    struct video_decoder_param *decoder_param;
};

struct media_demuxing {
    struct media_demuxing_param param;
    struct video_decoder *decoder;
};

struct media_demuxing *demuxing_open(struct media_demuxing_param *param);

void demuxing_close(struct media_demuxing *demuxing);

int demuxing_get_video_param(struct media_demuxing *demuxing, struct video_decoder_param *param);

int demuxing_get_audio_param(struct media_demuxing *demuxing, struct audio_decoder_param *param);

int demuxing_one_pkt(struct media_demuxing *demuxing, struct media_packet **pkt);

uint64_t demuxing_get_pts(struct media_demuxing *media_demuxing, struct media_packet *pkt);

uint64_t demuxing_get_duration(struct media_demuxing *media_demuxing);

int demuxing_get_metadata(struct media_demuxing *demuxing, struct media_metadata *metadata);

int demuxing_seek_forward(struct media_demuxing *media_demuxing, int64_t abs_us);

int demuxing_seek_backward(struct media_demuxing *media_demuxing, int64_t abs_us);
#endif