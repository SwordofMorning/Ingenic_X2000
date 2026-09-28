#include <libmedia/rotate_video.h>
#include <libmedia/video_frame.h>
#include <errno.h>

int video_frame_rotate(struct video_frame *src, struct video_frame *dst, float angle, uint32_t color)
{
    assert(src->format == dst->format);

    int (*rotate_ops)(
        uint8_t **src, uint32_t *src_linesz, int src_width, int src_height,
        uint8_t **dst, uint32_t *dst_linesz, int dst_width, int dst_height,
        float angle, uint32_t color) = NULL;

    switch (src->format) {
        case VIDEO_nv12:  rotate_ops = rotate_nv12; break;
        case VIDEO_y16:   rotate_ops = rotate_y16; break;
        case VIDEO_y8:    rotate_ops = rotate_y8; break;
        case VIDEO_alpha: rotate_ops = rotate_y8; break;
        case VIDEO_bgra:  rotate_ops = rotate_bgra; break;
        default: break;
    }

    if (!rotate_ops) {
        fprintf(stderr, "rotate: not support: %s to %s\n", video_fmt_name(src->format), video_fmt_name(dst->format));
        return -1;
    }

    return rotate_ops(src->data, src->linesize, src->width, src->height,
            dst->data, dst->linesize, dst->width, dst->height, angle, color);
}
