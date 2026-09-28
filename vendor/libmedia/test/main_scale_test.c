#include <libmedia/decode/turbo_jpeg_video_decoder.h>
#include <libmedia/scale/ffmpeg_video_scaler.h>

#include <libmedia/utils/file_utils.h>
#include <libutils2/boot_time.h>

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

    struct video_frame *frame;
    ret = video_decoder_get_frame(decoder, &frame);
    if (ret)
        return -1;

    struct ffmpeg_video_scaler_param scaler_param = {
        .fmt = VIDEO_yuv420p,
        .param.dst_width = 1920,
        .param.dst_height = 1280,
    };

    ffmpeg_video_scaler_init_param(&scaler_param);

    struct video_scaler *scaler = video_scaler_open(&scaler_param.param);
    if (!scaler)
        return -1;

    uint64_t start = boot_time_usecs();
    /* scaler的时间与图片大小有关，以分辨率为 1280 * 720(720p)的图片为例
     * 缩放为原始的1/4（640 * 360）：46.394ms
     * 放大为1280p（1920 * 1280）  ：165.799ms
     */
    ret = video_scaler_send_frame(scaler, frame);
    if (ret) {
        video_scaler_close(scaler);
        return -1;
    }

    struct video_frame *dst_frame = NULL;
    ret = video_scaler_get_frame(scaler, &dst_frame);
    if (ret)
        return -1;

    printf("scaler: size: %dx%d scaler_time: %.03fms\n",
            dst_frame->width, dst_frame->height, (boot_time_usecs()-start)/1000.0);

    ret = file_write_data("/tmp/picture", dst_frame->data[0], dst_frame->total_size);

    return 0;
}
