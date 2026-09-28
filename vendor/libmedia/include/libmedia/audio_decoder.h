#ifndef __AUDIO_DECODER_H__
#define __AUDIO_DECODER_H__

#include <libmedia/audio_frame.h>
#include <libmedia/media_packet.h>
#include <libmedia/media_errno.h>


struct audio_decoder;
struct audio_decoder_param;

struct audio_decoder_cb {
    struct audio_decoder *(*open_decoder)(struct audio_decoder_param *param);
    void (*close_decoder)(struct audio_decoder *decoder);
    int (*send_pkt)(struct audio_decoder *decoder, struct media_packet *pkt);
    int (*get_frame)(struct audio_decoder *decoder, struct audio_frame **frame);
};

struct audio_decoder_param {
    char *codec_name;

    int bit_rate;

    int rate;
    int channels;

    enum audio_frame_format  fmt;
    int channel_layout;

    uint8_t *extradata;
    int extradata_size;

    struct audio_decoder_cb *cb;
};

struct audio_decoder{
    struct audio_decoder_param param;
};

struct audio_decoder *audio_decoder_open(struct audio_decoder_param *param);
void audio_decoder_close(struct audio_decoder *decoder);
int audio_decoder_send_pkt(struct audio_decoder *decoder, struct media_packet *pkt);
int audio_decoder_get_frame(struct audio_decoder *decoder, struct audio_frame **frame);


#endif