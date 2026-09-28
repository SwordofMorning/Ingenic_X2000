#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <time.h>
#include <stdint.h>

#include <libmedia/scale/ffmpeg_video_scaler.h>
#include <libmedia/utils/ffmpeg_utils.h>

struct ffmpeg_scaler {
    struct video_scaler scaler;
    AVFilterGraph *graph;
    AVFilterContext *scale_ctx;
    AVFilterContext *sink_ctx;
    AVFilterContext *src_ctx;

    AVFrame *frame;
    int src_width;
    int src_height;
    int src_format;
};

static AVFilterContext *create_src_ctx(
    AVFilterGraph *graph, int width, int height,
    enum AVPixelFormat format, int framerate, const char *name)
{
    const AVFilter *buffer_src = avfilter_get_by_name("buffer");
    assert(buffer_src);

	char video_args[512];

	snprintf(video_args, sizeof(video_args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		width,
		height,
		format,
		framerate, 1, 1, 1);

    AVFilterContext *src_ctx = NULL;

    int ret = avfilter_graph_create_filter(
        &src_ctx, buffer_src, name, video_args, NULL, graph);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_video_scaler: failed to create src filter: %s\n", av_err2str(ret));

    return src_ctx;
}

static AVFilterContext *create_scale_ctx(
    AVFilterGraph *graph, int width, int height, const char *name)
{
    const AVFilter *scale = avfilter_get_by_name("scale");
    assert(scale);

	char video_args[512];

	snprintf(video_args, sizeof(video_args),
		"%d:%d",
		width,
		height);

    AVFilterContext *scale_ctx = NULL;

    int ret = avfilter_graph_create_filter(
        &scale_ctx, scale, name, video_args, NULL, graph);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_video_scaler: failed to create scale filter: %s\n", av_err2str(ret));

    return scale_ctx;
}

static AVFilterContext *create_sink_ctx(
    AVFilterGraph *graph, const char *name)
{
    const AVFilter *buffer_sink = avfilter_get_by_name("buffersink");
    assert(buffer_sink);

    AVFilterContext *sink_ctx = NULL;
    AVBufferSinkParams* sink_params = av_buffersink_params_alloc();

    int ret = avfilter_graph_create_filter(
        &sink_ctx, buffer_sink, name, NULL, sink_params, graph);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_video_scaler: failed to create sink filter: %s\n", av_err2str(ret));

    av_free(sink_params);

    return sink_ctx;
}

static struct video_scaler *ffmpeg_video_scaler_open(struct video_scaler_param *param)
{
    AVFilterGraph *graph = avfilter_graph_alloc();
    assert(graph);

    AVFilterContext *sink_ctx = create_sink_ctx(graph, "scaler sink");
    assert(sink_ctx);

    AVFilterContext *scale_ctx = create_scale_ctx(graph, param->dst_width, param->dst_height, "scaler");
    assert(scale_ctx);

    int ret = avfilter_link(scale_ctx, 0, sink_ctx, 0);
    assert(!ret);

    struct ffmpeg_scaler *scaler = malloc(sizeof(*scaler));
    assert(scaler);

    scaler->graph = graph;
    scaler->sink_ctx = sink_ctx;
    scaler->scale_ctx = scale_ctx;
    scaler->src_ctx = NULL;
    scaler->frame = av_frame_alloc();

