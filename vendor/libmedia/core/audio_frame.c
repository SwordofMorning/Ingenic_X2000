#include <libmedia/audio_frame.h>
#include <stdlib.h>
#include <string.h>

static const char *fmt_names[] = {
    [AUDIO_s16le] = "s16le",
    [AUDIO_s32le] = "s32le",
    [AUDIO_flt] = "flt",
    [AUDIO_s16lep] = "s16lep",
    [AUDIO_s32lep] = "s32lep",
    [AUDIO_fltp] = "fltp",
};

const char *audio_fmt_name(enum audio_frame_format fmt)
{
    return fmt_names[fmt] ? fmt_names[fmt] : "err-audio-fmt";
}

struct audio_frame* audio_frame_alloc(void)
{
    struct audio_frame *frame = malloc(sizeof(*frame));
    if(!frame) {
        fprintf(stderr, "failed to alloc audio frame\n");
        return NULL;
    }

    memset(frame, 0 , sizeof(*frame));

    return frame;
}

void audio_frame_free(struct audio_frame *frame)
{
    if(!frame)
        return;

    assert(!frame->user_cnt);
    free(frame);
}

void audio_frame_put(struct audio_frame *frame)
{
    if(!frame)
        return;

    frame->user_cnt--;

    assert(frame->user_cnt >= 0);

    if (!frame->user_cnt && frame->put_frame)
        frame->put_frame(frame->handle, frame);
}

void audio_frame_get(struct audio_frame *frame)
{
    frame->user_cnt++;
}


int audio_frame_bytes_per_sample(enum audio_frame_format format)
{
    switch (format)
    {
        case AUDIO_s16lep:
        case AUDIO_s16le:
            return 2;

        case AUDIO_flt:
        case AUDIO_fltp:
        case AUDIO_s32lep:
        case AUDIO_s32le:
            return 4;
        default:
            fprintf(stderr, "unknown audio format\n");
            return 0;
    }

}

static int audio_data_is_planar(enum audio_frame_format format)
{
    if (format >= AUDIO_s16lep)
        return 1;

    return 0;
}

static void audio_frame_free_frame(void *handle, struct audio_frame *frame)
{
    free(frame->data[0]);

    audio_frame_free(frame);
}


struct audio_frame *audio_frame_alloc_with_buffer(int sample_rate, int nb_samples,
                                                  int channels, enum audio_frame_format format)
{

    struct audio_frame *frame = audio_frame_alloc();
    assert(frame);

    int bytes_per_sample = audio_frame_bytes_per_sample(format);
    int is_planar = audio_data_is_planar(format);

    int i;

    frame->total_size = nb_samples *channels * bytes_per_sample;
    void *data = malloc(nb_samples * channels * bytes_per_sample);

    if (is_planar) {
        for(i = 0; i < channels; i++) {
            frame->data[i] = data + nb_samples * bytes_per_sample * i;
            frame->size[i] = nb_samples * bytes_per_sample;
        }
    } else {
        frame->data[0] = data;
        frame->size[0] = frame->total_size;
    }

    frame->sample_rate = sample_rate;
    frame->format = format;
    frame->nb_samples = nb_samples;
    frame->channels = channels;

    frame->put_frame = audio_frame_free_frame;
    frame->handle = NULL;

    audio_frame_get(frame);

    return frame;
}