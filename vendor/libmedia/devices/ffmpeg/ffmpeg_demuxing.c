#include <string.h>
#include <libmedia/media_demuxing.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libmedia/utils/ffmpeg_utils.h>
#include <libavutil/dict.h>

enum {
    AUDIO,
    VIDEO,
};

struct ffmpeg_demuxing {
    struct media_demuxing demuxing;

    AVFormatContext *fmt_ctx;
    AVCodecContext *dec_ctx;
    int stream_index[2];
};

static int init_demuxing(struct ffmpeg_demuxing *demuxing, int type)
{
    int stream_index;

    AVFormatContext *fmt_ctx = demuxing->fmt_ctx;

    enum AVMediaType media_type = type ? AVMEDIA_TYPE_VIDEO : AVMEDIA_TYPE_AUDIO;

    stream_index = av_find_best_stream(fmt_ctx, media_type, -1, -1, NULL, 0);
    if (stream_index < 0)
        return -1;

    demuxing->stream_index[type] = stream_index;

    return 0;
}

struct media_demuxing *ffmpeg_demuxing_open(struct media_demuxing_param *param)
{
    AVFormatContext *fmt_ctx = avformat_alloc_context();

    AVDictionary *options = NULL;

    int ret;
    if (strstr(param->input_file, "rtsp://") && param->is_server) {
        ret = av_dict_set(&options, "rtsp_flags", "listen", 0);
        if (ret < 0) {
            fprintf(stderr, "ffmpeg_demuxing: failed to set listen options %s\n", av_err2str(ret));
            return NULL;
        }
    }

    if (strstr(param->input_file, "tcp://") && param->is_server) {
        av_dict_set(&options, "listen", "1", 0);
        if (ret < 0) {
            fprintf(stderr, "ffmpeg_demuxing: failed to set listen options %s\n", av_err2str(ret));
            return NULL;
        }
    }

    if (strstr(param->input_file, "udp://")) {
        av_dict_set(&options, "fifo_size", "4096", 0);
        av_dict_set(&options, "overrun_nonfatal", "1", 0);
        if (ret < 0) {
            fprintf(stderr, "ffmpeg_demuxing: failed to set fifo options %s\n", av_err2str(ret));
            return NULL;
        }
    }

    ret = avformat_open_input(&fmt_ctx, param->input_file, NULL, &options);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_demuxing: Could not open input file %s, %s\n", param->input_file, av_err2str(ret));
        return NULL;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_demuxing: Faild to find stream info %s\n", av_err2str(ret));
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    struct ffmpeg_demuxing *demuxing = malloc(sizeof(*demuxing));
    assert(demuxing);

    memset(demuxing, 0, sizeof(*demuxing));

    demuxing->fmt_ctx = fmt_ctx;
    demuxing->stream_index[VIDEO] = -1;
    demuxing->stream_index[AUDIO] = -1;

    ret = init_demuxing(demuxing, VIDEO);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_demuxing: Could not find VIDEO stream\n");

    ret = init_demuxing(demuxing, AUDIO);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_demuxing: Could not find AUDIO stream\n");

    av_dump_format(demuxing->fmt_ctx, 0, param->input_file, 0);

    return &demuxing->demuxing;
}

static void ffmpeg_demuxing_close(struct media_demuxing *media_demuxing)
{
    struct ffmpeg_demuxing *demuxing = (void *)media_demuxing;

    avformat_close_input(&demuxing->fmt_ctx);

    free(demuxing);
}

static int ffmpeg_demuxing_get_video_param(struct media_demuxing *media_demuxing, struct video_decoder_param *param)
{
    struct ffmpeg_demuxing *demuxing = (void *)media_demuxing;
    int stream_index = demuxing->stream_index[VIDEO];
    if (stream_index < 0)
        return -1;
    if (!param)
        return -1;

    AVStream *st = demuxing->fmt_ctx->streams[stream_index];
    if (!st)
        return -1;

    AVCodec *codec = avcodec_find_decoder(st->codecpar->codec_id);
    param->codec_name  = (char *)codec->name;

    param->bit_rate    = st->codecpar->bit_rate;

    param->fmt     = ffmpeg_fmt_to_video_fmt(st->codecpar->format);
    param->width   = st->codecpar->width;
    param->height  = st->codecpar->height;
    param->extradata = NULL;
    param->extradata_size = 0;

    param->timebase_num = st->time_base.num;
    param->timebase_den = st->time_base.den;
    param->fps_num = st->avg_frame_rate.num;
    param->fps_den = st->avg_frame_rate.den;

    if (st->codecpar->extradata) {
        param->extradata = malloc(st->codecpar->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!param->extradata)
            return -1;

        memcpy(param->extradata, st->codecpar->extradata, st->codecpar->extradata_size);
        param->extradata_size = st->codecpar->extradata_size;
    }

    return 0;
}

