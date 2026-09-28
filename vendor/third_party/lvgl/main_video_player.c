#include <libmedia/media_demuxing.h>
#include <libmedia/media_player.h>
#include <libmedia/decode/ffmpeg_video_decoder.h>
#include <libmedia/decode/ffmpeg_audio_decoder.h>
#include <libmedia/resample/ffmpeg_audio_resampler.h>
#include <libmedia/play/fb_video_player.h>
#include <libmedia/play/alsa_audio_player.h>
#include <libmedia/rotate/hw_video_rotater.h>
#include <libmedia/utils/fifo_utils.h>
#include <libmedia/utils/pkt_parse.h>
#include <time.h>
#include <libutils2/boot_time.h>
#include <pthread.h>

#include <linux/keyboard.h>
#include <linux/input.h>


void ffmpeg_demuxing_init_param(struct media_demuxing_param *param);

int main(int argc, char *argv[])
{
    int ret = fifo_create_current_pid();
    if (ret < 0)
        return -1;

    struct fifo *fifo = fifo_open_current_pid(1, 0);
    if (!fifo)
        return -1;

    struct media_demuxing_param demuxing_param = {0};
    struct ffmpeg_video_decoder_param video_decoder_param = {0};
    struct fb_video_player_param fb_param = {0};

    struct alsa_audio_player_param alsa_player_param = {0};
    struct audio_resampler_param resampler_param = {0};
    struct ffmpeg_audio_decoder_param audio_decoder_param = {0};

    struct media_player_param m_p_param = {0};

    ffmpeg_demuxing_init_param(&demuxing_param);
    ffmpeg_video_decoder_init_param(&video_decoder_param);
    fb_video_player_init_default_param(&fb_param);
    fb_param.fb_device = "/dev/fb0";
    fb_param.layer_order = lcdc_layer_0;

    ffmpeg_audio_decoder_init_param(&audio_decoder_param);
    alsa_audio_player_init_default_param(&alsa_player_param, 2, 48000);
    ffmpeg_audio_resample_init_default_param(&resampler_param, 2, 48000, AUDIO_fltp, AUDIO_flt);

    m_p_param.demuxing_param = &demuxing_param;
    m_p_param.video_player_param = &fb_param.param;
    m_p_param.video_decoder_param = &video_decoder_param.param;

    m_p_param.audio_player_param = &alsa_player_param.param;
    m_p_param.audio_decoder_param = &audio_decoder_param.param;
    m_p_param.audio_resampler_param = &resampler_param;

    m_p_param.demuxing_param->input_file = argv[1];

    struct media_player *player = NULL;

    int key = -1;
    int is_pause = 0;
    int64_t now;
    int64_t to_time = 0;
    int64_t seek_time = 0;
    uint64_t pos = boot_time_usecs();

    char buf[2048] = {0};

    while (1) {
        player = media_player_open(&m_p_param);
        if (!player) {
            fprintf(stderr, "media player open err\n");
            return -1;
        }

        while (1) {
            to_time = 0;
            seek_time = 0;
            key = -1;
            ret = fifo_read_pkt(fifo, buf, sizeof(buf), 10);
            if (ret < 0) {
                printf("test 123\n");
                break;
            }


            if (buf[0] != '\0') {
                pkt_parse_int(buf, "key_type", &key, 10);
                pkt_parse_int64(buf, "to_time", &to_time, 10);
                pkt_parse_int64(buf, "seek_time", &seek_time, 10);
                if (seek_time == 0)
                    seek_time = 2*1000*1000;
            }

            switch (key) {
                case KEY_LEFT:
                    now = media_player_current_time(player);
                    now -= seek_time;
                    if (now < 0)
                        now = 0;
                    media_player_seek_backward(player, now);
                    break;
                case KEY_RIGHT:
                    now = media_player_current_time(player);
                    now += seek_time;
                    media_player_seek_forward(player, now);
                    break;
                case KEY_PAUSE:
                    if (!is_pause)
                        media_player_pause(player);
                    else
                        media_player_resume(player);

                    is_pause = !is_pause;
                    break;
                case KEY_SETUP:
                    // printf("to time = %lld\n", to_time);
                    now = media_player_current_time(player);
                    if (now < to_time)
                        media_player_seek_forward(player, to_time);
                    else
                        media_player_seek_backward(player, to_time);
                    break;
                default:
                    break;
            }

            if (key == KEY_END) {
               media_player_close(player);
               return 0;
            }

            ret = media_player_play_one_frame(player);
            if (ret < 0){
                printf("meida player play one _frame err\n");
                break;
            }

            if (boot_time_usecs() - pos >= 200 * 1000) {
                pos = boot_time_usecs();

                fifo_write_pkt2(fifo, 100, "current_time=%lld\n", media_player_current_time(player));
            }
        }

        media_player_close(player);
        player = NULL;
    }


    return 0;
}