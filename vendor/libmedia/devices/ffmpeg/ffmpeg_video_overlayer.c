#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <time.h>
#include <stdint.h>

#include <libmedia/overlay/ffmpeg_video_overlayer.h>
#include <libmedia/utils/ffmpeg_utils.h>

struct ffmpeg_overlayer {
    struct video_overlayer overlayer;
    AVFilterContext *bg_ctx;
    AVFilterContext *sink_ctx;

    struct {
        AVFilterContext *src_ctx;
        AVFilterContext *overlay_ctx;
    } *fg;
    int fg_len;
    AVFilterGraph *graph;
    AVFrame *frame;
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
        fprintf(stderr, "ffmpeg_video_overlayer: failed to create src filter: %s\n", av_err2str(ret));

    return src_ctx;
}

static AVFilterContext *create_overlay_ctx(
    AVFilterGraph *graph, int x, int y, const char *name)
{
    const AVFilter *overlay = avfilter_get_by_name("overlay");
    assert(overlay);

    char video_args[512];

    snprintf(video_args, sizeof(video_args),
        "x=%d:y=%d", x, y);

    AVFilterContext *overlay_ctx = NULL;

    int ret = avfilter_graph_create_filter(
        &overlay_ctx, overlay, name, video_args, NULL, graph);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_video_overlayer: failed to create overlay filter: %s\n", av_err2str(ret));

    return overlay_ctx;
}

static AVFilterContext *create_sink_ctx(
    AVFilterGraph *graph, enum AVPixelFormat format, const char *name)
{
    const AVFilter *buffer_sink = avfilter_get_by_name("buffersink");
    assert(buffer_sink);

    AVFilterContext *sink_ctx = NULL;
    enum AVPixelFormat pix_fmts[] = {format, AV_PIX_FMT_NONE};
    AVBufferSinkParams* sink_params = av_buffersink_params_alloc();
    sink_params->pixel_fmts = pix_fmts;

    int ret = avfilter_graph_create_filter(
        &sink_ctx, buffer_sink, name, NULL, sink_params, graph);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_video_overlayer: failed to create sink filter: %s\n", av_err2str(ret));

    av_free(sink_params);

    return sink_ctx;
}

static struct video_overlayer *ffmpeg_video_overlayer_open(struct video_overlayer_param *bg_param, struct video_overlayer_param *fg_param, int fg_len)
{
    struct video_overlayer_layer_info *bg_info = &bg_param->info;

    struct ffmpeg_overlayer *overlayer = malloc(sizeof(*overlayer));
    assert(overlayer);

    overlayer->fg = malloc(sizeof(*overlayer->fg) * fg_len);
    assert(overlayer->fg);

    AVFilterGraph *graph = avfilter_graph_alloc();
    assert(graph);

    enum AVPixelFormat fmt = video_fmt_to_ffmpeg_fmt(bg_info->format);
    assert(fmt >= 0);

    AVFilterContext *sink_ctx = create_sink_ctx(graph, fmt, "overlayer sink");
    assert(sink_ctx);

    AVFilterContext *bg_ctx = create_src_ctx(graph, bg_info->width, bg_info->height, fmt, 30, "overlayer bg");
    assert(bg_ctx);

    int i;
    for (i = 0; i < fg_len; i++) {
        struct video_overlayer_layer_info *info = &fg_param[i].info;
        enum AVPixelFormat fmt = video_fmt_to_ffmpeg_fmt(info->format);
        AVFilterContext *src_ctx = create_src_ctx(graph, info->width, info->height, fmt, 30, "overlayer fg");
        assert(src_ctx);
        overlayer->fg[i].src_ctx = src_ctx;
        AVFilterContext *overlay_ctx = create_overlay_ctx(graph, info->xpos, info->ypos, "overlayer");
        assert(overlay_ctx);
        overlayer->fg[i].overlay_ctx = overlay_ctx;
    }

    int ret = avfilter_link(bg_ctx, 0, overlayer->fg[0].overlay_ctx, 0);
    assert(ret >= 0);

