
#include <libmedia/resample/ffmpeg_audio_resampler.h>
#include <libmedia/utils/ffmpeg_utils.h>

struct ffmpeg_resampler_param {
    int src_sample_rate;
    enum AVSampleFormat src_sample_fmt;
    uint64_t src_channel_layout;

    int dst_sample_rate;
    enum AVSampleFormat dst_sample_fmt;
    uint64_t dst_channel_layout;
};


struct ffmpeg_audio_resampler {
    struct audio_resampler resampler;

    struct ffmpeg_resampler_param ffmpeg_param;
    AVFilterGraph *graph;
    AVFilterContext *src_ctx;
    AVFilterContext *sink_ctx;
    AVFilterContext *aformat_ctx;

    AVFrame *frame_in;
};

static AVFilterContext *create_src_ctx(struct ffmpeg_resampler_param *param, AVFilterGraph *graph)
{
    char args[512];

    snprintf(args, sizeof(args),
            "sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
             param->src_sample_rate, av_get_sample_fmt_name(param->src_sample_fmt), param->src_channel_layout);

    AVFilterContext *src_ctx = NULL;

    int ret = avfilter_graph_create_filter(&src_ctx, avfilter_get_by_name("abuffer"), "in",
                                       args, NULL, graph);
    if (ret < 0)
        return NULL;

    return src_ctx;
}

static AVFilterContext *create_aformat_ctx(struct ffmpeg_resampler_param *param, AVFilterGraph *graph)
{
    char args[512];

    AVFilterContext *aformat_ctx = NULL;

    snprintf(args, sizeof(args),
            "sample_fmts=%s:sample_rates=%d:channel_layouts=0x%"PRIx64,
             av_get_sample_fmt_name(param->dst_sample_fmt),
             param->dst_sample_rate,
             param->dst_channel_layout);

    int ret = avfilter_graph_create_filter(&aformat_ctx, avfilter_get_by_name("aformat"),
                                    "format_out", args, NULL, graph);
    if (ret < 0)
        return NULL;

    return aformat_ctx;
}

static AVFilterContext *create_sink_ctx(struct ffmpeg_resampler_param *param, AVFilterGraph *graph)
{
    AVFilterContext *sink_ctx = NULL;

    enum AVSampleFormat out_sample_fmts[] = { param->dst_sample_fmt, -1 };
    int64_t out_channel_layouts[] = { param->dst_channel_layout, -1 };
    int out_sample_rates[] = { param->dst_sample_rate, -1 };

    int ret = avfilter_graph_create_filter(&sink_ctx, avfilter_get_by_name("abuffersink"),
                                "out", NULL, NULL, graph);
    if (ret < 0)
        return NULL;

    ret = av_opt_set_int_list(sink_ctx, "sample_fmts", out_sample_fmts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        fprintf(stderr, "[ffmpeg_resampler2] Cannot set output sample format: %s\n", av_err2str(ret));
        goto end;
    }

    ret = av_opt_set_int_list(sink_ctx, "channel_layouts", out_channel_layouts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        fprintf(stderr, "[ffmpeg_resampler2] Cannot set output channel layout: %s\n", av_err2str(ret));
        goto end;
    }

    ret = av_opt_set_int_list(sink_ctx, "sample_rates", out_sample_rates, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        fprintf(stderr, "[ffmpeg_resampler2] Cannot set output sample rate: %s\n", av_err2str(ret));
        goto end;
    }

    return sink_ctx;

end:
    avfilter_free(sink_ctx);

    return NULL;
}

static void init_ffmpeg_frame(struct audio_frame *frame, AVFrame *av_frame)
{
    int i;

    for(i = 0; i < MAX_AUDIO_FRAME_BUF_CNT; i++) {
        if(frame->data[i])
            av_frame->data[i] = frame->data[i];
    }

    av_frame->channel_layout = av_get_default_channel_layout(frame->channels);
    av_frame->channels = frame->channels;
    av_frame->nb_samples = frame->nb_samples;
    av_frame->sample_rate = frame->sample_rate;
    av_frame->format = audio_fmt_to_ffmpeg_fmt(frame->format);

}

void ffmpeg_audio_resampler_free_frame(void *handle, struct audio_frame *frame)
{
    AVFrame *av_frame = frame->pdata;
    av_frame_unref(av_frame);
    av_frame_free(&av_frame);
    audio_frame_free(frame);
}

static struct audio_frame *ffmpeg_audio_resampler_alloc_frame(AVFrame *av_frame)
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
    frame->put_frame = ffmpeg_audio_resampler_free_frame;

    audio_frame_get(frame);

    return frame;
}

static void init_ffmpeg_resampler_param(struct audio_resampler_param *audio_param, struct ffmpeg_resampler_param *ffmpeg_param)
{

    ffmpeg_param->src_channel_layout = av_get_default_channel_layout(audio_param->src_channels);
    ffmpeg_param->src_sample_fmt = audio_fmt_to_ffmpeg_fmt(audio_param->src_format);
    ffmpeg_param->src_sample_rate = audio_param->src_rate;

    ffmpeg_param->dst_channel_layout = av_get_default_channel_layout(audio_param->dst_channels);
    ffmpeg_param->dst_sample_fmt = audio_fmt_to_ffmpeg_fmt(audio_param->dst_format);
    ffmpeg_param->dst_sample_rate = audio_param->dst_rate;
}


