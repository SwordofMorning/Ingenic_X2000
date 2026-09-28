#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <unistd.h>
#include <time.h>
#include <stdint.h>

#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

#include <libutils2/boot_time.h>
#include <libmedia/encode/ffmpeg_video_encoder.h>
#include <libmedia/utils/ffmpeg_utils.h>

struct ffmpeg_video_encoder {
    struct video_encoder encoder;
    struct ffmpeg_video_encoder_param param;

    enum media_packet_type pkt_type;
    AVCodec *codec;
    AVCodecContext *ctx;
    AVFrame *frame;
    int framerate;
    int force_key_frame;
    volatile int64_t start_time;
    volatile int64_t last_time;

    volatile int64_t first_write_time;
    volatile unsigned int write_count;
};

struct ffmpeg_decoder_bit_rate {
    int width;
    int height;
    int bit_rate;
};

static struct ffmpeg_decoder_bit_rate rates[] = {
    {640, 360, 400*1000},
    {640, 480, 900*1000},
    {1280, 720, 2500*1000},
    {1920, 1080, 4000*1000},
};

static int get_bit_rate(struct ffmpeg_video_encoder_param *param)
{
    if (param->bit_rate >= 200*1000)
        return param->bit_rate;

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
    // fprintf(stderr, "encoder: auto bit rate: %lld\n", bit_rate);

    return bit_rate;
}

static enum media_packet_type to_pkt_type(enum AVCodecID id)
{
    switch (id) {
        case AV_CODEC_ID_AAC: return AUDIO_pkt_aac;
        case AV_CODEC_ID_H264: return VIDEO_pkt_h264;
        case AV_CODEC_ID_MJPEG: return VIDEO_pkt_mjpeg;
        case AV_CODEC_ID_PNG: return VIDEO_pkt_png;
        default:
            fprintf(stderr, "ffmpeg_video_encoder: not support this codec id: %d\n", id);
            return -1;
    }
}

static struct video_encoder *ffmpeg_video_encoder_open(struct video_encoder_param *param)
{
    int ret;

    struct ffmpeg_video_encoder_param *ffmpeg_param = (void *)param;
    assert(ffmpeg_param);

    char *encoder_name = (char *)ffmpeg_param->encoder_name;
    if (!encoder_name)
        encoder_name = "h264_v4l2m2m";

    AVCodec *codec = avcodec_find_encoder_by_name(encoder_name);
    if (!codec) {
        fprintf(stderr, "ffmpeg_video_encoder: failed to find codec %s\n", encoder_name);
        return NULL;
    }

    AVCodecContext *ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        fprintf(stderr, "ffmpeg_video_encoder failed to open codec ctx\n");
        return NULL;
    }

    ctx->bit_rate       = get_bit_rate(ffmpeg_param);
    ctx->pix_fmt        = ffmpeg_param->pix_fmt;
    ctx->width          = ffmpeg_param->width;
    ctx->height         = ffmpeg_param->height;
    ctx->time_base      = (AVRational){1, ffmpeg_param->framerate*3};
    ctx->framerate      = (AVRational){ffmpeg_param->framerate, 1};

    ctx->gop_size      = ffmpeg_param->gop_size;
    ctx->profile       = ffmpeg_param->profile;
    ctx->flags        |= AV_CODEC_FLAG_GLOBAL_HEADER;
    ctx->max_b_frames  = 0;

    ret = avcodec_open2(ctx, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_video_encoder failed to open codec\n");
        goto free_ctx;
    }

    struct ffmpeg_video_encoder *ffmpeg_encoder = malloc(sizeof(*ffmpeg_encoder));
    assert(ffmpeg_encoder);

    ffmpeg_encoder->codec = codec;
    ffmpeg_encoder->ctx = ctx;
    ffmpeg_encoder->frame = av_frame_alloc();
    ffmpeg_encoder->framerate = ffmpeg_param->framerate;
    ffmpeg_encoder->pkt_type = to_pkt_type(codec->id);
    ffmpeg_encoder->param = *ffmpeg_param;

    return &ffmpeg_encoder->encoder;

free_ctx:
    avcodec_free_context(&ctx);
    return NULL;
}

static void ffmpeg_video_encoder_close(struct video_encoder *encoder)
{
    struct ffmpeg_video_encoder *ffmpeg_encoder = (void *)encoder;

    av_frame_free(&ffmpeg_encoder->frame);
    avcodec_close(ffmpeg_encoder->ctx);
    avcodec_free_context(&ffmpeg_encoder->ctx);

    free(ffmpeg_encoder);
}

static void init_ffmpeg_frame(struct video_frame *frame, AVFrame *av_frame)
{
    int i;

    av_frame->format = video_fmt_to_ffmpeg_fmt(frame->format);
    av_frame->width = frame->width;
    av_frame->height = frame->height;

    int ret = av_frame_get_buffer(av_frame, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    for(i = 0; i < MAX_AUDIO_FRAME_BUF_CNT; i++) {
        if (frame->data[i])
            av_frame->data[i] = frame->data[i];
        if (frame->linesize[i])
            av_frame->linesize[i] = frame->linesize[i];
    }
}

static int ffmpeg_video_encoder_write_frame(struct video_encoder *encoder, struct video_frame *frame)
{
    int ret;

    struct ffmpeg_video_encoder *ffmpeg_encoder = (void *)encoder;
    AVFrame *av_frame = NULL;

    if (frame) {
        av_frame = ffmpeg_encoder->frame;
        init_ffmpeg_frame(frame, av_frame);
    }

    ret = avcodec_send_frame(ffmpeg_encoder->ctx, av_frame);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_video_encoder: failed to send frame: %s\n", av_err2str(ret));
        return -1;
    }

    return 0;
}

