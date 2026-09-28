#include <libmedia/read/alsa_audio_reader.h>
#include <libmedia/read/isp_video_reader.h>

#include <libmedia/play/fb_video_player.h>
#include <libmedia/rotate/hw_video_rotater.h>

#include <libmedia/media_previewer.h>

#ifdef APP_libmedia_ffmpeg
#include <libmedia/resample/ffmpeg_audio_resampler.h>
#endif

int main(int argc, char *argv[])
{
    int ret;

#ifdef APP_libmedia_ffmpeg
    struct audio_resampler_param resampler_param = {
        .src_channels = 2,
        .src_format = AUDIO_s16le,
        .src_rate = 48000,

        .dst_channels = 2,
        .dst_format = AUDIO_flt,
        .dst_rate = 48000,
    };

    ffmpeg_audio_resampler_init_param(&resampler_param);
#endif

    struct alsa_audio_reader_param alsa_reader_param = {
        .alsa_capture_device = "plughw:0,0",
        .alsa_params = {
            .rate = 48000,
            .format = SND_PCM_FORMAT_S16_LE,
            .channels = 2,
        },
#ifdef APP_libmedia_ffmpeg
        .param.resampler_param = &resampler_param,
#endif
    };


    alsa_audio_reader_init_param(&alsa_reader_param);


    struct video_rotater_param rotater_param = {
        .rotate_angle = rotate_0,
        .hflip = 1,
        .vflip = 0,
    };

    struct fb_video_player_param fb_param = {
        .fb_device = "/dev/fb0",
    };

    struct isp_video_reader_param isp_param = {
        .isp_device = "/dev/mscaler1-ch0",
        .isp_params = {
            .pixel_format       = CAMERA_PIX_FMT_NV12,
            .frame_nums         = 3,
            .width              = 1280,
            .height             = 720,
            .scaler.enable      = 1,
            .scaler.width       = 1280,
            .scaler.height      = 720,
            .crop.enable        = 0,
        },
    };

    fb_video_player_init_param(&fb_param);
    isp_video_reader_init_param(&isp_param);
    hw_video_rotater_init_param(&rotater_param);

    struct media_previewer_param previewer_param = {
        .audio_reader_param = &alsa_reader_param.param,
        .video_player_param = &fb_param.param,
        .player_rotater_param = NULL,
        .video_reader_param = &isp_param.param,
    };

    struct media_previewer *previewer = media_previewer_open(&previewer_param);
    if(!previewer)
        return -1;

    int cnt = 500;
    struct video_frame *frame = NULL;

    while(1) {
        ret = video_reader_read_frame(previewer->video, &frame);
        if (ret < 0)
            break;
        if (ret == 1) {
            usleep(10*1000);
            continue;
        }
        cnt--;
        media_previewer_display_video_frame(previewer, frame, 0);

        video_frame_put(frame);
    }

    media_previewer_close(previewer);

    return 0;
}