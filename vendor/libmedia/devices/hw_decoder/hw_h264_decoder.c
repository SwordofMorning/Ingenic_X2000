#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <libmedia/media_alloter.h>
#include <libmedia/decode/hw_h264_decoder.h>
#include <linux/videodev2.h>

struct hw_h264_video_decoder {
    struct video_decoder decoder;
    struct v4l2_h264_decoder *v4l2_h264_decoder;

    int width;
    int height;
    enum video_frame_format format;
    int linesize;

    int align_height;
    struct video_frame *frame;
};

#define FMT_C(f) \
    ((char *)&(f))[0],((char *)&(f))[1],((char *)&(f))[2],((char *)&(f))[3]

static struct video_decoder *hw_h264_video_decoder_open(struct video_decoder_param *param)
{
    struct hw_h264_video_decoder_param *p = (void *)param;
    assert(p);

    struct v4l2_h264_decoder_config *cfg = &p->config;

    struct hw_h264_video_decoder *h264_decoder = malloc(sizeof(*h264_decoder));
    assert(h264_decoder);

    struct v4l2_h264_decoder *v4l2_h264_decoder = v4l2_h264_decoder_direct_open(cfg);
    if (!v4l2_h264_decoder) {
        fprintf(stderr, "hw_h264_decoder: Unable to open v4l2 h264 decoder\n");
        free(h264_decoder);
        return NULL;
    }

    h264_decoder->v4l2_h264_decoder = v4l2_h264_decoder;
    h264_decoder->width = cfg->width;
    h264_decoder->height = cfg->height;
    h264_decoder->linesize = cfg->linesize;
    h264_decoder->align_height = cfg->colunmsize;
    h264_decoder->format = VIDEO_nv12;
    h264_decoder->frame = NULL;

    return &h264_decoder->decoder;
}

static void hw_h264_video_decoder_close(struct video_decoder *decoder)
{
    struct hw_h264_video_decoder *h264_decoder = (void *)decoder;

    v4l2_h264_decoder_close(h264_decoder->v4l2_h264_decoder);

    free(h264_decoder);
}

static int hw_h264_video_decoder_send_pkt(struct video_decoder *decoder, struct media_packet *pkt)
{
    struct hw_h264_video_decoder *h264_decoder = (void *)decoder;

    int ret;
    int width = h264_decoder->width;
    int height = h264_decoder->align_height;

    if (h264_decoder->frame) {
        video_frame_put(h264_decoder->frame);
        h264_decoder->frame = NULL;
    }


    struct video_frame *frame = media_alloter_alloc_video_frame_planar(NULL, width, height, h264_decoder->format, 128);
    if (!frame) {
        fprintf(stderr, "hw_h264_decoder: failed to alloc frame, [%dx%d]\n", width, height);
        return -1;
    }


    ret = v4l2_h264_decoder_direct_work(h264_decoder->v4l2_h264_decoder, pkt->data, pkt->size, frame->data[0], frame->data[1]);
    if (ret) {
        fprintf(stderr, "hw_h264_decoder: failed to decode h264\n");
        return -1;
    }

    h264_decoder->frame = frame;

    return 0;
}

static int hw_h264_video_decoder_get_frame(struct video_decoder *decoder, struct video_frame **dst_frame)
{
    struct hw_h264_video_decoder *h264_decoder = (void *)decoder;

    if (!h264_decoder->frame)
        return -MEDIA_EAGAIN;

    *dst_frame = h264_decoder->frame;
    h264_decoder->frame = NULL;

    return 0;
}

struct video_decoder_cb h264_video_decoder_cb = {
    .open_decoder = hw_h264_video_decoder_open,
    .close_decoder = hw_h264_video_decoder_close,
    .send_pkt = hw_h264_video_decoder_send_pkt,
    .get_frame = hw_h264_video_decoder_get_frame,
};

void hw_h264_video_decoder_init_param(struct hw_h264_video_decoder_param *decoder_param)
{
    struct video_decoder_param *param = &decoder_param->param;

    param->cb = &h264_video_decoder_cb;

}

void hw_h264_decoder_init_default_param(
    struct hw_h264_video_decoder_param *decoder_param, int width, int height)
{
    memset(decoder_param, 0 , sizeof(*decoder_param));

    decoder_param->config.width = width;
    decoder_param->config.height = height;
    decoder_param->config.output_fmt = V4L2_PIX_FMT_NV12;
    decoder_param->config.video_path = "/dev/video2";

    hw_h264_video_decoder_init_param(decoder_param);

}

