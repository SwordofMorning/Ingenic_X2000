#include <libmedia/muxing/ffmpeg_muxing.h>
#include <libutils2/boot_time.h>
#include <libmedia/utils/ffmpeg_utils.h>
#include <pthread.h>

enum {
    VIDEO,
    AUDIO,
};

struct ffmpeg_muxing {
    struct media_muxing muxing;
    AVFormatContext *fmt_ctx;
    AVStream *stream[2];
    const char *output_file;
    int is_open_file;
    int is_write_header;
    int is_muxing[2];

    int stream_index[2];

    volatile int64_t start_time[2];
    int last_time[2];
    AVRational time_base[2];

    pthread_mutex_t mutex;
};

static int check_write_header(struct ffmpeg_muxing *muxing, int index)
{
    int ret = 0;
    AVFormatContext *fmt_ctx = muxing->fmt_ctx;

    if (!muxing->is_open_file) {
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            ret = avio_open(&fmt_ctx->pb, muxing->output_file, AVIO_FLAG_WRITE);
            if (ret < 0) {
                fprintf(stderr, "ffmpeg_muxing: failed to open '%s': %s\n", muxing->output_file,
                        av_err2str(ret));
                return -1;
            }
        }
        muxing->is_open_file = 1;
    }

    if (!muxing->is_write_header) {
        ret = avformat_write_header(fmt_ctx, NULL);
        if (ret < 0) {
            fprintf(stderr, "ffmpeg_muxing: failed to open output file: %s\n",
                    av_err2str(ret));
            return -1;
        }
        muxing->is_write_header = 1;
    }

    if (!muxing->is_muxing[index]) {
        muxing->start_time[index] = boot_time_usecs();
        muxing->is_muxing[index] = 1;
    }

    return 0;
}

static int write_format_pkt(
    struct ffmpeg_muxing *muxing, struct media_packet *pkt_, int index, int has_ts)
{
    int ret;

    AVFormatContext *fmt_ctx = muxing->fmt_ctx;
    AVStream *stream = muxing->stream[index];
    AVPacket *pkt = av_packet_alloc();
    av_init_packet(pkt);

    pkt->data = pkt_->data;
    pkt->size = pkt_->size;

    int64_t now = boot_time_usecs();
    int64_t time_us = now - muxing->start_time[index];

    pkt->dts = av_rescale_q(time_us, AV_TIME_BASE_Q, stream->time_base);
    pkt->pts = pkt->dts;
    pkt->duration = 0;
    pkt->stream_index = muxing->stream_index[index];

    ret = av_write_frame(fmt_ctx, pkt);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_muxing: pkt_ -> type = %d, failed to write frame: %s\n",pkt_->type, av_err2str(ret));

    av_packet_unref(pkt);
    av_packet_free(&pkt);

    return ret;
}

static int to_codec_id(enum media_packet_type pkt_type)
{
    switch (pkt_type) {
        case AUDIO_pkt_aac: return AV_CODEC_ID_AAC;
        case VIDEO_pkt_h264: return AV_CODEC_ID_H264;
        case VIDEO_pkt_mjpeg: return AV_CODEC_ID_MJPEG;
        default:
            fprintf(stderr, "ffmpeg muxing: not support this pkt type: %d\n", pkt_type);
            return -1;
    }
}

static int init_video(struct ffmpeg_muxing *formater, struct media_muxing_video_param *param)
{
    if (!param->enable)
        return 0;

    enum AVCodecID codec_id = to_codec_id(param->type);
    if (codec_id < 0)
        return -1;

    enum AVPixelFormat fmt = video_fmt_to_ffmpeg_fmt(param->fmt);
    if (fmt < 0)
        return -1;

    AVStream *stream = avformat_new_stream(formater->fmt_ctx, NULL);
    if (!stream) {
        fprintf(stderr, "ffmpeg_muxing: avformat_new_stream failed\n");
        return -1;
    }

    stream->codecpar->width = param->width;
    stream->codecpar->height = param->height;
    stream->codecpar->format = fmt;
    stream->codecpar->bit_rate = param->bit_rate;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->codec_id = codec_id;
    stream->time_base = (AVRational){1, param->framerate * 3};

    if (param->extradata) {
        stream->codecpar->extradata = malloc(param->extradata_size);
        if (!stream->codecpar->extradata)
            return -1;

        memcpy(stream->codecpar->extradata, param->extradata, param->extradata_size);
        stream->codecpar->extradata_size = param->extradata_size;
    }


    formater->stream[VIDEO] = stream;

    formater->start_time[VIDEO] = boot_time_usecs();
    formater->last_time[VIDEO] = formater->start_time[VIDEO];
    formater->time_base[VIDEO] = (AVRational){1, param->framerate * 3};
    formater->stream_index[VIDEO] = stream->index;

    return 0;
}

