#ifndef __AUDIO_FRAME_H__
#define __AUDIO_FRAME_H__

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define MAX_AUDIO_FRAME_BUF_CNT  8

enum audio_frame_format {
    AUDIO_s16le,
    AUDIO_s32le,
    AUDIO_flt,

    AUDIO_s16lep,
    AUDIO_s32lep,
    AUDIO_fltp,
};

struct audio_frame {
    enum audio_frame_format format;

    uint8_t *data[MAX_AUDIO_FRAME_BUF_CNT];
    uint32_t size[MAX_AUDIO_FRAME_BUF_CNT];

    uint32_t total_size;

    void *pdata;
    int user_cnt;

    void *handle;
    void (*put_frame)(void *handle, struct audio_frame *frame);

    int nb_samples;
    int sample_rate;
    int channels;
};

const char *audio_fmt_name(enum audio_frame_format fmt);

struct audio_frame* audio_frame_alloc(void);
void audio_frame_free(struct audio_frame *frame);
void audio_frame_get(struct audio_frame *frame);
void audio_frame_put(struct audio_frame *frame);
int audio_frame_bytes_per_sample(enum audio_frame_format format);
struct audio_frame *audio_frame_alloc_with_buffer(int sample_rate, int nb_samples,
                                                  int channels, enum audio_frame_format format);

#endif