#include <libmedia/utils/file_utils.h>
#include <libmedia/play/fb_video_player.h>
#include <libmedia/read/isp_video_reader.h>
#include <libutils2/boot_time.h>
#include <libmedia/decode/turbo_jpeg_video_decoder.h>

int main(int argc, char *argv[])
{
    long data_size;
    void *data;
    int ret = file_read_data(argv[1], &data, &data_size);
    if (ret)
        return -1;

    struct turbo_jpeg_video_decoder_param param;
    memset(&param, 0, sizeof(param));
    turbo_jpeg_video_decoder_init_param(&param);

    struct video_decoder *decoder = video_decoder_open(&param.param);
    if (!decoder)
        return -1;

    struct media_packet pkt = {
        .data = data,
        .size = data_size,
        .type = VIDEO_pkt_mjpeg,
    };

    ret = video_decoder_send_pkt(decoder, &pkt);
    if (ret)
        return -1;

    struct video_frame *src_frame;
    ret = video_decoder_get_frame(decoder, &src_frame);
    if (ret)
        return -1;

    struct video_frame *m_frame = media_alloter_alloc_video_frame(
                                            NULL, src_frame->width, src_frame->height, VIDEO_nv12, 0);

    video_frame_copy(src_frame, m_frame);

    struct video_frame *dst_frame = media_alloter_alloc_video_frame(
                                    NULL, m_frame->width, m_frame->height, VIDEO_bgra, 0);
    video_frame_copy(m_frame, dst_frame);

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

    while(1) {
        ret = video_reader_read_frame(reader, &frame);
        if (ret == 1)
            continue;
        if (ret < 0)
            break;

        uint64_t start = boot_time_usecs();
        /* 图片覆盖：bgra图片格式叠在NV12视频格式上
         * 图片分辨率         msa优化(时间)       普通模式(时间)
         * 720P(1280*720)
         *      满叠           6.093ms             27.124ms
         * 1080P(1920*1080)
         *      满叠           14.757ms            61.233ms
         * 满叠是指将图片叠到其对应分辨率大小的视频上，如将720p的照片叠到720p分辨率的视频上
         */
        video_frame_blend(dst_frame, frame, 0, 0, 0x7f);
        printf("overlayer: size: %dx%d overlayer_time: %.03fms\n",
            dst_frame->width, dst_frame->height, (boot_time_usecs()-start)/1000.0);

        video_player_set_media_frame(player, frame);

        video_player_display(player);
        video_frame_put(frame);
    }

    return 0;
}