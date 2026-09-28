#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libmedia/uvc_demuxing.h>
#include <libmedia/media_demuxing.h>
#include <libmedia/media_packet.h>
#include <libmedia/decode/hw_h264_decoder.h>
#include <libmedia/video_frame.h>
#include <libmedia/rotate/hw_video_2d_rotater.h>
#include <libmedia/play/fb_video_player.h>
#include <unistd.h>
#include <signal.h>

struct media_demuxing *demuxing = NULL;
struct video_player *player = NULL;
struct video_decoder *decoder = NULL;
struct video_rotater *rotater_2d = NULL;

struct media_packet *pkt = NULL;
struct video_frame *nv12_frame = NULL;
struct video_frame *bgra_frame = NULL;
struct video_frame *convert_frame = NULL;

void killall_signal_handler(int signum) {
    if (signum == SIGTERM) {
        if (rotater_2d)
            video_rotater_close(rotater_2d);
        if (player)
            video_player_close(player, 1);
        if (decoder)
            video_decoder_close(decoder);
        if (demuxing)
            demuxing_close(demuxing);
        if (bgra_frame)
            video_frame_put(bgra_frame);
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    int ret;

    if (argc < 3) {
        printf("usage: %s width(uvc video) height(uvc video) device(uvc device)\n", argv[0]);
        return -1;
    }

    if (signal(SIGTERM, killall_signal_handler) == SIG_ERR) {
        fprintf(stderr, "Failed to set up signal handler");
        return -1;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    char *device = "/dev/video3";
    if (argc == 4)
        device = argv[3];

    bgra_frame = media_alloter_alloc_video_frame(NULL, width, height, VIDEO_bgra, width*4);
    if (!bgra_frame) {
        fprintf(stderr, "failed to alloc video frame \n");
        return -1;
    }

    struct uvc_demuxing_param uvc_demuxing_param = {
        .uvc_format.width = width,
        .uvc_format.height = height,
        .uvc_format.format = V4L2_PIX_FMT_H264,
        .uvc_device = device,
    };
    uvc_demuxing_init_param(&uvc_demuxing_param.param);

    struct hw_h264_video_decoder_param decoder_param = {
        .config.width = width,
        .config.height = height,
        .config.output_fmt = V4L2_PIX_FMT_NV12,
        .config.video_path = "/dev/video2",
        .param.codec_name = "h264",
        .param.width   = width,
        .param.height  = height,
    };
    hw_h264_video_decoder_init_param(&decoder_param);

    struct video_rotater_param rotater_param = {
        .rotate_angle = rotate_90,
        .hflip = 0,
        .vflip = 0,
    };
    hw_video_2d_rotater_init_param(&rotater_param);

    rotater_2d = video_rotater_open(&rotater_param);
    if (!rotater_2d) {
        fprintf(stderr, "failed to open video rotater\n");
        goto err_exit;
    }

    struct fb_video_player_param fb_param = {
        .fb_device = "/dev/fb0",
    };
    fb_video_player_init_param(&fb_param);

    decoder = video_decoder_open(&decoder_param.param);
    if (!decoder) {
        fprintf(stderr, "failed to open video decoder \n");
        goto free_rotater;
    }

    while (access(fb_param.fb_device, F_OK) == -1) {
        usleep(10000);
    }

    player = video_player_open(&fb_param.param);
    if (!player) {
        fprintf(stderr, "failed to open video player\n");
        goto free_decoder;
    }

restart:
    while (access(uvc_demuxing_param.uvc_device, F_OK) == -1) {
        usleep(10000);
    }

    demuxing = demuxing_open(&uvc_demuxing_param.param);
    if (!demuxing) {
        fprintf(stderr, "failed to open video demuxing\n");
        usleep(500000);
        goto restart;
    }

    while (1) {
        ret = demuxing_one_pkt(demuxing, &pkt);
        if (ret) {
            fprintf(stderr, "failed to demuxing one pkt \n");
            goto restart_free_demuxing;
        }

        ret = video_decoder_send_pkt(decoder, pkt);
        media_packet_put(pkt);
        if (ret) {
            fprintf(stderr, "failed to send pkt\n");
            goto restart_free_demuxing;
        }

        ret = video_decoder_get_frame(decoder, &nv12_frame);
        if (ret) {
            fprintf(stderr, "failed to get frame\n");
            goto restart_free_demuxing;
        }

        /* convert video frame nv12_to_bgra */
        video_frame_copy(nv12_frame, bgra_frame);

        /* rotate by 2d */
        ret = video_rotater_convert_video_frame(rotater_2d, bgra_frame, &convert_frame);
        if (ret) {
            fprintf(stderr, "failed to rotate frame\n");
            goto restart_free_demuxing;
        }

        video_player_set_media_frame(player, convert_frame);
        video_player_display(player);

        video_frame_put(nv12_frame);
        video_frame_put(convert_frame);
    }

restart_free_demuxing:
    demuxing_close(demuxing);
    demuxing = NULL;

    usleep(300000);
    goto restart;

    return 0;

free_decoder:
    video_decoder_close(decoder);
free_rotater:
    video_rotater_close(rotater_2d);
err_exit:
    video_frame_put(bgra_frame);
    return -1;
}