static int init_audio(struct ffmpeg_muxing *formater, struct media_muxing_audio_param *param)
{
    if (!param->enable)
        return 0;

    enum AVCodecID codec_id = to_codec_id(param->type);
    if (codec_id < 0)
        return -1;

    enum AVSampleFormat fmt = audio_fmt_to_ffmpeg_fmt(param->fmt);
    if (fmt < 0)
        return -1;

    AVStream *stream = avformat_new_stream(formater->fmt_ctx, NULL);
    if (!stream) {
        fprintf(stderr, "ffmpeg_muxing: avformat_new_stream failed\n");
        return -1;
    }

    stream->codecpar->channels = param->channels;
    stream->codecpar->format = fmt;
    stream->codecpar->channel_layout = av_get_default_channel_layout(param->channels);
    stream->codecpar->sample_rate = param->sample_rate;
    stream->codecpar->bit_rate = param->bit_rate;
    stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    stream->codecpar->codec_id = codec_id;
    stream->time_base = (AVRational){1, param->sample_rate};

    formater->stream[AUDIO] = stream;
    formater->start_time[AUDIO] = boot_time_usecs();
    formater->last_time[AUDIO] = formater->start_time[AUDIO];
    formater->time_base[AUDIO] = (AVRational){1, param->sample_rate};
    formater->stream_index[AUDIO] = stream->index;

    return 0;
}

static struct media_muxing *ffmpeg_muxing_open(struct media_muxing_param *param)
{
    AVFormatContext *fmt_ctx;

    int ret = avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, param->output_file);
    if (ret) {
        fprintf(stderr, "ffmpeg_muxing: failed to open output context: %s, param->output_file = %s\n", av_err2str(ret), param->output_file);
        return NULL;
    }

    struct ffmpeg_muxing *formater = malloc(sizeof(*formater));
    assert(formater);

    memset(formater, 0, sizeof(*formater));

    pthread_mutex_init(&formater->mutex, NULL);

    formater->fmt_ctx = fmt_ctx;

    ret = init_video(formater, &param->video_param);
    if (ret)
        goto init_video_err;


    ret = init_audio(formater, &param->audio_param);
    if (ret)
        goto init_audio_err;

    formater->output_file = strdup(param->output_file);

    return &formater->muxing;

init_audio_err:
init_video_err:
    avformat_free_context(fmt_ctx);

    free(formater);

    return NULL;
}

static void ffmpeg_muxing_close(struct media_muxing *muxing)
{
    struct ffmpeg_muxing *formater = (void *)muxing;
    AVFormatContext *fmt_ctx = formater->fmt_ctx;

    int ret;

    if (formater->is_write_header) {
        ret = av_write_trailer(fmt_ctx);
        if (ret)
            fprintf(stderr, "ffmpeg_muxing: failed to write trailer: %d\n", ret);
    }

    if (formater->is_open_file) {
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&fmt_ctx->pb);
    }

    avformat_free_context(fmt_ctx);

    if (muxing->param.video_param.extradata)
        free(muxing->param.video_param.extradata);

    free((void *)formater->output_file);

    pthread_mutex_destroy(&formater->mutex);

    free(formater);
}

static int ffmpeg_muxing_one_video_pkt(struct media_muxing *muxing, struct media_packet *pkt)
{
    int ret;

    struct ffmpeg_muxing *formater = (void *)muxing;

    ret = check_write_header(formater, VIDEO);
    if (ret < 0)
        return -1;

    ret = write_format_pkt(formater, pkt, VIDEO, 0);
    if (ret < 0)
        return -1;

    return 0;
}

static int ffmpeg_muxing_one_audio_pkt(struct media_muxing *muxing, struct media_packet *pkt)
{
    int ret;

    struct ffmpeg_muxing *formater = (void *)muxing;

    ret = check_write_header(formater, AUDIO);
    if (ret < 0)
        return -1;

    ret = write_format_pkt(formater, pkt, AUDIO, 0);
    if (ret < 0)
        return -1;

    return 0;
}

static int is_video_pkt(struct media_packet *pkt)
{
    if (pkt->type >= VIDEO_pkt_h264)
        return 1;

    return 0;
}

static int ffmpeg_muxing_one_pkt(struct media_muxing *muxing, struct media_packet *pkt)
{
    int ret = 0;

    struct ffmpeg_muxing *formater = (void *)muxing;

    pthread_mutex_lock(&formater->mutex);

    if (is_video_pkt(pkt))
        ret = ffmpeg_muxing_one_video_pkt(muxing, pkt);
    else
        ret = ffmpeg_muxing_one_audio_pkt(muxing, pkt);

    pthread_mutex_unlock(&formater->mutex);
    return ret;
}

struct media_muxing_cb ffmpeg_muxing_cb = {
    .open_muxing = ffmpeg_muxing_open,
    .close_muxing = ffmpeg_muxing_close,
    .muxing_packet = ffmpeg_muxing_one_pkt,
};

void ffmpeg_muxing_init_param(struct media_muxing_param *param)
{
    param->cb = &ffmpeg_muxing_cb;
}