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
#include <libmedia/encode/ffmpeg_audio_encoder.h>
#include <libmedia/utils/ffmpeg_utils.h>

struct ffmpeg_audio_encoder {
    struct audio_encoder encoder;

    struct ffmpeg_audio_encoder_param param;
    enum media_packet_type pkt_type;
    AVCodec *codec;
    AVCodecContext *ctx;
    AVFrame *frame;
    int sample_rate;
    int64_t start_time;
    int64_t last_time;
};

static int to_pkt_type(enum AVCodecID codec_id)
{
    if (codec_id == AV_CODEC_ID_AAC)
        return AUDIO_pkt_aac;
    else
        return -1;
}

static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }

    return 0;
}

static void init_ffmpeg_frame(struct audio_frame *frame, AVFrame *av_frame)
{
    int i;

    for(i = 0; i < MAX_AUDIO_FRAME_BUF_CNT; i++) {
        if (frame->data[i])
            av_frame->data[i] = frame->data[i];
    }

    av_frame->channel_layout = av_get_default_channel_layout(frame->channels);
    av_frame->channels = frame->channels;
    av_frame->nb_samples = frame->nb_samples;
    av_frame->sample_rate = frame->sample_rate;
    av_frame->format = audio_fmt_to_ffmpeg_fmt(frame->format);
}

static void ffmpeg_free_audio_pkt(void *handle, struct media_packet *pkt)
{
    AVPacket *av_pkt = pkt->pdata;
    av_packet_unref(av_pkt);
    av_packet_free(&av_pkt);
    media_packet_free(pkt);
}

static struct media_packet *ffmpeg_alloc_audio_pkt(struct ffmpeg_audio_encoder *encoder, AVPacket *av_pkt)
{
    struct media_packet *pkt = media_packet_alloc();
    assert(pkt);

    ffmpeg_pkt_to_media_pkt(av_pkt, pkt);

    pkt->put_packet = ffmpeg_free_audio_pkt;
    pkt->pdata = av_pkt;
    pkt->pdata_type = MEDIA_PACKET_PDATA_FFMPEG;
    pkt->type = encoder->pkt_type;

    media_packet_get(pkt);

    return pkt;
}

static struct audio_encoder *ffmpeg_audio_encoder_open(struct audio_encoder_param *param)
{
    int ret;

    struct ffmpeg_audio_encoder_param *ffmpeg_param = (void *)param;

    const char *encoder_name = ffmpeg_param->encoder_name;
    if (!encoder_name)
        encoder_name = "aac";

    AVCodec *codec = avcodec_find_encoder_by_name(encoder_name);
    if (!codec) {
        fprintf(stderr, "ffmpeg_audio_encoder: failed to find codec %s\n", encoder_name);
        return NULL;
    }

    if (!check_sample_fmt(codec, ffmpeg_param->sample_fmt)) {
        fprintf(stderr, "ffmpeg_audio_encoder: Encoder does not support sample format %s\n",
                av_get_sample_fmt_name(ffmpeg_param->sample_fmt));
        return NULL;
    }

    AVCodecContext *ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        fprintf(stderr, "ffmpeg_audio_encoder: failed to open codec ctx\n");
        return NULL;
    }

    ctx->bit_rate       = ffmpeg_param->bit_rate;
    ctx->sample_fmt     = ffmpeg_param->sample_fmt;
    ctx->sample_rate    = ffmpeg_param->sample_rate;
    ctx->channels       = ffmpeg_param->channels;
    ctx->channel_layout = av_get_default_channel_layout(ctx->channels);

    ret = avcodec_open2(ctx, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_audio_encoder: failed to open codec\n");
        goto free_ctx;
    }

    struct ffmpeg_audio_encoder *encoder = malloc(sizeof(*encoder));
    assert(encoder);

    encoder->codec = codec;
    encoder->ctx = ctx;
    encoder->sample_rate = ffmpeg_param->sample_rate;

    encoder->start_time = boot_time_usecs();
    encoder->last_time = encoder->start_time;
    encoder->frame = av_frame_alloc();
    encoder->pkt_type = to_pkt_type(ctx->codec_id);
    encoder->param = *ffmpeg_param;

    return &encoder->encoder;