static int ffmpeg_video_encoder_read_pkt(struct ffmpeg_video_encoder *encoder, AVPacket *pkt)
{
    int ret;
    AVCodecContext *ctx = encoder->ctx;

    memset(pkt, 0, sizeof(*pkt));
    av_init_packet(pkt);
    ret = avcodec_receive_packet(ctx, pkt);
    if (ret == AVERROR(EAGAIN))
        return -MEDIA_EAGAIN;
    if (ret == AVERROR_EOF)
        return -MEDIA_EOF;
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_video_encoder: failed to receive packet\n");
        return -1;
    }

    int64_t now = boot_time_usecs();
    int64_t time_us = now - encoder->start_time;
    int64_t duration_us = 0;

    pkt->dts = av_rescale_q(time_us, AV_TIME_BASE_Q, ctx->time_base);
    pkt->pts = pkt->dts;
    pkt->duration = av_rescale_q(duration_us, AV_TIME_BASE_Q, ctx->time_base);

    return 0;
}

static void ffmpeg_free_video_pkt(void *handle, struct media_packet *pkt)
{
    AVPacket *av_pkt = pkt->pdata;
    av_packet_unref(av_pkt);
    av_packet_free(&av_pkt);
    media_packet_free(pkt);
}

static struct media_packet *ffmpeg_alloc_video_pkt(struct ffmpeg_video_encoder *encoder, AVPacket *av_pkt)
{
    struct media_packet *pkt = media_packet_alloc();
    assert(pkt);

    ffmpeg_pkt_to_media_pkt(av_pkt, pkt);
    pkt->put_packet = ffmpeg_free_video_pkt;
    pkt->pdata = av_pkt;
    pkt->pdata_type = MEDIA_PACKET_PDATA_FFMPEG;
    pkt->type = encoder->pkt_type;

    media_packet_get(pkt);

    return pkt;
}

static int ffmpeg_video_encoder_get_packet(struct video_encoder *encoder, struct media_packet **pkt)
{
    int ret;
    struct ffmpeg_video_encoder *ffmpeg_encoder = (void *)encoder;

    AVPacket *av_pkt = av_packet_alloc();
    av_init_packet(av_pkt);

    ret = ffmpeg_video_encoder_read_pkt(ffmpeg_encoder, av_pkt);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_video_encoder: failed to read pkt\n");
        return -1;
    }

    if (ret != 0) {
        av_packet_free(&av_pkt);
        return ret;
    }

    *pkt = ffmpeg_alloc_video_pkt(ffmpeg_encoder, av_pkt);

    return 0;
}

static void ffmpeg_video_encoder_init_muxing_param(struct video_encoder *encoder, struct media_muxing_video_param *param)
{
    struct ffmpeg_video_encoder *ffmpeg_encoder = (void *)encoder;
    struct ffmpeg_video_encoder_param *ffmpeg_param = &ffmpeg_encoder->param;

    param->bit_rate = ffmpeg_param->bit_rate;
    param->fmt = ffmpeg_fmt_to_video_fmt(ffmpeg_param->pix_fmt);
    param->framerate = ffmpeg_param->framerate;
    param->gop_size = ffmpeg_param->gop_size;
    param->width = ffmpeg_param->width;
    param->height = ffmpeg_param->height;
    param->type = ffmpeg_encoder->pkt_type;

    if (ffmpeg_encoder->ctx->extradata) {
        param->extradata = malloc(ffmpeg_encoder->ctx->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!param->extradata)
            return;

        memcpy(param->extradata, ffmpeg_encoder->ctx->extradata, ffmpeg_encoder->ctx->extradata_size);
        param->extradata_size = ffmpeg_encoder->ctx->extradata_size;
    }
}

struct video_encoder_cb ffmpeg_video_encoder_cb = {
    .open_encoder = ffmpeg_video_encoder_open,
    .close_encoder = ffmpeg_video_encoder_close,
    .write_frame = ffmpeg_video_encoder_write_frame,
    .get_pkt = ffmpeg_video_encoder_get_packet,
    .init_muxing_param = ffmpeg_video_encoder_init_muxing_param,
};

void ffmpeg_video_encoder_init_param(struct ffmpeg_video_encoder_param *encoder_param)
{
    struct video_encoder_param *param = &encoder_param->param;

    param->cb = &ffmpeg_video_encoder_cb;
}

void ffmpeg_h264_encoder_init_default_param(
    struct ffmpeg_video_encoder_param *param, int width, int height)
{
    memset(param, 0, sizeof(*param));

    param->width = width;
    param->height = height;
    param->pix_fmt = AV_PIX_FMT_NV12;
    param->bit_rate = 0;
    param->gop_size = 10;
    param->framerate = 30;
    param->profile = FF_PROFILE_H264_MAIN;
    param->encoder_name = "h264_v4l2m2m";

    ffmpeg_video_encoder_init_param(param);
}