static int ffmpeg_demuxing_get_audio_param(struct media_demuxing *media_demuxing, struct audio_decoder_param *param)
{
    struct ffmpeg_demuxing *demuxing = (void *)media_demuxing;
    int stream_index = demuxing->stream_index[AUDIO];

    if (stream_index < 0)
        return -1;
    if (!param)
        return -1;

    AVStream *st = demuxing->fmt_ctx->streams[stream_index];
    if (!st)
        return -1;

    AVCodec *codec = avcodec_find_decoder(st->codecpar->codec_id);
    param->codec_name  = (char *)codec->name;

    param->bit_rate    = st->codecpar->bit_rate;

    param->fmt       = ffmpeg_fmt_to_audio_fmt(st->codecpar->format);
    param->channel_layout   = st->codecpar->channel_layout;
    param->channels         = st->codecpar->channels;
    param->rate      = st->codecpar->sample_rate;

    param->extradata = NULL;
    param->extradata_size = 0;

    if (st->codecpar->extradata) {
        param->extradata = malloc(st->codecpar->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!param->extradata)
            return -1;

        memcpy(param->extradata, st->codecpar->extradata, st->codecpar->extradata_size);
        param->extradata_size = st->codecpar->extradata_size;
    }

    return 0;
}

static void demuxing_pkt_free(void *handle, struct media_packet *pkt)
{
    AVPacket *avpkt = pkt->pdata;
    av_packet_unref(avpkt);
    av_packet_free(&avpkt);
    media_packet_free(pkt);
}

int ffmpeg_demuxing_one_pkt(struct media_demuxing *media_demuxing, struct media_packet **dst_pkt)
{
    int ret = 0;
    struct ffmpeg_demuxing *demuxing = (void *)media_demuxing;

    AVFormatContext *fmt_ctx = demuxing->fmt_ctx;

    AVPacket *avpkt = av_packet_alloc();

    av_init_packet(avpkt);
    avpkt->data = NULL;
    avpkt->size = 0;

    ret = av_read_frame(fmt_ctx, avpkt);
    if (ret < 0) {
        av_packet_free(&avpkt);
        if (ret == AVERROR(EAGAIN))
            return -MEDIA_EAGAIN;
        else if (ret == AVERROR_EOF)
            return -MEDIA_EOF;
        else {
            fprintf(stderr, "ffmpeg_demuxing: failed to read frame: %s\n", av_err2str(ret));
            return -1;
        }
    }

    struct media_packet *pkt = media_packet_alloc();

    ffmpeg_pkt_to_media_pkt(avpkt, pkt);
    pkt->pdata = avpkt;
    pkt->pdata_type = MEDIA_PACKET_PDATA_FFMPEG;
    pkt->put_packet = demuxing_pkt_free;
    pkt->type = (avpkt->stream_index == demuxing->stream_index[VIDEO]) ? VIDEO_pkt_h264 : AUDIO_pkt_aac;
    pkt->stream_index = avpkt->stream_index;
    pkt->is_key_frame = avpkt->flags & AV_PKT_FLAG_KEY ? 1 : 0;
    *dst_pkt = pkt;

    media_packet_get(pkt);

    return 0;
}

static uint64_t ffmpeg_demuxing_get_pts_time(struct media_demuxing *media_demuxing, struct media_packet *pkt)
{
    struct ffmpeg_demuxing *demuxing = (void *)media_demuxing;
    AVStream *stream = demuxing->fmt_ctx->streams[pkt->stream_index];
    return av_rescale_q(pkt->pts, stream->time_base, AV_TIME_BASE_Q);
}