    for (i = 0; i < fg_len; i++) {
        ret = avfilter_link(overlayer->fg[i].src_ctx, 0, overlayer->fg[i].overlay_ctx, 1);
        assert(ret >= 0);

        if (i != (fg_len - 1)) {
            ret = avfilter_link(overlayer->fg[i].overlay_ctx, 0, overlayer->fg[i+1].overlay_ctx, 0);
            assert(ret >= 0);
        }
    }

    ret = avfilter_link(overlayer->fg[fg_len-1].overlay_ctx, 0, sink_ctx, 0);
    assert(ret >= 0);

    ret = avfilter_graph_config(graph, NULL);
    assert(ret >= 0);

    overlayer->bg_ctx = bg_ctx;
    overlayer->fg_len = fg_len;
    overlayer->graph = graph;
    overlayer->sink_ctx = sink_ctx;
    overlayer->frame = av_frame_alloc();

    return &overlayer->overlayer;
}

static void ffmpeg_video_overlayer_close(struct video_overlayer *overlayer)
{
    struct ffmpeg_overlayer *ffmpeg_overlayer = (void *)overlayer;

    av_frame_free(&ffmpeg_overlayer->frame);

    int i;
    for (i = 0; i < ffmpeg_overlayer->fg_len; i++) {
        avfilter_free(ffmpeg_overlayer->fg[i].src_ctx);
        avfilter_free(ffmpeg_overlayer->fg[i].overlay_ctx);
    }
    avfilter_free(ffmpeg_overlayer->bg_ctx);
    avfilter_free(ffmpeg_overlayer->sink_ctx);

    avfilter_graph_free(&ffmpeg_overlayer->graph);

    free(ffmpeg_overlayer->fg);
    free(ffmpeg_overlayer);
}

static int ffmpeg_video_overlayer_send_bg_frame(struct video_overlayer *overlayer, struct video_frame *frame)
{
    struct ffmpeg_overlayer *ffmpeg_overlayer = (void *)overlayer;
    AVFrame *avframe = ffmpeg_overlayer->frame;

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

    avframe->pts = 0;

    int ret = av_buffersrc_write_frame(ffmpeg_overlayer->bg_ctx, avframe);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_overlayer: failed to write bg frame: %s\n", av_err2str(ret));

    return ret;
}

static int ffmpeg_video_overlayer_send_fg_frame(struct video_overlayer *overlayer, struct video_frame *frame, int fg_index)
{
    struct ffmpeg_overlayer *ffmpeg_overlayer = (void *)overlayer;
    AVFrame *avframe = ffmpeg_overlayer->frame;

    if (fg_index >= ffmpeg_overlayer->fg_len) {
        fprintf(stderr, "ffmpeg_overlayer: no support fg_index\n");
        return -1;
    }

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
    avframe->pts = 0;

    int ret = av_buffersrc_write_frame(ffmpeg_overlayer->fg[fg_index].src_ctx, avframe);
    if (ret < 0)
        fprintf(stderr, "ffmpeg_overlayer: failed to write frame: %s\n", av_err2str(ret));

    return ret;
}

static void ffmpeg_overlayer_free_frame(void *handle, struct video_frame *frame)
{
    video_frame_free(frame);
}

static int ffmpeg_video_overlayer_get_frame(struct video_overlayer *overlayer, struct video_frame **dst_frame)
{
    struct ffmpeg_overlayer *ffmpeg_overlayer = (void *)overlayer;
    AVFrame *avframe = ffmpeg_overlayer->frame;

    int ret = av_buffersink_get_frame(ffmpeg_overlayer->sink_ctx, avframe);
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
        frame->put_frame = ffmpeg_overlayer_free_frame;
        *dst_frame = frame;
        video_frame_get(frame);
    }

    return 0;
}

struct video_overlayer_cb ffmpeg_video_overlayer_cb = {
    .open_overlayer = ffmpeg_video_overlayer_open,
    .close_overlayer = ffmpeg_video_overlayer_close,
    .send_bg_frame = ffmpeg_video_overlayer_send_bg_frame,
    .send_fg_frame = ffmpeg_video_overlayer_send_fg_frame,
    .get_frame = ffmpeg_video_overlayer_get_frame,
};

void ffmpeg_video_overlayer_init_param(struct video_overlayer_param *overlayer_param)
{
    overlayer_param->cb = &ffmpeg_video_overlayer_cb;
}