static struct audio_resampler *ffmpeg_audio_resampler_create(struct audio_resampler_param *param)
{
    int ret = 0;

    struct ffmpeg_audio_resampler *resampler = malloc(sizeof(*resampler));
    memset(resampler, 0, sizeof(*resampler));

    struct ffmpeg_resampler_param *ffmpeg_param = &resampler->ffmpeg_param;
    init_ffmpeg_resampler_param(param, ffmpeg_param);

    AVFilterGraph *graph = avfilter_graph_alloc();
    if (!graph) {
        fprintf(stderr, "[ffmpeg_resampler2] avfilter_graph_alloc err\n");
        return NULL;
    }

    AVFilterContext *src_ctx = create_src_ctx(ffmpeg_param, graph);
    if (!src_ctx) {
        fprintf(stderr, "[ffmpeg_resampler2] create_src_ctx err\n");
        goto free_graph;
    }

    AVFilterContext *aformat_ctx = create_aformat_ctx(ffmpeg_param, graph);
    if (!aformat_ctx) {
        fprintf(stderr, "[ffmpeg_resampler2] create_aformat_ctx err\n");
        goto free_src_ctx;
    }

    AVFilterContext *sink_ctx = create_sink_ctx(ffmpeg_param, graph);
    if (!sink_ctx) {
        fprintf(stderr, "[ffmpeg_resampler2] create_sink_ctx err\n");
        goto free_aformat_ctx;
    }

    ret = ret ? ret : avfilter_link(src_ctx, 0, aformat_ctx, 0);
    ret = ret ? ret : avfilter_link(aformat_ctx, 0, sink_ctx, 0);
    ret = ret ? ret : avfilter_graph_config(graph, NULL);
    if (ret < 0) {
        fprintf(stderr, "[ffmpeg_resampler2] avfilter_graph_config err: %s\n", av_err2str(ret));
        goto free_sink_ctx;
    }

    resampler->graph = graph;
    resampler->src_ctx = src_ctx;
    resampler->aformat_ctx = aformat_ctx;
    resampler->sink_ctx = sink_ctx;
    resampler->frame_in = av_frame_alloc();

    return &resampler->resampler;

free_sink_ctx:
    avfilter_free(sink_ctx);
free_aformat_ctx:
    avfilter_free(aformat_ctx);
free_src_ctx:
    avfilter_free(src_ctx);
free_graph:
    avfilter_graph_free(&graph);

    free(resampler);

    return NULL;
}

static int ffmpeg_audio_resampler_convert(struct audio_resampler *resampler, struct audio_frame *frame_in, struct audio_frame **frame_out)
{
    struct ffmpeg_audio_resampler *ffmpeg_resampler = (void *)resampler;

    AVFrame *av_frame_in = ffmpeg_resampler->frame_in;

    init_ffmpeg_frame(frame_in, av_frame_in);

    int ret = av_buffersrc_add_frame_flags(ffmpeg_resampler->src_ctx, av_frame_in, 0);
    if (ret < 0) {
        fprintf(stderr, "ffmpeg_audio_resampler: av_buffersrc_add_frame_flags err:%s\n", av_err2str(ret));
        return -1;
    }

    AVFrame *av_frame_out = av_frame_alloc();

    ret = av_buffersink_get_frame(ffmpeg_resampler->sink_ctx, av_frame_out);
    if (ret == AVERROR(EAGAIN)) {
        av_frame_free(&av_frame_out);
        return 1;
    }

    if (ret < 0) {
        fprintf(stderr, "[ffmpeg_resampler2] av_buffersink_get_frame err: %s\n", av_err2str(ret));
        av_frame_free(&av_frame_out);
        return -1;
    }

    ret = 0;
    *frame_out = ffmpeg_audio_resampler_alloc_frame(av_frame_out);

    return ret;

}

static void ffmpeg_audio_resampler_delete(struct audio_resampler *resampler)
{
    struct ffmpeg_audio_resampler *ffmpeg_resampler = (void *)resampler;
    av_frame_free(&ffmpeg_resampler->frame_in);

    avfilter_free(ffmpeg_resampler->src_ctx);
    avfilter_free(ffmpeg_resampler->aformat_ctx);
    avfilter_free(ffmpeg_resampler->sink_ctx);
    avfilter_graph_free(&ffmpeg_resampler->graph);

    free(resampler);
}


struct audio_resampler_cb ffmpeg_audio_resampler_cb = {
    .create_resampler = ffmpeg_audio_resampler_create,
    .convert_audio = ffmpeg_audio_resampler_convert,
    .delete_resampler = ffmpeg_audio_resampler_delete,
};


void ffmpeg_audio_resampler_init_param(struct audio_resampler_param *param)
{
    param->cb = &ffmpeg_audio_resampler_cb;
}

void ffmpeg_audio_resample_init_default_param(
    struct audio_resampler_param *param, int channels, int rate, 
    enum audio_frame_format src_fmt, enum audio_frame_format dst_fmt)
{
    memset(param, 0, sizeof(*param));

    param->src_channels = channels;
    param->src_format = src_fmt;
    param->src_rate = rate;

    param->dst_channels = channels;
    param->dst_format = dst_fmt;
    param->dst_rate = rate;
    ffmpeg_audio_resampler_init_param(param);
}
