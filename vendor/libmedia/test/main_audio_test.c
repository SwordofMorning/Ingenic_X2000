#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <time.h>
#include <stdint.h>
#include <unistd.h>

#include <libmedia/audio_reader.h>
#include <libmedia/read/alsa_audio_reader.h>

#include <libmedia/audio_player.h>
#include <libmedia/play/alsa_audio_player.h>

#include <libmedia/resample/ffmpeg_audio_resampler.h>
#include <libmedia/audio_resampler.h>

int main(void)
{
    struct audio_resampler_param resampler_param = {
        .src_channels = 2,
        .src_format = AUDIO_s16le,
        .src_rate = 48000,

        .dst_channels = 2,
        .dst_format = AUDIO_flt,
        .dst_rate = 48000,
    };

    ffmpeg_audio_resampler_init_param(&resampler_param);

    struct alsa_audio_reader_param alsa_reader_param = {
        .alsa_capture_device = "plughw:0,0",
        .alsa_params = {
            .rate = 48000,
            .format = SND_PCM_FORMAT_S16_LE,
            .channels = 2,
        },
        .param.resampler_param = &resampler_param,
    };


    alsa_audio_reader_init_param(&alsa_reader_param);
    struct audio_reader *reader = audio_reader_open(&alsa_reader_param.param);
    if (!reader)
        return -1;


    struct alsa_audio_player_param alsa_player_param = {
        .alsa_playback_device = "plughw:1,0",
        .alsa_params = {
            .rate = 48000,
            .format = SND_PCM_FORMAT_FLOAT_LE,
            .channels = 2,
        },
    };

    alsa_audio_player_init_param(&alsa_player_param);
    struct audio_player *player = audio_player_open(&alsa_player_param.param);
    if(!player)
        return -1;

    struct audio_frame *audio_frame = NULL;

    FILE *file1 = fopen("/usr/data/1.pcm", "w");

    int cnt = 500;
    int ret;


    while(cnt--) {
        ret = audio_reader_read_frame(reader, &audio_frame);

        if(ret < 0)
            return -1;

        fwrite(audio_frame->data[0], 1, audio_frame->total_size, file1);

        audio_player_display_audio(player, audio_frame);

        audio_frame_put(audio_frame);
    }

    fclose(file1);

}