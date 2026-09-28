#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <libmedia/decode/ffmpeg_video_decoder.h>
#include <libmedia/utils/ffmpeg_utils.h>

#include <libavcodec/avcodec.h>

struct ffmpeg_video_decoder {
    struct video_decoder decoder;
    AVCodecContext *dec_ctx;
};

static AVCodec *try_find_h264_hwcodec(char *name)
{
    AVCodec *dec = avcodec_find_decoder_by_name("h264_v4l2m2m");
    if (!dec)
        dec = avcodec_find_decoder_by_name(name);

    return dec;
}

static struct video_decoder *ffmpeg_video_decoder_open(struct video_decoder_param *param)
{
    assert(param);

    AVCodec *dec;

    if (!strcmp(param->codec_name, "h264"))
        dec = try_find_h264_hwcodec(param->codec_name);
    else
        dec = avcodec_find_decoder_by_name(param->codec_name);

    if (!dec) {
        fprintf(stderr, "ffmpeg_video_decoder: Could not find video decoder.\n");
        return NULL;
    }

    AVCodecContext *dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx) {
        fprintf(stderr, "ffmpeg_video_decoder: Could not alloc avcodec context3.\n");
        return NULL;
    }

    dec_ctx->bit_rate    = param->bit_rate;

    dec_ctx->pix_fmt =  video_fmt_to_ffmpeg_fmt(param->fmt);
    dec_ctx->width = param->width;
    dec_ctx->height = param->height;

    dec_ctx->extradata = param->extradata;
    dec_ctx->extradata_size = param->extradata_size;

    dec_ctx->pkt_timebase = av_make_q(param->timebase_num, param->timebase_den);
    dec_ctx->framerate = av_make_q(param->fps_num, param->fps_den);

    int ret = avcodec_open2(dec_ctx, dec, NULL);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_video_decoder: avcodec_open2 err %s.\n", av_err2str(ret));
        goto out;
    }

    struct ffmpeg_video_decoder *decoder = malloc(sizeof(*decoder));
    assert(decoder);

    memset(decoder, 0, sizeof(*decoder));

    decoder->dec_ctx = dec_ctx;

    return &decoder->decoder;

out:
    avcodec_free_context(&dec_ctx);

    return NULL;
}

static void ffmpeg_video_decoder_close(struct video_decoder *video_decoder)
{
    struct ffmpeg_video_decoder *decoder = (void *)video_decoder;

    avcodec_close(decoder->dec_ctx);
    avcodec_free_context(&decoder->dec_ctx);
    free(decoder);
}

static void ffmpeg_decoder_free_frame(void *handle, struct video_frame *frame)
{
    AVFrame *avframe = frame->pdata;

    av_frame_free(&avframe);

    video_frame_free(frame);
}

int ffmpeg_video_decoder_send_pkt(struct video_decoder *video_decoder, struct media_packet *pkt)
{
    struct ffmpeg_video_decoder *decoder = (void *)video_decoder;

    AVPacket tmp_pkt = {0};
    AVPacket *avpkt = &tmp_pkt;

    if (pkt->pdata_type == MEDIA_PACKET_PDATA_FFMPEG)
        avpkt = pkt->pdata;
    else {
        av_init_packet(avpkt);
        media_pkt_to_ffmpeg_pkt(pkt, avpkt);
    }

    int ret = avcodec_send_packet(decoder->dec_ctx, avpkt);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_video_decoder: failed to send pkt, %s\n", av_err2str(ret));
        return -1;
    }

    return 0;
}

int ffmpeg_video_decoder_get_frame(struct video_decoder *video_decoder, struct video_frame **dst_frame)
{
    struct ffmpeg_video_decoder *decoder = (void *)video_decoder;
    AVCodecContext *dec_ctx = decoder->dec_ctx;
    enum AVPixelFormat pix_fmt = dec_ctx->pix_fmt;
    enum video_frame_format video_format;

    int ret = 0;

    video_format = ffmpeg_fmt_to_video_fmt(pix_fmt);
    if (video_format < 0)
        return -1;

    AVFrame *avframe = av_frame_alloc();

    ret = avcodec_receive_frame(dec_ctx, avframe);
    if (ret < 0) {
        av_frame_free(&avframe);
        *dst_frame = NULL;
        if (ret == AVERROR(EAGAIN))
            return -MEDIA_EAGAIN;
        else if (ret == AVERROR_EOF)
            return -MEDIA_EOF;
        else {
            fprintf(stderr, "ffmpeg_decoder: failed to receive frame: %s\n", av_err2str(ret));
            return -1;
        }
    }

    struct video_frame *frame = video_frame_alloc();

    frame->width = avframe->width;
    frame->height = avframe->height;
    int i;
    for (i = 0; i < MAX_VIDEO_FRAME_BUF_CNT; i++) {
        if (!avframe->data[i])
            continue;

        frame->size[i] = avframe->height * avframe->linesize[i];
        frame->linesize[i] = avframe->linesize[i];
        frame->data[i] = avframe->data[i];
    }
    frame->format = video_format;
    frame->pdata = avframe;
    frame->handle = NULL;
    frame->put_frame = ffmpeg_decoder_free_frame;

    frame->timestamp_us = av_rescale_q(avframe->pts, dec_ctx->pkt_timebase, AV_TIME_BASE_Q);
    if (frame->timestamp_us == AV_NOPTS_VALUE)
        frame->timestamp_us = -1;
    *dst_frame = frame;
    video_frame_get(frame);

    return ret;
}


struct video_decoder_cb ffmpeg_video_decoder_cb = {
    .open_decoder = ffmpeg_video_decoder_open,
    .close_decoder = ffmpeg_video_decoder_close,
    .send_pkt =ffmpeg_video_decoder_send_pkt,
    .get_frame =ffmpeg_video_decoder_get_frame,
};

void ffmpeg_video_decoder_init_param(struct ffmpeg_video_decoder_param *decoder_param)
{
    struct video_decoder_param *param = &decoder_param->param;

    param->cb = &ffmpeg_video_decoder_cb;
}