#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <libmedia/encode/hw_h264_video_encoder.h>

struct hw_h264_video_encoder {
    struct video_encoder encoder;
    struct hw_h264_video_encoder_param param;
    struct v4l2_h264_encoder *v4l2_h264_encoder;
    int encoder_fd;

    void *mem;
    unsigned long phy_mem;
    enum media_packet_type pkt_type;
    int set_key_frame;
};

struct h264_decoder_bit_rate {
    int width;
    int height;
    int bit_rate;
};

static struct h264_decoder_bit_rate rates[] = {
    {640, 360, 400*1000},
    {640, 480, 900*1000},
    {1280, 720, 2500*1000},
    {1920, 1080, 4000*1000},
};

static int get_bit_rate(struct v4l2_h264_encoder_config *param)
{
    if (param->bitrate >= 200*1000)
        return param->bitrate;

    int width = param->width;
    int height = param->height;

    int i;
    int size = width*height;
    int N = sizeof(rates)/sizeof(rates[0]);
    for (i = 0; i < N; i++) {
        if (size == (rates[i].width*rates[i].height))
            return rates[i].bit_rate;
        if (size < (rates[i].width*rates[i].height))
            break;
    }

    if (i == 0)
        return rates[0].bit_rate;

    if (i == N)
        return rates[N-1].bit_rate;

    int size0 = rates[i-1].width * rates[i-1].height;
    int size1 = rates[i].width * rates[i].height;
    int64_t bit_rate = rates[i].bit_rate - rates[i-1].bit_rate;

    bit_rate = rates[i-1].bit_rate + bit_rate * (size-size0) / (size1-size0);

    return bit_rate;
}

static struct video_encoder *h264_video_encoder_open(struct video_encoder_param *param)
{
    struct hw_h264_video_encoder_param *p = (void *)param;
    assert(p);

    struct v4l2_h264_encoder_config *cfg = &p->config;

    cfg->bitrate = get_bit_rate(cfg);

    struct hw_h264_video_encoder *h264_encoder = malloc(sizeof(*h264_encoder));
    assert(h264_encoder);

    struct v4l2_h264_encoder *v4l2_h264_encoder = v4l2_h264_encoder_open(cfg);
    if (!v4l2_h264_encoder) {
        fprintf(stderr, "Unable to open v4l2 h264 encoder\n");
        free(h264_encoder);
        return NULL;
    }

    h264_encoder->v4l2_h264_encoder = v4l2_h264_encoder;
    h264_encoder->mem = NULL;
    h264_encoder->phy_mem = 0;
    h264_encoder->pkt_type = VIDEO_pkt_h264;
    h264_encoder->param = *p;

    return &h264_encoder->encoder;
}

static void h264_video_encoder_close(struct video_encoder *encoder)
{
    struct hw_h264_video_encoder *h264_encoder = (void *)encoder;

    v4l2_h264_encoder_close(h264_encoder->v4l2_h264_encoder);

    free(h264_encoder);
}

static struct media_packet *h264_video_encoder_alloc_pkt(struct hw_h264_video_encoder *h264_encoder, void *mem, int size, int key_flags)
{
    struct media_packet *pkt = media_packet_alloc();

    pkt->data = mem;
    pkt->size = size;
    pkt->type = h264_encoder->pkt_type;
    pkt->is_key_frame = key_flags;

    media_packet_get(pkt);

    return pkt;
}

static int h264_video_encoder_write_frame(struct video_encoder *encoder, struct video_frame *frame)
{
    struct hw_h264_video_encoder *h264_encoder = (void *)encoder;

    if (h264_encoder->mem && h264_encoder->phy_mem) {
        fprintf(stderr, "h264_video_encoder: pls get packet first\n");
        return -1;
    }

    h264_encoder->mem = frame->data[0];
    h264_encoder->phy_mem = frame->phys_data[0];

    return 0;
}

static int h264_video_encoder_get_packet(struct video_encoder *encoder, struct media_packet **pkt)
{
    unsigned int output_size;
    int key_flags = 0;
    struct hw_h264_video_encoder *h264_encoder = (void *)encoder;

    if (!h264_encoder->mem || !h264_encoder->phy_mem)
        return 1;

    if (h264_encoder->set_key_frame) {
        v4l2_h264_encoder_set_keyframe(h264_encoder->v4l2_h264_encoder);
        h264_encoder->set_key_frame = 0;
    }

    void *mem = v4l2_h264_encoder_work_by_phy_mem(h264_encoder->v4l2_h264_encoder,
                                                  h264_encoder->mem, h264_encoder->phy_mem, &output_size);

    if (!mem) {
        fprintf(stderr, "h264_video_encoder:failed to encode frame\n");
        return -1;
    }


    key_flags = v4l2_h264_encoder_get_whether_keyframe(h264_encoder->v4l2_h264_encoder);

    h264_encoder->mem = NULL;
    h264_encoder->phy_mem = 0;

    *pkt = h264_video_encoder_alloc_pkt(h264_encoder, mem, output_size, key_flags);

    return 0;
}

struct h264_header {
	unsigned int size;
	unsigned char header;
};


static void h264_video_encoder_init_muxing_param(struct video_encoder *encoder, struct media_muxing_video_param *param)
{
    struct hw_h264_video_encoder *h264_encoder = (void *)encoder;
    struct hw_h264_video_encoder_param *h264_param = &h264_encoder->param;
    struct v4l2_streamparm s_param;

    param->bit_rate = h264_param->config.bitrate;
    param->fmt = VIDEO_nv12;
    param->framerate = h264_param->framerate;
    param->gop_size = h264_param->config.gop_size;
    param->width = h264_param->config.width;
    param->height = h264_param->config.height;
    param->type = h264_encoder->pkt_type;

    v4l2_h264_encoder_get_stream_param(h264_encoder->v4l2_h264_encoder, &s_param);

    struct h264_header *header = (void *)s_param.parm.raw_data;

    param->extradata = malloc(header->size);
    if (!param->extradata)
        return;

    memcpy(param->extradata, &header->header, header->size);
    param->extradata_size = header->size;
}

static void h264_video_encoder_set_next_keyframe(struct video_encoder *encoder)
{
    struct hw_h264_video_encoder *h264_encoder = (void *)encoder;
    h264_encoder->set_key_frame = 1;
}

struct video_encoder_cb h264_video_encoder_cb = {
    .open_encoder = h264_video_encoder_open,
    .close_encoder = h264_video_encoder_close,
    .set_next_keyframe = h264_video_encoder_set_next_keyframe,
    .write_frame = h264_video_encoder_write_frame,
    .get_pkt = h264_video_encoder_get_packet,
    .init_muxing_param = h264_video_encoder_init_muxing_param,
};

void hw_h264_video_encoder_init_param(struct hw_h264_video_encoder_param *encoder_param)
{
    struct video_encoder_param *param = &encoder_param->param;

    param->cb = &h264_video_encoder_cb;
}

void hw_h264_encoder_init_default_param(
    struct hw_h264_video_encoder_param *param, int width, int height)
{
    memset(param, 0, sizeof(*param));

    param->config.width = width;
    param->config.height = height;
    param->config.line_length = width;
    param->config.input_fmt = V4L2_PIX_FMT_NV12;
    param->config.bitrate = 0;
    param->config.gop_size = 10;
    param->config.video_path = "/dev/video1";
    param->framerate = 30;

    hw_h264_video_encoder_init_param(param);
}