#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libmedia/video_decoder.h>
#include <libmedia/media_alloter.h>
#include <libmedia/decode/hw_jpeg_decoder.h>
#include <linux/videodev2.h>

struct hw_jpeg_video_decoder {
    struct video_decoder decoder;
    struct v4l2_jpeg_decoder *v4l2_jpeg_decoder;

    int width;
    int height;
    enum video_frame_format format;
    struct video_frame *frame;
};

static struct video_decoder *hw_jpeg_decoder_open(struct video_decoder_param *param)
{
    struct hw_jpeg_video_decoder_param *p = (void *)param;
    assert(p);

    struct v4l2_jpeg_decoder_config *cfg = &p->config;

    struct hw_jpeg_video_decoder *jpeg_decoder = malloc(sizeof(*jpeg_decoder));
    assert(jpeg_decoder);

    struct v4l2_jpeg_decoder *v4l2_jpeg_decoder = v4l2_jpeg_decoder_open(cfg);
    if (!v4l2_jpeg_decoder) {
        fprintf(stderr, "hwjpeg: Uable to open v4l2 jpeg decoder\n");
        free(jpeg_decoder);
        return NULL;
    }

    jpeg_decoder->v4l2_jpeg_decoder = v4l2_jpeg_decoder;
    jpeg_decoder->width = cfg->width;
    jpeg_decoder->height = cfg->height;
    jpeg_decoder->format = VIDEO_nv12;
    jpeg_decoder->frame = NULL;

    return &jpeg_decoder->decoder;

}

static void hw_jpeg_decoder_close(struct video_decoder *decoder)
{
    struct hw_jpeg_video_decoder *jpeg_decoder = (void *)decoder;

    v4l2_jpeg_decoder_close(jpeg_decoder->v4l2_jpeg_decoder);

    free(jpeg_decoder);
}

static int hw_jpeg_decoder_send_pkt(struct video_decoder *decoder, struct media_packet *pkt)
{
    struct hw_jpeg_video_decoder *jpeg_decoder = (void *)decoder;
    int ret;
    int width, height;
    void *out_mem[2];

    if (jpeg_decoder->frame) {
        video_frame_put(jpeg_decoder->frame);
        jpeg_decoder->frame = NULL;
    }

    width = jpeg_decoder->width;
    height = jpeg_decoder->height;

    ret = v4l2_jpeg_decoder_work(jpeg_decoder->v4l2_jpeg_decoder, pkt->data, pkt->size, out_mem);
    if (ret) {
        fprintf(stderr, "hwjpeg: failed to decoder jpeg\n");
        return -1;
    }

    struct video_frame *frame = media_alloter_alloc_video_frame(NULL, width, height, jpeg_decoder->format, 0);
    if (!frame) {
        fprintf(stderr, "hwjpeg: failed to alloc %dx%d format: %d \n", width, height, jpeg_decoder->format);
        return -1;
    }

    memcpy(frame->data[0], out_mem[0], width * height);
    memcpy(frame->data[1], out_mem[1], width * height / 2);

    jpeg_decoder->frame = frame;

    return 0;
}

static int hw_jpeg_decoder_get_frame(struct video_decoder *decoder, struct video_frame **dst_frame)
{
    struct hw_jpeg_video_decoder *jpeg_decoder = (void *)decoder;

    if (!jpeg_decoder->frame)
        return -MEDIA_EAGAIN;

    *dst_frame = jpeg_decoder->frame;
    jpeg_decoder->frame = NULL;

    return 0;
}

struct video_decoder_cb hw_jpeg_decoder_cb = {
    .open_decoder = hw_jpeg_decoder_open,
    .close_decoder = hw_jpeg_decoder_close,
    .send_pkt = hw_jpeg_decoder_send_pkt,
    .get_frame = hw_jpeg_decoder_get_frame,
};

void hw_jpeg_video_decoder_init_param(struct hw_jpeg_video_decoder_param *decoder_param)
{
    struct video_decoder_param *param = &decoder_param->param;

    param->cb = &hw_jpeg_decoder_cb;
}

void hw_jpeg_decoder_init_default_param(
    struct hw_jpeg_video_decoder_param *decoder_param, int width, int height)
{
    memset(decoder_param, 0 , sizeof(*decoder_param));

    decoder_param->config.width = width;
    decoder_param->config.height = height;
    decoder_param->config.output_fmt = V4L2_PIX_FMT_NV12;
    decoder_param->config.video_path = "/dev/video1";

    hw_jpeg_video_decoder_init_param(decoder_param);
}