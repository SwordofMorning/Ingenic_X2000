#include <libmedia/video_rotater.h>
#include <libmedia/media_errno.h>

#include <libhardware2/fb.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <lib2d/ingenic2d.h>


struct hw_video_2d_rotater {
    struct video_rotater rotater;
    struct ingenic_2d *ingenic_2d;
};

static int video_fmt_to_2d_fmt(enum video_frame_format video_format)
{
    switch (video_format) {
    case VIDEO_bgra:    return INGENIC_2D_ARGB8888;
    default:
        fprintf(stderr, "hw 2d rotater: not support this video fmt: %d\n", video_format);
        return -1;
    }
}

static struct video_rotater *hw_video_2d_rotater_open(struct video_rotater_param *param)
{
    struct ingenic_2d *ingenic_2d = ingenic_2d_open();
    if (!ingenic_2d) {
        fprintf(stderr, "hw 2d rotater: open ingenic_2d failed\n");
        return NULL;
    }

    struct hw_video_2d_rotater *hw_rotater = malloc(sizeof(*hw_rotater));
    hw_rotater->ingenic_2d = ingenic_2d;

    return &hw_rotater->rotater;
}

static void hw_video_2d_rotater_close(struct video_rotater *rotater)
{
    struct hw_video_2d_rotater *hw_rotater = (void *)rotater;

    ingenic_2d_close(hw_rotater->ingenic_2d);

    free(hw_rotater);
}

static void ingenic_2d_rotator_free_frame(void *handle, struct video_frame *dst_frame)
{
    struct ingenic_2d_frame *dst_2d_frame = dst_frame->pdata;
    ingenic_2d_free_frame(handle, dst_2d_frame);

    video_frame_free(dst_frame);
}

static struct video_frame *rotator_by_2d(struct hw_video_2d_rotater *rotater, struct video_frame *src_frame, enum video_rotate_angle angle)
{
    struct ingenic_2d *ingenic_2d = rotater->ingenic_2d;
    struct ingenic_2d_frame *src_2d_frame;
    struct ingenic_2d_frame *dst_2d_frame;

    int src_width = src_frame->width;
    int src_height = src_frame->height;
    int dst_width = (angle == rotate_90 || angle == rotate_270) ? src_frame->height : src_frame->width;
    int dst_height = (angle == rotate_90 || angle == rotate_270) ? src_frame->width : src_frame->height;

    int src_2d_fmt = video_fmt_to_2d_fmt(src_frame->format);
    if (src_2d_fmt == -1)
        return NULL;
    int dst_2d_fmt = video_fmt_to_2d_fmt(VIDEO_bgra);

    /* src_2d_frame have 2 way to alloc */
    if (src_frame->is_phys)
        src_2d_frame = ingenic_2d_alloc_frame_by_user(ingenic_2d, src_width, src_height, src_2d_fmt,
                                                        src_frame->phys_data[0], src_frame->data[0], src_frame->total_size);
    else {
        src_2d_frame = ingenic_2d_alloc_frame(ingenic_2d, src_width, src_height, src_2d_fmt);
        memcpy(src_2d_frame->addr[0], src_frame->data[0], src_frame->total_size);
    }

    if (!src_2d_frame) {
        fprintf(stderr, "hw 2d rotater: alloc src ingenic_2d_frame failed\n");
        return NULL;
    }

    /* dst_2d_frame only have onw way to alloc */
    dst_2d_frame = ingenic_2d_alloc_frame(ingenic_2d, dst_width, dst_height, dst_2d_fmt);
    if (!dst_2d_frame) {
        fprintf(stderr, "hw 2d rotater: alloc dst ingenic_2d_frame failed\n");
        ingenic_2d_free_frame(ingenic_2d, src_2d_frame);
        return NULL;
    }

    struct ingenic_2d_rect src_rect = ingenic_2d_rect_init(src_2d_frame, 0, 0, src_width, src_height);
    struct ingenic_2d_rect dst_rect = ingenic_2d_rect_init(dst_2d_frame, 0, 0, 0, 0);/* 目前dst的 x, y, w, h全失效*/

    int ret = ingenic_2d_rotate(ingenic_2d, angle, &src_rect, &dst_rect);
    if (ret < 0) {
        fprintf(stderr, "hw 2d rotater: failed\n");
        ingenic_2d_free_frame(ingenic_2d, src_2d_frame);
        ingenic_2d_free_frame(ingenic_2d, dst_2d_frame);
        return NULL;
    }
    struct video_frame *dst_frame = video_frame_alloc();
    ret = video_frame_init(dst_frame, dst_width, dst_height, VIDEO_bgra, 0,
                           dst_2d_frame->addr[0], (unsigned long)dst_2d_frame->phyaddr[0], ingenic_2d, ingenic_2d_rotator_free_frame);
    if (ret < 0) {
        fprintf(stderr, "hw 2d rotater: failed to init video_frame\n");
        ingenic_2d_free_frame(ingenic_2d, src_2d_frame);
        ingenic_2d_free_frame(ingenic_2d, dst_2d_frame);
        free(dst_frame);
        return NULL;
    }

    ingenic_2d_free_frame(ingenic_2d, src_2d_frame);

    dst_frame->pdata = dst_2d_frame;
    video_frame_get(dst_frame);

    return dst_frame;
}

int hw_video_2d_rotater_convert_frame(struct video_rotater *rotater, struct video_frame *src_frame, struct video_frame **dst_frame)
{
    assert(src_frame);

    struct hw_video_2d_rotater *hw_rotater = (void *)rotater;

    enum video_rotate_angle angle = rotater->param.rotate_angle;
    int hflip = rotater->param.hflip;
    int vflip = rotater->param.vflip;

    if (hflip == 1 || vflip == 1) {
        fprintf(stderr, "hw 2d rotater: Do not support flip temporarily\n");
        return -1;
    }

    struct video_frame * rotator_frame = rotator_by_2d(hw_rotater, src_frame, angle);
    if (!rotator_frame)
        return -MEDIA_EAGAIN;

    *dst_frame = rotator_frame;

    return 0;
}

struct video_rotater_cb hw_video_2d_rotater_cb = {
    .open_rotater = hw_video_2d_rotater_open,
    .close_rotater = hw_video_2d_rotater_close,
    .convert_video_frame = hw_video_2d_rotater_convert_frame,
};

void hw_video_2d_rotater_init_param(struct video_rotater_param *param)
{
    param->cb = &hw_video_2d_rotater_cb;
}