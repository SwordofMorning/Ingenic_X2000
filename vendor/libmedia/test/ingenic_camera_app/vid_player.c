#include <libmedia/media_demuxing.h>
#include <libmedia/media_player.h>
#include <libmedia/decode/ffmpeg_video_decoder.h>
#include <libmedia/decode/ffmpeg_audio_decoder.h>
#include <libmedia/resample/ffmpeg_audio_resampler.h>
#include <libmedia/play/fb_video_player.h>
#include <libmedia/play/alsa_audio_player.h>
#include <libmedia/rotate/hw_video_rotater.h>

void ffmpeg_demuxing_init_param(struct media_demuxing_param *param);

static struct video_frame *get_first_video_frame(const char *file_name)
{
    struct media_demuxing_param demuxing_param;
    struct ffmpeg_video_decoder_param video_decoder_param;
    int ret;

    struct video_frame *dst_frame = NULL;

    demuxing_param.input_file = file_name;

    ffmpeg_demuxing_init_param(&demuxing_param);
    ffmpeg_video_decoder_init_param(&video_decoder_param);

    struct media_demuxing *demux = demuxing_open(&demuxing_param);
    if (!demux) {
        fprintf(stderr, "demuxing open err.\n");
        return NULL;
    }

    ret = demuxing_get_video_param(demux, &video_decoder_param.param);
    if (ret) {
        fprintf(stderr, "get_video_param err.\n");
        goto get_video_param_err;
    }

    struct video_decoder *decoder = video_decoder_open(&video_decoder_param.param);
    if (!decoder) {
        fprintf(stderr, "video decoder open err.\n");
        goto decoder_open_err;
    }

    while (1) {
        struct video_frame *frame = NULL;
        struct media_packet *pkt = NULL;
        ret = demuxing_one_pkt(demux, &pkt);
        if (ret == -MEDIA_EOF)
            break;

        if (pkt->type != VIDEO_pkt_h264) {
            media_packet_put(pkt);
            continue;
        }

        ret = video_decoder_send_pkt(decoder, pkt);
        if (ret < 0) {
            fprintf(stderr, "video decoder send pkt err.\n");
            media_packet_put(pkt);
            break;
        }

        ret = video_decoder_get_frame(decoder, &frame);

        media_packet_put(pkt);

        if (ret == -MEDIA_EAGAIN)
            continue;

        dst_frame = media_alloter_alloc_video_frame(
                                NULL, frame->width, frame->height,
                                VIDEO_nv12, 0);

        video_frame_copy(frame, dst_frame);

        video_frame_put(frame);

        break;
    }

    video_decoder_close(decoder);

decoder_open_err:
get_video_param_err:
    demuxing_close(demux);

    return dst_frame;
}

