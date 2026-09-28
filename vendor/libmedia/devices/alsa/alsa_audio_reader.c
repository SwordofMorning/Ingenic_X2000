#include <stdlib.h>
#include <libhardware2/alsa.h>
#include <libmedia/read/alsa_audio_reader.h>

#define DEFAULT_READ_SAMPLES       1024

struct alsa_audio_reader {
    struct audio_reader reader;
    struct alsa_pcm *alsa;
};

static int to_sample_fmt(struct alsa_params *alsa_param)
{
    switch (alsa_param->format) {
    case SND_PCM_FORMAT_S16_LE:     return AUDIO_s16le;
    case SND_PCM_FORMAT_S32_LE:     return AUDIO_s32le;
    case SND_PCM_FORMAT_FLOAT_LE:   return AUDIO_flt;
    default:
        fprintf(stderr, "alsa_audio_reader: not support this alsa fmt: %d\n",
         alsa_param->format);
        return -1;
    }
}

struct audio_reader *alsa_audio_reader_open(struct audio_reader_param *param)
{
    struct alsa_audio_reader_param *p = (void *)param;
    struct alsa_params *alsa_param = &p->alsa_params;

    int sample_fmt = to_sample_fmt(alsa_param);
    if (sample_fmt == -1)
        return NULL;

    struct alsa_pcm *alsa = NULL;
    alsa = alsa_pcm_open_capture_device(p->alsa_capture_device);
    if (!alsa) {
        fprintf(stderr, "alsa_audio_reader: failed to open audio device\n");
        return NULL;
    }

    int ret = alsa_pcm_set_params(alsa, alsa_param);
    if (ret < 0) {
        fprintf(stderr, "alsa_audio_reader: failed to set audio params\n");
        goto close_alsa;
    }

    struct alsa_audio_reader *reader = malloc(sizeof(*reader));
    assert(reader);

    reader->alsa = alsa;
    reader->reader.frame_bytes = alsa_param->frame_bytes;

    return &reader->reader;

close_alsa:
    alsa_pcm_close(alsa);
    return NULL;
}

void alsa_audio_reader_close(struct audio_reader *reader)
{
    struct alsa_audio_reader *alsa = (void *)reader;

    alsa_pcm_close(alsa->alsa);

    free(alsa);
}

int alsa_audio_reader_read_audio(struct audio_reader *reader, struct audio_frame **frame)
{
    int ret;
    struct alsa_audio_reader *alsa = (void *)reader;

    int read_samples = 0;
    if (reader->param.samples)
        read_samples = reader->param.samples;
    else
        read_samples = DEFAULT_READ_SAMPLES;

    struct audio_frame *audio_frame = audio_frame_alloc_with_buffer(reader->param.rate, read_samples,
                                                                    reader->param.channels, reader->param.format);

    ret = alsa_pcm_read(alsa->alsa, audio_frame->data[0], read_samples);
    if(ret < 0) {
        fprintf(stderr, "alsa audio reader:failed to read pcm\n");
        audio_frame_put(audio_frame);
        return ret;
    }

    *frame = audio_frame;

    return 0;
}

int alsa_audio_reader_drop_audio(struct audio_reader *reader)
{
    struct alsa_audio_reader *alsa = (void *)reader;

    return alsa_pcm_drop(alsa->alsa);
}

int alsa_audio_reader_avail_audio(struct audio_reader *reader)
{
    struct alsa_audio_reader *alsa = (void *)reader;

    return alsa_pcm_avil(alsa->alsa);
}


struct audio_reader_cb alsa_audio_reader_cb = {
    .open_reader = alsa_audio_reader_open,
    .close_reader = alsa_audio_reader_close,
    .read_audio = alsa_audio_reader_read_audio,
    .drop_audio = alsa_audio_reader_drop_audio,
    .avail_audio = alsa_audio_reader_avail_audio,
};

void alsa_audio_reader_init_param(
    struct alsa_audio_reader_param *alsa_param)
{
    struct audio_reader_param *param = &alsa_param->param;

    param->rate = alsa_param->alsa_params.rate;
    param->channels = alsa_param->alsa_params.channels;
    param->format = to_sample_fmt(&alsa_param->alsa_params);

    param->cb = &alsa_audio_reader_cb;
}

void alsa_audio_reader_init_default_param(
    struct alsa_audio_reader_param *param, int channles, int rate)
{
    memset(param, 0, sizeof(*param));

    param->alsa_capture_device = "plughw:0,0";
    param->alsa_params.channels = channles;
    param->alsa_params.rate = rate;
    param->alsa_params.format = SND_PCM_FORMAT_S16_LE;
    param->param.resampler_param = NULL;
    param->param.samples = 0;

    alsa_audio_reader_init_param(param);
}