static int is_attached_stream(AVFormatContext *fmt_ctx, int stream_index)
{
    if (fmt_ctx->streams[stream_index]->disposition & AV_DISPOSITION_ATTACHED_PIC)
        return 1;
    return 0;
}

static uint64_t ffmpeg_demuxing_get_duration(struct media_demuxing *media_demuxing)
{
    struct ffmpeg_demuxing *demuxing = (void *)media_demuxing;
    int index = -1;

    if (demuxing->stream_index[AUDIO] >= 0)
        index = demuxing->stream_index[AUDIO];
    if (demuxing->stream_index[VIDEO] >= 0)
        if (!is_attached_stream(demuxing->fmt_ctx, demuxing->stream_index[VIDEO]))
            index = demuxing->stream_index[VIDEO];
    if (index < 0)
        return 0;

    AVStream *stream = demuxing->fmt_ctx->streams[index];
    return av_rescale_q(stream->duration, stream->time_base, AV_TIME_BASE_Q);
}

int ffmpeg_get_metadata(struct media_demuxing *media_demuxing, struct media_metadata *metadata)
{
    struct ffmpeg_demuxing *demuxing = (void *)media_demuxing;

    const AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(demuxing->fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if (strcmp(metadata->key, tag->key) == 0) {
            metadata->value = tag->value;
            return 0;
        }
    }
    return -1;
}

static int ffmpeg_demuxing_seek_forward(struct media_demuxing *media_demuxing, int64_t abs_us)
{
    struct ffmpeg_demuxing *demuxing = (void *)media_demuxing;

    int stream_index = demuxing->stream_index[AUDIO];
    if (demuxing->stream_index[VIDEO] >= 0)
        if (!is_attached_stream(demuxing->fmt_ctx, demuxing->stream_index[VIDEO]))
            stream_index = demuxing->stream_index[VIDEO];

    if (stream_index < 0) {
        fprintf(stderr, "ffmpeg_demuxing:failed to seek forward, no video and audio stream\n");
        return -1;
    }

    AVStream *stream = demuxing->fmt_ctx->streams[stream_index];

    int64_t time = av_rescale_q(abs_us, AV_TIME_BASE_Q, stream->time_base);

    int ret = av_seek_frame(demuxing->fmt_ctx, stream_index, time, 0);

    return ret >= 0 ? 0 : -1;
}

static int ffmpeg_demuxing_seek_backward(struct media_demuxing *media_demuxing, int64_t abs_us)
{
    struct ffmpeg_demuxing *demuxing = (void *)media_demuxing;

    int stream_index = demuxing->stream_index[AUDIO];
    if (demuxing->stream_index[VIDEO] >= 0)
        if (!is_attached_stream(demuxing->fmt_ctx, demuxing->stream_index[VIDEO]))
            stream_index = demuxing->stream_index[VIDEO];

    if (stream_index < 0) {
        fprintf(stderr, "ffmpeg_demuxing:failed to seek backward, no video and audio stream\n");
        return -1;
    }

    AVStream *stream = demuxing->fmt_ctx->streams[stream_index];

    int64_t time = av_rescale_q(abs_us, AV_TIME_BASE_Q, stream->time_base);
    if (time < 0)
        time = 0;

    int ret = av_seek_frame(demuxing->fmt_ctx, stream_index, time, AVSEEK_FLAG_BACKWARD);

    return ret >= 0 ? 0 : -1;
}

struct media_demuxing_cb ffmpeg_demuxing_cb = {
    .open_demuxing = ffmpeg_demuxing_open,
    .close_demuxing = ffmpeg_demuxing_close,
    .get_video_param = ffmpeg_demuxing_get_video_param,
    .get_audio_param = ffmpeg_demuxing_get_audio_param,
    .demuxing_packet = ffmpeg_demuxing_one_pkt,
    .get_pts = ffmpeg_demuxing_get_pts_time,
    .get_duration = ffmpeg_demuxing_get_duration,
    .get_metadata = ffmpeg_get_metadata,
    .seek_forward = ffmpeg_demuxing_seek_forward,
    .seek_backward = ffmpeg_demuxing_seek_backward,
};

void ffmpeg_demuxing_init_param(struct media_demuxing_param *param)
{
    param->cb = &ffmpeg_demuxing_cb;
}