free_ctx:
    avcodec_free_context(&ctx);
    return NULL;

}

static void ffmpeg_audio_encoder_close(struct audio_encoder *encoder)
{
    struct ffmpeg_audio_encoder *ffmpeg_encoder = (void *)encoder;

    avcodec_close(ffmpeg_encoder->ctx);
    avcodec_free_context(&ffmpeg_encoder->ctx);
    av_frame_free(&ffmpeg_encoder->frame);
    free(ffmpeg_encoder);
}

static int ffmpeg_audio_encoder_write_frame(struct audio_encoder *encoder, struct audio_frame *frame)
{
    int ret;

    struct ffmpeg_audio_encoder *ffmpeg_encoder = (void *)encoder;

    AVFrame *av_frame = ffmpeg_encoder->frame;

    init_ffmpeg_frame(frame, av_frame);

    ret = avcodec_send_frame(ffmpeg_encoder->ctx, av_frame);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_audio_encoder: failed to send frame: %s\n", av_err2str(ret));
        return -1;
    }

    return 0;
}

static int ffmpeg_audio_encoder_read_pkt(struct ffmpeg_audio_encoder *encoder, AVPacket *pkt)
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
        fprintf(stderr, "ffmpeg_audio_encoder: failed to receive packet\n");
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

static int ffmpeg_audio_encoder_get_packet(struct audio_encoder *encoder, struct media_packet **pkt)
{
    int ret;
    struct ffmpeg_audio_encoder *ffmpeg_encoder = (void *)encoder;

    AVPacket *av_pkt = av_packet_alloc();
    av_init_packet(av_pkt);

    ret = ffmpeg_audio_encoder_read_pkt(ffmpeg_encoder, av_pkt);
    if (ret < 0 && ret != -MEDIA_EAGAIN && ret != -MEDIA_EOF) {
        fprintf(stderr, "ffmpeg audio encoder: failed to read pkt\n");
        return -1;
    }

    if (ret != 0) {
        av_packet_free(&av_pkt);
        return ret;
    }

    *pkt = ffmpeg_alloc_audio_pkt(ffmpeg_encoder,av_pkt);

    return 0;
}

static void ffmpeg_audio_encoder_init_muxing_param(struct audio_encoder *encoder, struct media_muxing_audio_param *param)
{
    struct ffmpeg_audio_encoder *ffmpeg_encoder = (void *)encoder;
    struct ffmpeg_audio_encoder_param *ffmpeg_param = &ffmpeg_encoder->param;

    param->bit_rate = ffmpeg_param->bit_rate;
    param->channels = ffmpeg_param->channels;
    param->sample_rate = ffmpeg_param->sample_rate;
    param->fmt = ffmpeg_fmt_to_audio_fmt(ffmpeg_param->sample_fmt);
    param->type = ffmpeg_encoder->pkt_type;
}


struct audio_encoder_cb ffmpeg_audio_encoder_cb = {
    .open_encoder = ffmpeg_audio_encoder_open,
    .close_encoder = ffmpeg_audio_encoder_close,
    .write_frame = ffmpeg_audio_encoder_write_frame,
    .get_packet = ffmpeg_audio_encoder_get_packet,
    .init_muxing_param = ffmpeg_audio_encoder_init_muxing_param,
};

void ffmpeg_audio_encoder_init_param(struct ffmpeg_audio_encoder_param *ffmpeg_param)
{
    struct audio_encoder_param *param = &ffmpeg_param->param;
    param->cb = &ffmpeg_audio_encoder_cb;
}

void ffmpeg_audio_encoder_init_default_param(
    struct ffmpeg_audio_encoder_param *param, int channels, int rate)
{
    memset(param, 0, sizeof(*param));

    param->bit_rate = 64000;
    param->channels = channels;
    param->sample_rate = rate;
    param->sample_fmt = AV_SAMPLE_FMT_FLTP;
    param->encoder_name = "aac";

    ffmpeg_audio_encoder_init_param(param);
}