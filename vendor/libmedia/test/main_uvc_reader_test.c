#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <libmedia/video_frame.h>
#include <libmedia/play/fb_video_player.h>
#include <libmedia/decode/hw_jpeg_decoder.h>
#include <libmedia/read/uvc_video_reader.h>

int main(int argc, char *argv[])
{
    int ret = 0;

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    struct hw_jpeg_video_decoder_param decoder_param;
    hw_jpeg_decoder_init_default_param(&decoder_param, width, height);

    struct uvc_video_reader_param uvc_reader_param = {
        .decoder_param = &decoder_param.param,
        .uvc_format.width = width,
        .uvc_format.height = height,
        .uvc_device = "/dev/video4",
    };
    uvc_video_reader_init_param(&uvc_reader_param);


    struct fb_video_player_param fb_param = {
        .fb_device = "/dev/fb0",
    };

    fb_video_player_init_param(&fb_param);

    struct video_player *player = video_player_open(&fb_param.param);
    if (!player) {
        fprintf(stderr, "failed to open video player\n");
        return -1;
    }

    struct video_reader *reader = video_reader_open(&uvc_reader_param.param);
    if (!reader) {
        fprintf(stderr, "failed to open video reader\n");
        return -1;
    }

    struct video_frame *frame;

    while (1) {
        ret = video_reader_read_frame(reader, &frame);
        if (ret) {
            fprintf(stderr, "failed to reader frame\n");
            continue;
        }
        video_player_set_media_frame(player, frame);
        video_player_display(player);

        video_frame_put(frame);

    }

    return 0;
}