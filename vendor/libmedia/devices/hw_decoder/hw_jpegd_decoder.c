#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libmedia/media_alloter.h>
#include <libmedia/video_frame.h>
#include <libmedia/decode/hw_jpegd_decoder.h>

int video_fmt_to_jpegd_fmt(enum video_frame_format format)
{
    switch (format) {
        case VIDEO_nv12: return JPEGD_PIX_FMT_NV12;
        case VIDEO_yuv444p: return JPEGD_PIX_FMT_YUV444;
        case VIDEO_bgra: return JPEGD_PIX_FMT_BGRA_8888;
        case VIDEO_yuv422p: return JPEGD_PIX_FMT_YUYV;
        default:
            fprintf(stderr, "hw_jpegd: not support this video format: %d\n", format);
            return -1;
    }
}

int jpegd_fmt_to_video_fmt(jpegd_pixel_fmt format)
{
    switch (format) {
        case JPEGD_PIX_FMT_NV12:
        case JPEGD_PIX_FMT_NV21: return VIDEO_nv12;
        case JPEGD_PIX_FMT_YUV444: return VIDEO_yuv444p;
        case JPEGD_PIX_FMT_BGRA_8888: return VIDEO_bgra;
        case JPEGD_PIX_FMT_YUYV: return VIDEO_yuv422p;
        default:
            fprintf(stderr, "hw_jpegd: not support this video format: %d\n", format);
            return -1;
    }
}

struct hw_jpegd_decoder {
    struct video_decoder decoder;
    struct jpegd_decoder *module_jpegd_decoder;
    struct jpegd_decoder_config *config;

    int width;
    int height;
    struct video_frame *frame;
};

static struct video_decoder *hw_jpegd_decoder_open(struct video_decoder_param *param)
{
    struct hw_jpegd_decoder_param *p = (void *)param;
    assert(p);

    struct jpegd_decoder_config *cfg = &p->config;

    struct hw_jpegd_decoder *jpegd_decoder = malloc(sizeof(*jpegd_decoder));
    assert(jpegd_decoder);

    struct jpegd_decoder *module_jpegd_decoder = jpegd_decoder_open();
    if (!module_jpegd_decoder) {
        fprintf(stderr, "hw_jpegd: Uable to open jpeg decoder module\n");
        free(jpegd_decoder);
        return NULL;
    }

    jpegd_decoder->module_jpegd_decoder = module_jpegd_decoder;
    jpegd_decoder->config = cfg;
    jpegd_decoder->width = cfg->width;
    jpegd_decoder->height = cfg->height;
    jpegd_decoder->frame = NULL;

    return &jpegd_decoder->decoder;
}

static void hw_jpegd_decoder_close(struct video_decoder *decoder)
{
    struct hw_jpegd_decoder *jpegd_decoder = (void *)decoder;

    jpegd_decoder_close(jpegd_decoder->module_jpegd_decoder);

    free(jpegd_decoder);
}

static void jpegd_decoder_free_frame(void *handle, struct video_frame *frame)
{
    struct jpegd_decoder_output_data *output = frame->pdata;
    jpegd_decoder_put((struct jpegd_decoder *)handle, output);
    free(output);
    video_frame_free(frame);
}

static int hw_jpegd_decoder_send_pkt(struct video_decoder *decoder, struct media_packet *pkt)
{
    struct hw_jpegd_decoder *jpegd_decoder = (void *)decoder;
    int ret;
    struct jpegd_decoder_output_data *output = malloc(sizeof(*output));
    struct jpegd_decoder_config *cfg = jpegd_decoder->config;

    if (jpegd_decoder->frame) {
        video_frame_put(jpegd_decoder->frame);
        jpegd_decoder->frame = NULL;
    }

    cfg->file_size = pkt->size;
    cfg->input_mem = pkt->data;

    ret = jpegd_decoder_get(jpegd_decoder->module_jpegd_decoder, cfg, output);
    if (ret) {
        fprintf(stderr, "hw_jpegd: failed to decoder jpeg\n");
        free(output);
        return -1;
    }

    struct video_frame *frame = video_frame_alloc();
    enum video_frame_format format = jpegd_fmt_to_video_fmt(cfg->out_fmt);
    ret = video_frame_init(frame, output->image_width, output->image_height,
        format, 0, output->mem, output->phy_mem, jpegd_decoder->module_jpegd_decoder, jpegd_decoder_free_frame);
    if (ret < 0) {
        fprintf(stderr, "hw_jpegd: failed to alloc %dx%d format: %d \n", output->image_width, output->image_height, format);
        free(output);
        return -1;
    }

    frame->pdata = output;
    jpegd_decoder->frame = frame;
    video_frame_get(frame);

    return 0;
}

static int hw_jpegd_decoder_get_frame(struct video_decoder *decoder, struct video_frame **dst_frame)
{
    struct hw_jpegd_decoder *jpegd_decoder = (void *)decoder;

    if (!jpegd_decoder->frame)
        return -MEDIA_EAGAIN;

    *dst_frame = jpegd_decoder->frame;
    jpegd_decoder->frame = NULL;

    return 0;
}

struct video_decoder_cb hw_jpegd_decoder_cb = {
    .open_decoder = hw_jpegd_decoder_open,
    .close_decoder = hw_jpegd_decoder_close,
    .send_pkt = hw_jpegd_decoder_send_pkt,
    .get_frame = hw_jpegd_decoder_get_frame,
};

void hw_jpegd_decoder_init_param(struct hw_jpegd_decoder_param *decoder_param)
{
    struct video_decoder_param *param = &decoder_param->param;

    param->cb = &hw_jpegd_decoder_cb;
}

void hw_jpegd_decoder_init_default_param(
    struct hw_jpegd_decoder_param *decoder_param, int width, int height)
{
    memset(decoder_param, 0 , sizeof(*decoder_param));

    decoder_param->config.width = width;
    decoder_param->config.height = height;
    decoder_param->config.out_fmt = JPEGD_PIX_FMT_NV12;

    hw_jpegd_decoder_init_param(decoder_param);
}
