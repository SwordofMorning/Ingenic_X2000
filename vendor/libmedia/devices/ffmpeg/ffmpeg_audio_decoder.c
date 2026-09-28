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

#include <libmedia/decode/ffmpeg_audio_decoder.h>

#include <libmedia/utils/ffmpeg_utils.h>

struct ffmpeg_audio_decoder {
    struct audio_decoder decoder;
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *ctx;
};

struct audio_decoder *ffmpeg_audio_decoder_open(struct audio_decoder_param *param)
{
    int ret;

    const AVCodec *codec = avcodec_find_decoder_by_name(param->codec_name);
    if (codec == NULL) {
        fprintf(stderr, "ffmpeg_audio_decoder: failed to find codec: %s\n", param->codec_name);
        return NULL;
    }

    AVCodecContext *ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        fprintf(stderr, "ffmpeg_audio_decoder: failed to alloc codec ctx\n");
        return NULL;
    }

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */
    ctx->channels = param->channels;
    ctx->sample_rate = param->rate;

    /* APE 需要设置 bits_per_coded_sample 和 extradata
     */
    ctx->bits_per_coded_sample = audio_frame_bytes_per_sample(param->fmt)*8;
    ctx->extradata = param->extradata;
    ctx->extradata_size = param->extradata_size;

    /* open it */
    ret = avcodec_open2(ctx, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_decoder: failed to open codec\n");
        goto free_ctx;
    }

    AVFrame *frame = av_frame_alloc();
    assert(frame);

    struct ffmpeg_audio_decoder *decoder = malloc(sizeof(*decoder));
    assert(decoder);

    decoder->codec = codec;
    decoder->ctx = ctx;
    decoder->parser = NULL;

    return &decoder->decoder;
free_ctx:
    avcodec_free_context(&ctx);
    return NULL;
}

void ffmpeg_audio_decoder_close(struct audio_decoder *decoder)
{
    struct ffmpeg_audio_decoder *ffmpeg_decoder = (void *)decoder;

    avcodec_close(ffmpeg_decoder->ctx);
    avcodec_free_context(&ffmpeg_decoder->ctx);
    free(ffmpeg_decoder);
}

void ffmpeg_audio_decoder_free_frame(void *handle, struct audio_frame *frame)
{
    AVFrame *av_frame = frame->pdata;
    av_frame_unref(av_frame);
    av_frame_free(&av_frame);
    audio_frame_free(frame);
}

struct audio_frame *ffmpeg_audio_decoder_alloc_frame(AVFrame *av_frame)
{
    struct audio_frame *frame = audio_frame_alloc();
    assert(frame);

    int i;
    int is_planar = av_sample_fmt_is_planar(av_frame->format);

    frame->total_size = av_samples_get_buffer_size(NULL, av_frame->channels, av_frame->nb_samples, av_frame->format, 0);

    if (is_planar) {
        for(i = 0; i < av_frame->channels; i++) {
            frame->data[i] = av_frame->data[i];
            frame->size[i] = av_samples_get_buffer_size(NULL, 1, av_frame->nb_samples, av_frame->format, 0);
        }
    } else {
        frame->data[0] = av_frame->data[0];
        frame->size[0] = frame->total_size;
    }

    frame->channels = av_frame->channels;
    frame->sample_rate = av_frame->sample_rate;
    frame->nb_samples = av_frame->nb_samples;
    frame->format = ffmpeg_fmt_to_audio_fmt(av_frame->format);

    frame->pdata = av_frame;

    frame->handle = NULL;
    frame->put_frame = ffmpeg_audio_decoder_free_frame;

    audio_frame_get(frame);

    return frame;
}

int ffmpeg_audio_decoder_send_pkt(struct audio_decoder *decoder, struct media_packet *pkt)
{
    int ret;
    struct ffmpeg_audio_decoder *ffmpeg_decoder = (void *)decoder;

    AVPacket tmp_pkt = {0};
    AVPacket *av_pkt = &tmp_pkt;

    if (pkt->pdata && pkt->pdata_type == MEDIA_PACKET_PDATA_FFMPEG)
        av_pkt = pkt->pdata;
    else {
        av_init_packet(av_pkt);
        media_pkt_to_ffmpeg_pkt(pkt, av_pkt);
    }

    ret = avcodec_send_packet(ffmpeg_decoder->ctx, av_pkt);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_audio_decoder: failed to send pkt, %s\n", av_err2str(ret));
        return -1;
    }

    return 0;
}

int ffmpeg_audio_decoder_get_frame(struct audio_decoder *decoder, struct audio_frame **frame)
{
    int ret;
    struct ffmpeg_audio_decoder *ffmpeg_decoder = (void *)decoder;

    AVFrame *av_frame = av_frame_alloc();

    ret = avcodec_receive_frame(ffmpeg_decoder->ctx, av_frame);
    if (ret < 0) {
        av_frame_free(&av_frame);
        if (ret == AVERROR(EAGAIN))
            return -MEDIA_EAGAIN;
        else if (ret == AVERROR_EOF)
            return -MEDIA_EOF;
        else {
            fprintf(stderr, "ffmpeg_audio_decoder: failed to receive frame: %s\n", av_err2str(ret));
            return -1;
        }
    }

    *frame = ffmpeg_audio_decoder_alloc_frame(av_frame);

    return 0;
}

struct audio_decoder_cb ffmpeg_audio_decoder_cb = {
    .open_decoder = ffmpeg_audio_decoder_open,
    .close_decoder = ffmpeg_audio_decoder_close,
    .send_pkt = ffmpeg_audio_decoder_send_pkt,
    .get_frame = ffmpeg_audio_decoder_get_frame,
};


void ffmpeg_audio_decoder_init_param(struct ffmpeg_audio_decoder_param *ffmpeg_param)
{
    struct audio_decoder_param *param = &ffmpeg_param->param;

    param->rate = ffmpeg_param->sample_rate;
    param->channels = ffmpeg_param->channels;
    param->cb = &ffmpeg_audio_decoder_cb;
    param->codec_name = (char *)ffmpeg_param->decoder_name;
}
