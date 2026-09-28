#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libmedia/decode/turbo_jpeg_video_decoder.h>

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

    uint64_t start = boot_time_usecs();

    ret = video_decoder_send_pkt(decoder, &pkt);
    if (ret)
        return -1;

    struct video_frame *frame;
    ret = video_decoder_get_frame(decoder, &frame);
    if (ret)
        return -1;
    file_write_data("/tmp/picture", frame->data[0], frame->total_size);

    printf("fmt: %s size: %dx%d decode_time: %.03fms\n",
         video_fmt_name(frame->format), frame->width, frame->height,
         (boot_time_usecs()-start)/1000.0);

    return 0;
}
