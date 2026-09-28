#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libmedia/video_frame.h>
#include <libmedia/uvc_demuxing.h>
#include <libmedia/media_demuxing.h>
#include <libmedia/media_packet.h>
#include <libmedia/decode/hw_jpeg_decoder.h>
#include <libmedia/play/fb_video_player.h>

int main(int argc, char *argv[])
{
    int ret = 0;
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    struct hw_jpeg_video_decoder_param decoder_param;
    hw_jpeg_decoder_init_default_param(&decoder_param, width, height);

    struct uvc_demuxing_param uvc_demuxing_param = {
        .uvc_format.width = width,
        .uvc_format.height = height,
        .uvc_device = "/dev/video4",
    };
    uvc_demuxing_init_param(&uvc_demuxing_param.param);

    struct fb_video_player_param fb_param = {
        .fb_device = "/dev/fb0",
    };
    fb_video_player_init_param(&fb_param);

    struct video_player *player = video_player_open(&fb_param.param);
    if (!player) {
        fprintf(stderr, "failed to open video player\n");
        return -1;
    }

    struct media_demuxing *demuxing = demuxing_open(&uvc_demuxing_param.param);
    if (!demuxing) {
        fprintf(stderr, "failed to open video demuxing\n");
        return -1;
    }

    struct video_decoder *decoder = video_decoder_open(&decoder_param.param);
    if (!decoder) {
        fprintf(stderr, "failed to open video decoder \n");
        return -1;
    }

    struct video_frame *frame;
    struct media_packet *pkt;

    while(1) {

        ret = demuxing_one_pkt(demuxing, &pkt);
        if (ret) {
            fprintf(stderr, "failed to demuxing one pkt \n");
            break;
        }

        ret = video_decoder_send_pkt(decoder, pkt);
        if (ret) {
            fprintf(stderr, "failed to send pkt\n");
            return -1;
        }

        ret = video_decoder_get_frame(decoder, &frame);
        if (ret) {
            fprintf(stderr, "failed to get frame\n");
            return -1;
        }

        video_player_set_media_frame(player, frame);
        video_player_display(player);

        media_packet_put(pkt);

        video_frame_put(frame);

    }

    return 0;
}