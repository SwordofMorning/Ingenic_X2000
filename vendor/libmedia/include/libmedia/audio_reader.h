#ifndef _AUDIO_READER_H_
#define _AUDIO_READER_H_

#include <libmedia/audio_frame.h>
#include <libmedia/audio_resampler.h>

struct audio_reader;
struct audio_reader_param;

struct audio_reader_cb {
    struct audio_reader *(*open_reader)(struct audio_reader_param *param);
    void (*close_reader)(struct audio_reader *reader);
    int (*read_audio)(struct audio_reader *reader, struct audio_frame **frame);
    int (*drop_audio)(struct audio_reader *reader);
    int (*avail_audio)(struct audio_reader *reader);
};

struct audio_reader_param {
    int is_enable;
    enum audio_frame_format format;
    int rate;
    int channels;
    int samples;
    struct audio_reader_cb *cb;
    struct audio_resampler_param *resampler_param;
};

struct audio_reader {
    struct audio_reader_param param;
    struct audio_resampler *resampler;
    int frame_bytes;
};

struct audio_reader *audio_reader_open(struct audio_reader_param *param);

void audio_reader_close(struct audio_reader *reader);

int audio_reader_read_frame(struct audio_reader *reader, struct audio_frame **frame);

int audio_reader_drop_audio(struct audio_reader *reader);

int audio_reader_avail_audio(struct audio_reader *reader);

#endif /* _AUDIO_READER_H_ */
