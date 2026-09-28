#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/videodev2.h>

#include <libmedia/media_demuxing.h>
#include <libmedia/media_packet.h>
#include <libmedia/video_frame.h>
#include <libmedia/decode/hw_h264_decoder.h>
#include <libmedia/play/fb_video_player.h>

#include <libmedia/utils/file_utils.h>

void ffmpeg_demuxing_init_param(struct media_demuxing_param *param);

int main(int argc, char *argv[])
{
    int ret;
    if (argc != 4) {
        fprintf(stderr, "usage: %s input_name width height\n", argv[0]);
        return -1;
    }

    char *file_name = argv[1];
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);

    struct media_demuxing_param demuxing_param;

    ffmpeg_demuxing_init_param(&demuxing_param);

    demuxing_param.input_file = file_name;

    struct media_demuxing *demuxing = demuxing_open(&demuxing_param);
    if (!demuxing) {
        fprintf(stderr, "demuxing open failed\n");
        goto err_demuxing_open;
    }

    struct hw_h264_video_decoder_param h264_decoder_param;
    hw_h264_decoder_init_default_param(&h264_decoder_param, width, height);

    struct video_decoder *h264_decoder = video_decoder_open(&h264_decoder_param.param);
    if (!h264_decoder) {
        fprintf(stderr, "h264_decoder open failed\n");
        goto err_h264_decoder_open;
    }

    struct fb_video_player_param fb_param = {
        .fb_device = "/dev/fb0",
    };
    fb_video_player_init_param(&fb_param);

    struct video_player *player = video_player_open(&fb_param.param);
    if (!player) {
        fprintf(stderr, "player open failed\n");
        goto err_video_player_open;
    }

    struct media_packet *pkt;
    while (1) {
        ret = demuxing_one_pkt(demuxing, &pkt);
        if (ret == -MEDIA_EOF) {
            fprintf(stderr, "is eof\n");
            break;
        }

        if (pkt->type == VIDEO_pkt_h264) {
            // fprintf(stderr, "pkt: data = %p, size = %d\n", pkt->data, pkt->size);
            video_decoder_send_pkt(h264_decoder, pkt);
            if (ret) {
                fprintf(stderr, "failed to send pkt\n");
                return -1;
            }

            struct video_frame *frame;
            ret = video_decoder_get_frame(h264_decoder, &frame);
            if (ret) {
                fprintf(stderr, "failed to get frame\n");
                return -1;
            }

            video_player_set_media_frame(player, frame);
            video_player_display(player);

            video_frame_put(frame);

            media_packet_put(pkt);
        } else {
            media_packet_put(pkt);
        }
    }

    video_player_close(player, 0);
err_video_player_open:
    video_decoder_close(h264_decoder);
err_h264_decoder_open:
    demuxing_close(demuxing);
err_demuxing_open:

    return 0;
}
