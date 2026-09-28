#include <libmedia/read/isp_video_reader.h>
#include <libmedia/video_reader.h>
#include <libmedia/play/fb_video_player.h>
#include <libmedia/video_player.h>
#include <libmedia/video_frame.h>
#include <libutils2/boot_time.h>

int main(void)
{
    int ret;
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
    struct video_frame *src_frame = NULL;
    struct video_frame *dst_frame = NULL;

    while(1) {
        ret = video_reader_read_frame(reader, &frame);
        if (ret == 1)
            continue;
        if (ret < 0)
            break;
        if(!src_frame)
            src_frame = media_alloter_alloc_video_frame(
                        NULL, frame->width, frame->height,
                        VIDEO_bgra, 0);
        uint64_t begin = boot_time_usecs();
        /* 帧格式转换:nv12格式转为bgra
         * 帧分辨率：1280*720
         * 普通模式转换时间：26.073ms
         */
        video_frame_copy(frame, src_frame);
        printf("convert: format:nv12 to bgra size: %dx%d convert_time: %.03fms\n",
                src_frame->width, src_frame->height, (boot_time_usecs()-begin)/1000.0);

        if (!dst_frame)
            dst_frame = media_alloter_alloc_video_frame(
                        NULL, frame->width, frame->height,
                        VIDEO_nv12, 0);

        uint64_t start = boot_time_usecs();
        /* 帧格式转换:bgra格式转为nv12
         * 帧分辨率：1280*720
         * 普通模式转换时间：15.736ms msa优化转换时间：4.329ms
         */
        video_frame_copy(src_frame, dst_frame);
        printf("convert: format: bgra to nv12 size: %dx%d convert_time: %.03fms\n",
                dst_frame->width, dst_frame->height, (boot_time_usecs()-start)/1000.0);

        video_player_set_media_frame(player, dst_frame);

        video_player_display(player);

        video_frame_put(frame);
    }

    return 0;
}