int main_app_video_play(int argc, char *argv[])
{
    int ret = fifo_create_current_pid();
    if (ret < 0)
        return -1;

    struct fifo *fifo = fifo_open_current_pid(1, 0);
    if (!fifo)
        return -1;

    struct media_demuxing_param demuxing_param;
    struct alsa_audio_player_param alsa_player_param;
    struct fb_video_player_param fb_param;
    struct audio_resampler_param resampler_param;
    struct ffmpeg_audio_decoder_param audio_decoder_param;
    struct ffmpeg_video_decoder_param video_decoder_param;

    ffmpeg_demuxing_init_param(&demuxing_param);
    ffmpeg_audio_decoder_init_param(&audio_decoder_param);
    ffmpeg_video_decoder_init_param(&video_decoder_param);

    fb_video_player_init_default_param(&fb_param);
    alsa_audio_player_init_default_param(&alsa_player_param, 2, 48000);
    ffmpeg_audio_resample_init_default_param(&resampler_param, 2, 48000, AUDIO_fltp, AUDIO_flt);

    struct media_player_param m_p_param = {
        .demuxing_param = &demuxing_param,
        .video_player_param = &fb_param.param,
        .video_decoder_param = &video_decoder_param.param,

        .audio_player_param = &alsa_player_param.param,
        .audio_decoder_param = &audio_decoder_param.param,
        .audio_resampler_param = &resampler_param,
    };

    char *s = argv[2];
    if (!s) {
        fprintf(stderr, "No such file or directory %s\n", s);
        return -1;
    }

    DIR *dir = opendir(s);
    if (!dir) {
        fprintf(stderr, "open directory %s err: %s\n", s, strerror(errno));
        return -1;
    }

    struct dir_table vid_file_node_table[MAX_VID_FILE_NODE_COUNT];

    int file_cnt = read_dir(dir, vid_file_node_table, MAX_VID_FILE_NODE_COUNT, 1);
    if (!file_cnt) {
        fprintf(stderr, "No video found.\n");
        return -1;
    }

    int cur_pos = file_cnt - 1;
    int old_pos = cur_pos;

    char file_name[257] = {0};
    sprintf(file_name, "%s/%s", s, vid_file_node_table[cur_pos].name);

    m_p_param.demuxing_param->input_file = file_name;

    struct media_player *player = media_player_open(&m_p_param);
    if (!player) {
        fprintf(stderr, "media player open err\n");
        goto end;
    }

    struct video_player *video_player = video_player_open(&fb_param.param);
    assert(video_player);

    int is_pause = 0;

    char buf[2048] = {0};

    int is_enter_play = 0;
    int is_show_first_frame = 0;

    printf("===============[video play]==================>\n");
    printf("    KEY_DOWN    => quit\n");
    printf("    KEY_LEFT    => seek backward\n");
    printf("    KEY_RIGHT   => seek forward\n");
    printf("    KEY_HOME    => pause/resume\n");
    printf("=============================================>\n");

    while (1) {
        ret = fifo_read_pkt(fifo, buf, sizeof(buf), 10);
        if (ret < 0)
            break;

        if (buf[0] != '\0') {
            int cmd_quit = 0;
            int cmd_key_left = 0;
            int cmd_key_right = 0;
            int cmd_home = 0;
            int key_type = 0;

            pkt_parse_int(buf, "key_type", &key_type, 10);

            if (key_type == KEY_DOWN)
                cmd_quit = 1;

            if (key_type == KEY_LEFT)
                cmd_key_left = 1;

            if (key_type == KEY_RIGHT)
                cmd_key_right = 1;

            if (key_type == KEY_HOME)
                cmd_home = 1;

            /* 当前场景为播放场景 */
            if (is_enter_play) {
                /* 返回视频预览场景 */
                if (cmd_quit) {
                    is_show_first_frame = 0;
                    is_enter_play = 0;
                }

                /* 播放/暂停 */
                if (cmd_home) {
                    if (is_pause)
                        media_player_resume(player);
                    else
                        media_player_pause(player);
                    is_pause = !is_pause;
                }

                /* 快退 */
                if (cmd_key_left) {
                    int64_t now = media_player_current_time(player);
                    now -= 1000*1000;
                    if (now < 0)
                        now = 0;
                    media_player_seek_backward(player, now);
                }

                /* 快进 */
                if (cmd_key_right) {
                    int64_t now = media_player_current_time(player);
                    now += 1000*1000;
                    media_player_seek_forward(player, now);
                }
            } else {
                /* 当前场景为视频预览场景(显示第一帧) */

                /* 退出 */
                if (cmd_quit)
                    break;

                /* 功能键切换至播放场景 */
                if (cmd_home)
                    is_enter_play = 1;

                /* 上一视频 */
                if (cmd_key_left)
                    cur_pos = get_last_pos(cur_pos, file_cnt);

                /* 下一视频 */
                if (cmd_key_right)
                    cur_pos = get_next_pos(cur_pos, file_cnt);

                if (old_pos != cur_pos) {
                    media_player_close(player);
                    video_player_close(video_player, 1);

                    char file_name[257] = {0};
                    sprintf(file_name, "%s/%s", s, vid_file_node_table[cur_pos].name);
                    m_p_param.demuxing_param->input_file = file_name;

                    video_player = video_player_open(&fb_param.param);
                    player = media_player_open(&m_p_param);

                    is_show_first_frame = 0;
                    old_pos = cur_pos;
                }
            }
        }

        /* 播放视频 */
        if (is_enter_play) {
            ret = media_player_play_one_frame(player);
            if (ret == -MEDIA_EOF) {
                is_enter_play = 0;
                is_show_first_frame = 0;
                continue;
            }

            if (ret < 0)
                break;
        }

        /* 预览视频第一帧 */
        if (!is_enter_play && !is_show_first_frame) {
            char file_name[257] = {0};
            sprintf(file_name, "%s/%s", s, vid_file_node_table[cur_pos].name);

            struct video_frame *frame = get_first_video_frame(file_name);
            if (frame) {
                video_player_set_media_frame(video_player, frame);
                video_player_display(video_player);
                video_frame_put(frame);
            }

            is_show_first_frame = 1;
        }
    }

    video_player_close(video_player, 1);
    media_player_close(player);

end:
    fifo_write_pkt2(fifo, 1000, "is_quit=1\n");

    printf("player end\n");
    return 0;
}