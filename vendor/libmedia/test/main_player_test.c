#include <stdio.h>

#include <libmedia/media_demuxing.h>
#include <libmedia/media_player.h>
#include <libmedia/decode/ffmpeg_video_decoder.h>
#include <libmedia/decode/ffmpeg_audio_decoder.h>
#include <libmedia/resample/ffmpeg_audio_resampler.h>
#include <libmedia/play/fb_video_player.h>
#include <libmedia/play/alsa_audio_player.h>
#include <libmedia/play/async_video_player.h>
#include <libmedia/play/async_audio_player.h>
#include <libmedia/rotate/hw_video_rotater.h>
#include <time.h>
#include <libutils2/boot_time.h>

#include <unistd.h>

void ffmpeg_demuxing_init_param(struct media_demuxing_param *param);

int main(int argc, char *argv[])
{
    char *file_name = argv[1];
    if (!file_name) {
        fprintf(stderr, "No such file or directory !!!!!!!!\n");
        return -1;
    }

    char *audio_device = "/dev/snd/pcmC0D0p";
    if (!access("/dev/snd/pcmC1D0p", F_OK))
        audio_device = "/dev/snd/pcmC1D0p";

    struct media_demuxing_param demuxing_param;
    struct ffmpeg_video_decoder_param video_decoder_param;
    struct fb_video_player_param fb_param;
    struct async_video_player_param async_video_param;

    struct alsa_audio_player_param alsa_player_param;
    struct async_audio_player_param async_audio_param;
    struct audio_resampler_param resampler_param;
    struct ffmpeg_audio_decoder_param audio_decoder_param;

    struct media_player_param m_p_param;

    /*
     * 这里使用async video/audio player 的目的是方便在开机启动的时候提前解码
     * 解码的初始化还是有点费时间的,和等待设备时间重合起来节约时间
     */
    ffmpeg_demuxing_init_param(&demuxing_param);
    ffmpeg_video_decoder_init_param(&video_decoder_param);
    fb_video_player_init_default_param(&fb_param);
    async_video_player_init_param(&async_video_param, fb_param.fb_device, &fb_param.param);

    ffmpeg_audio_decoder_init_param(&audio_decoder_param);
    alsa_audio_player_init_default_param(&alsa_player_param, 2, 48000);
    async_audio_player_init_param(&async_audio_param, audio_device, &alsa_player_param.param);
    ffmpeg_audio_resample_init_default_param(&resampler_param, 2, 48000, AUDIO_fltp, AUDIO_flt);

    m_p_param.demuxing_param = &demuxing_param;
    m_p_param.video_player_param = &async_video_param.param;
    m_p_param.video_decoder_param = &video_decoder_param.param;

    m_p_param.audio_player_param = &async_audio_param.param;
    m_p_param.audio_decoder_param = &audio_decoder_param.param;
    m_p_param.audio_resampler_param = &resampler_param;

    m_p_param.demuxing_param->input_file = file_name;

    struct media_player *player = media_player_open(&m_p_param);
    if (!player) {
        fprintf(stderr, "media player open err\n");
        return -1;
    }

    int ret;

    while (1) {
        ret = media_player_play_one_frame(player);
        if (ret < 0)
            break;
    }

    media_player_close(player);

    return 0;
}