    return &scaler->scaler;
}

static void ffmpeg_video_scaler_close(struct video_scaler *scaler)
{
    struct ffmpeg_scaler *ffmpeg_scaler = (void *)scaler;

    av_frame_free(&ffmpeg_scaler->frame);

    if (ffmpeg_scaler->src_ctx)
        avfilter_free(ffmpeg_scaler->src_ctx);

    avfilter_free(ffmpeg_scaler->scale_ctx);
    avfilter_free(ffmpeg_scaler->sink_ctx);

    avfilter_graph_free(&ffmpeg_scaler->graph);

    free(ffmpeg_scaler);
}

static void ffmpeg_video_scaler_config_filter(struct ffmpeg_scaler *scaler, int src_width, int src_height, enum video_frame_format format)
{
    if (scaler->src_ctx) {
        if (scaler->src_width == src_width && scaler->src_height == src_height && scaler->src_format == format)
            return;

        avfilter_free(scaler->src_ctx);
    }

    scaler->src_ctx = create_src_ctx(scaler->graph, src_width, src_height, video_fmt_to_ffmpeg_fmt(format), 30, "scaler src");
    assert(scaler->src_ctx);

    int ret = avfilter_link(scaler->src_ctx, 0, scaler->scale_ctx, 0);
    assert(!ret);

    avfilter_config_links(scaler->src_ctx);
    avfilter_config_links(scaler->scale_ctx);

    avfilter_graph_config(scaler->graph, NULL);
    scaler->src_width = src_width;
    scaler->src_height = src_height;
    scaler->src_format = format;

    return;
}

static int ffmpeg_video_scaler_send_frame(struct video_scaler *scaler, struct video_frame *frame)
{
    struct ffmpeg_scaler *ffmpeg_scaler = (void *)scaler;

    ffmpeg_video_scaler_config_filter(ffmpeg_scaler, frame->width, frame->height, frame->format);

    AVFrame *avframe = ffmpeg_scaler->frame;

    avframe->width = frame->width;
    avframe->height = frame->height;
    int i;
    for (i = 0; i < MAX_VIDEO_FRAME_BUF_CNT; i++) {
        if (!frame->data[i])
            continue;

        avframe->linesize[i] = frame->linesize[i];
        avframe->data[i] = frame->data[i];
    }

    avframe->format = video_fmt_to_ffmpeg_fmt(frame->format);

    int ret = av_buffersrc_write_frame(ffmpeg_scaler->src_ctx, avframe);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_scaler: failed to write frame: %s\n", av_err2str(ret));

    return ret;
}

static void ffmpeg_scaler_free_frame(void *handle, struct video_frame *frame)
{
    AVFrame *av_frame = frame->pdata;
    av_frame_unref(av_frame);
    av_frame_free(&av_frame);
    video_frame_free(frame);
}

static int ffmpeg_video_scaler_get_frame(struct video_scaler *scaler, struct video_frame **dst_frame)
{
    struct ffmpeg_scaler *ffmpeg_scaler = (void *)scaler;
    AVFrame *avframe = av_frame_alloc();

    int ret = av_buffersink_get_frame(ffmpeg_scaler->sink_ctx, avframe);
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN))
            return -MEDIA_EAGAIN;
        else if (ret == AVERROR_EOF)
            return -MEDIA_EOF;
        return -1;
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
    frame->format = ffmpeg_fmt_to_video_fmt(avframe->format);

    if (frame->format == VIDEO_yuv420p) {
        struct video_frame *copy_frame = media_alloter_alloc_video_frame(
                                        NULL, frame->width, frame->height,
                                        VIDEO_nv12, 0);

        video_frame_copy(frame, copy_frame);

        video_frame_free(frame);

        *dst_frame = copy_frame;
    } else {
        frame->pdata = avframe;
        frame->handle = NULL;
        frame->put_frame = ffmpeg_scaler_free_frame;

        *dst_frame = frame;
        video_frame_get(frame);
    }

    return 0;
}

struct video_scaler_cb ffmpeg_video_scaler_cb = {
    .open_scaler = ffmpeg_video_scaler_open,
    .close_scaler = ffmpeg_video_scaler_close,
    .send_frame = ffmpeg_video_scaler_send_frame,
    .get_frame = ffmpeg_video_scaler_get_frame,
};

void ffmpeg_video_scaler_init_param(struct ffmpeg_video_scaler_param *scaler_param)
{
    struct video_scaler_param *param = &scaler_param->param;

    param->cb = &ffmpeg_video_scaler_cb;
}