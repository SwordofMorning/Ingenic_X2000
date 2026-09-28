#include <libmedia/read/isp_video_reader.h>
#include <libmedia/video_reader.h>
#include <libmedia/play/fb_video_player.h>
#include "hw_video_rotater.h"
#include <libmedia/video_player.h>
#include <libmedia/video_frame.h>


int main(void)
{
    int ret;
    struct video_rotater_param rotater_param = {
        .rotate_angle = rotate_0,
        .hflip = 1,
        .vflip = 0,
    };

    struct fb_video_player_param fb_param = {
        .fb_device = "/dev/fb0",
        // .param.rotater_param = &rotater_param,
    };

    struct isp_video_reader_param isp_param = {
        .isp_device = "/dev/mscaler0-ch0",
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
        .param.rotater_param = &rotater_param,
    };

    fb_video_player_init_param(&fb_param);
    isp_video_reader_init_param(&isp_param);
    hw_video_rotater_init_param(&rotater_param);

    struct video_reader *reader = video_reader_open(&isp_param.param);
    if (!reader) {
        fprintf(stderr, "failed to open video reader\n");
        return -1;
    }

    struct video_player *player = video_player_open(&fb_param.param);
    if (!player) {
        fprintf(stderr, "failed to open video player\n");
        return -1;
    }

    struct video_frame *frame = NULL;

    while(1) {
        ret = video_reader_read_frame(reader, &frame);
        if (ret == 1)
            continue;

        if (ret < 0)
            break;

        video_player_set_media_frame(player, frame);

        video_player_display(player);

        video_frame_put(frame);
    }

    return 0;
}
