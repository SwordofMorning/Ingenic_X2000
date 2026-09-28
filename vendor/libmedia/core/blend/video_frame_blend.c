#include <libmedia/video_frame.h>
#include <libmedia/blend_video.h>
#include <assert.h>

void (*blend_func_list[][VIDEO_nums])(uint8_t **, uint32_t *, uint8_t **, uint32_t *, int, int, uint8_t) = {
    [VIDEO_y8][VIDEO_y8] = blend_y8,
    [VIDEO_nv12][VIDEO_nv12] = blend_nv12,
    [VIDEO_yuv420p][VIDEO_yuv420p] = blend_yuv420p,
    [VIDEO_yuvj420p][VIDEO_yuvj420p] = blend_yuv420p,
    [VIDEO_yuvj422p][VIDEO_yuvj422p] = blend_yuv422p,
    [VIDEO_yuvj444p][VIDEO_yuvj422p] = blend_yuv444p,
    [VIDEO_yuv422p][VIDEO_yuv422p] = blend_yuv422p,
    [VIDEO_yuv444p][VIDEO_yuv422p] = blend_yuv444p,
    [VIDEO_yuva420p][VIDEO_nv12] = blend_yuva420p_to_nv12,
    [VIDEO_bgra][VIDEO_nv12] = blend_bgra_to_nv12,
    [VIDEO_bgra][VIDEO_bgra] = blend_bgra_to_bgra,
};

void video_frame_blend_alpha_color(
    struct video_frame *src, unsigned int color, struct video_frame *dst, int x, int y)
{
    assert(src->format == VIDEO_alpha);

    struct video_frame s, d;

    int ret = video_frame_crop_src_dst(
        src, dst, x, y, src->width, src->height, &s, &d);
    if (ret)
        return;

    if (d.format == VIDEO_nv12)
        return blend_alpha_color_nv12(
            s.data[0], s.linesize[0], d.data[0], d.linesize[0],
            d.data[1], d.linesize[1], s.width, s.height, color);
    if (d.format == VIDEO_bgra)
        return blend_alpha_color_bgra(
            s.data[0], s.linesize[0], d.data[0], d.linesize[0],
            s.width, s.height, color
        );
    fprintf(stderr, "video frame blend: not support blend alpha color to %s\n",\
                    video_fmt_name(d.format));
}

void video_frame_blend(
    struct video_frame *src, struct video_frame *dst, int x, int y, uint8_t alpha)
{
    struct video_frame s, d;

    int ret = video_frame_crop_src_dst(
        src, dst, x, y, src->width, src->height, &s, &d);
    if (ret)
        return;

    void (*blend_func)(uint8_t **, uint32_t *, uint8_t **, uint32_t *, int, int, uint8_t) = blend_func_list[s.format][d.format];

    if (!blend_func) {
        fprintf(stderr, "video frame blend: not support %s blend to %s\n",\
                video_fmt_name(s.format),  video_fmt_name(d.format));
        return;
    }

    blend_func(s.data, s.linesize, d.data, d.linesize, s.width, s.height, alpha);
}