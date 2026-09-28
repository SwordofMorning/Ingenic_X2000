#include <libmedia/media_recorder.h>
#include <libmedia/muxing/ffmpeg_muxing.h>
#include <libmedia/read/isp_video_reader.h>

#include <libmedia/play/fb_video_player.h>
#include <libmedia/play/alsa_audio_player.h>
#include <libmedia/read/alsa_audio_reader.h>

#include <libmedia/rotate/hw_video_rotater.h>
#include <libmedia/resample/ffmpeg_audio_resampler.h>
#include <libmedia/encode/ffmpeg_audio_encoder.h>
#include <libmedia/encode/hw_h264_video_encoder.h>


static struct media_recorder_param *recorder_isp_alsa_ffmpeg_h264_create_param(
    int width, int height, int sample_rate, int channels)
{
    struct m_params {
    struct media_recorder_param recorder_param;
    struct audio_resampler_param resampler_param;
    struct alsa_audio_reader_param audio_param;
    struct isp_video_reader_param video_param;
    struct fb_video_player_param fb_param;
    struct media_previewer_param previewer_param;
    struct ffmpeg_audio_encoder_param audio_encoder_param;
    struct hw_h264_video_encoder_param video_encoder_param;
    struct media_muxing_param muxing_param;
    };

    struct m_params *p = malloc(sizeof(*p));

    struct audio_resampler_param *resampler_param = &p->resampler_param;
    struct alsa_audio_reader_param *audio_param = &p->audio_param;
    struct isp_video_reader_param *video_param = &p->video_param;
    struct fb_video_player_param *fb_param = &p->fb_param;
    struct media_previewer_param *previewer_param = &p->previewer_param;
    struct ffmpeg_audio_encoder_param *audio_encoder_param = &p->audio_encoder_param;
    struct hw_h264_video_encoder_param *video_encoder_param = &p->video_encoder_param;
    struct media_muxing_param *muxing_param = &p->muxing_param;
    struct media_recorder_param *recorder_param = &p->recorder_param;

    ffmpeg_audio_resample_init_default_param(resampler_param, channels, sample_rate, AUDIO_s16le, AUDIO_fltp);
    alsa_audio_reader_init_default_param(audio_param, channels, sample_rate);
    isp_video_reader_init_default_param(video_param, width, height);
    fb_video_player_init_default_param(fb_param);
    ffmpeg_audio_encoder_init_default_param(audio_encoder_param, channels, sample_rate);
    hw_h264_encoder_init_default_param(video_encoder_param, width, height);

    audio_param->param.resampler_param = resampler_param;
    previewer_param->audio_reader_param = &audio_param->param;
    previewer_param->video_reader_param = &video_param->param;
    previewer_param->video_player_param = &fb_param->param;
    previewer_param->player_rotater_param = NULL;

    ffmpeg_muxing_init_param(muxing_param);

    recorder_param->previewer_param = previewer_param;
    recorder_param->audio_encoder_param = &audio_encoder_param->param;
    recorder_param->video_encoder_param = &video_encoder_param->param;
    recorder_param->muxing_param = muxing_param;

    return recorder_param;
}


static void recorder_isp_alsa_ffmpeg_h264_free_param(struct media_recorder_param *recorder_param)
{
    free(recorder_param);
}