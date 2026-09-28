#include <libmedia/play/alsa_audio_player.h>

struct alsa_audio_player {
    struct audio_player player;
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

struct audio_player *alsa_audio_player_open(struct audio_player_param *param)
{
    struct alsa_audio_player_param *p = (void *)param;
    struct alsa_params *alsa_param = &p->alsa_params;

    int sample_fmt = to_sample_fmt(alsa_param);
    if (sample_fmt == -1)
        return NULL;


    struct alsa_pcm *alsa = NULL;
    alsa = alsa_pcm_open_playback_device(p->alsa_playback_device);
    if (!alsa) {
        fprintf(stderr, "alsa_audio_player: failed to open audio device\n");
        return NULL;
    }

    int ret = alsa_pcm_set_params(alsa, alsa_param);
    if (ret < 0) {
        fprintf(stderr, "alsa_audio_player: failed to set audio params\n");
        goto close_alsa;
    }

    struct alsa_audio_player *player = malloc(sizeof(*player));
    assert(player);

    player->alsa = alsa;

    return &player->player;


close_alsa:
    alsa_pcm_close(alsa);
    return NULL;

}

void alsa_audio_player_close(struct audio_player *player)
{
    struct alsa_audio_player *alsa = (void *)player;

    alsa_pcm_drain(alsa->alsa);
    alsa_pcm_close(alsa->alsa);

    free(alsa);
}


int alsa_audio_player_display_audio(struct audio_player *player, struct audio_frame *frame)
{
    struct alsa_audio_player *alsa = (void *)player;

    if (frame->channels != player->param.channels ||
        frame->sample_rate != player->param.rate ||
        frame->format != player->param.format) {

        fprintf(stderr, "alsa audio player: failed to playback this frame, channels = %d,sample_rate = %d,format = %d\n",
                         frame->channels, frame->sample_rate, frame->format);
        return -1;
    }

    return alsa_pcm_write(alsa->alsa, frame->data[0], frame->nb_samples);
}

struct audio_player_cb alsa_audio_player_cb = {
    .open_player = alsa_audio_player_open,
    .close_player = alsa_audio_player_close,
    .display_audio = alsa_audio_player_display_audio,
};


void alsa_audio_player_init_param(struct alsa_audio_player_param *alsa_param)
{
    struct audio_player_param *param = &alsa_param->param;
    param->rate = alsa_param->alsa_params.rate;
    param->channels = alsa_param->alsa_params.channels;
    param->format = to_sample_fmt(&alsa_param->alsa_params);

    param->cb = &alsa_audio_player_cb;
}

void alsa_audio_player_init_default_param(
    struct alsa_audio_player_param *param, int channles, int rate)
{
    memset(param, 0, sizeof(*param));

    if (!access("/dev/snd/pcmC1D0p", F_OK))
        param->alsa_playback_device = "plughw:1,0";
    else
        param->alsa_playback_device = "plughw:0,0";

    param->alsa_params.channels = channles;
    param->alsa_params.rate = rate;
    param->alsa_params.format = SND_PCM_FORMAT_FLOAT_LE;

    alsa_audio_player_init_param(param);
}