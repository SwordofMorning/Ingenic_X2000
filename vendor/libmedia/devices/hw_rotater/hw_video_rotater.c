#include <libmedia/video_rotater.h>

#include <libhardware2/rotator.h>
#include <libhardware2/fb.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <libmedia/rotate/hw_video_rotater.h>

struct hw_video_rotater {
    struct video_rotater rotater;

    int rotate_fd;
};

#define ALIGN(x, n) (((x) + (n) - 1) - ((x) + (n) - 1) % (n))

static int bytes_per_pixle(enum fb_fmt format)
{
    switch (format) {
        case fb_fmt_RGB555:
        case fb_fmt_RGB565:
            return 2;
        case fb_fmt_RGB888:
        case fb_fmt_ARGB8888:
            return 4;
        case fb_fmt_yuv422:
            return 2;
        case fb_fmt_NV12:
        case fb_fmt_NV21:
            return 1;
        default:
            fprintf(stderr, "not support this foramt:%d\n", format);
            return 0;
    }
}

static int bytes_per_line(int xres, enum fb_fmt format)
{
    int len = xres * bytes_per_pixle(format);

    return ALIGN(len, 8);
}

static int rotate_mem(struct hw_video_rotater *rotater,
     int width, int height, enum fb_fmt fmt,
     void *src_phys, int src_linesize, void *dst_phys)
{
    struct rotator_config_data config;
    memset(&config, 0, sizeof(config));

    switch (fmt) {
    case fb_fmt_NV12:
    case fb_fmt_NV21:
        config.src_fmt = ROTATOR_NV12;
        break;
    case fb_fmt_yuv422:
        config.src_fmt = ROTATOR_YUV422;
        break;
    default:
        config.src_fmt = ROTATOR_ARGB8888;
        break;
    }

    enum video_rotate_angle angle = rotater->rotater.param.rotate_angle;
    int hflip = rotater->rotater.param.hflip;
    int vflip = rotater->rotater.param.vflip;

    if (!src_linesize)
        src_linesize = bytes_per_line(width, fmt);

    int dst_linesize = bytes_per_line(width, fmt);

    switch (angle) {
        case rotate_0:
            config.rotate_angle = ROTATOR_ANGLE_0;
            break;
        case rotate_90:
            config.rotate_angle = ROTATOR_ANGLE_90;
            dst_linesize = bytes_per_line(height, fmt);;
            break;
        case rotate_180:
            config.rotate_angle = ROTATOR_ANGLE_180;
            break;
        case rotate_270:
            config.rotate_angle = ROTATOR_ANGLE_270;
            dst_linesize = bytes_per_line(height, fmt);;
            break;
        default:
            config.rotate_angle = ROTATOR_ANGLE_0;
            break;
    }

    config.dst_fmt = config.src_fmt;
    config.frame_width        = width;
    config.frame_height       = height;
    config.src_stride         = src_linesize/bytes_per_pixle(fmt);
    config.dst_stride         = dst_linesize/bytes_per_pixle(fmt);
    config.convert_color      = ROTATOR_ORDER_RGB_TO_RGB; //RGB→RGB
    config.horizontal_mirror  = hflip;
    config.vertical_mirror    = vflip;
    config.src_buf            = (void *)src_phys;
    config.dst_buf            = (void *)dst_phys;

    // printf("%d %d %d %d %p %p %d\n",
    //     config.frame_width, config.frame_height,
    //     config.src_stride, config.dst_stride,
    //     config.src_buf, config.dst_buf, config.dst_fmt);

    int ret = rotator_complete_conversion(rotater->rotate_fd, &config);
    if (ret < 0)
        fprintf(stderr, "hw rotater: failed to roate\n");

    return ret;
}

struct video_rotater *hw_video_rotater_open(struct video_rotater_param *param)
{
    int rotate_fd = rotator_open();
    if (rotate_fd < 0) {
        fprintf(stderr, "failed to open rotator\n");
        return NULL;
    }

    struct hw_video_rotater *hw_rotater = malloc(sizeof(*hw_rotater));
    hw_rotater->rotate_fd = rotate_fd;

    return &hw_rotater->rotater;
}

void hw_video_rotater_close(struct video_rotater *rotater)
{
    struct hw_video_rotater *hw_rotater = (void *)rotater;

    rotator_close(hw_rotater->rotate_fd);

    free(hw_rotater);
}

static void rotate_virt_mem(struct hw_video_rotater *rotater, struct video_frame *src_frame, struct video_frame *dst_frame, enum fb_fmt fb_fmt)
{
    int video_width = src_frame->width;
    int video_height = src_frame->height;

    enum video_frame_format video_format = src_frame->format;

    struct video_frame *copy_frame = media_alloter_alloc_video_frame(NULL, video_width, video_height, video_format, 0);
    if (!copy_frame) {
        fprintf(stderr, "hw rotater: faield to alloc rotate mem\n");
        return;
    }

    video_frame_copy(src_frame, copy_frame);

    unsigned long src_phys = copy_frame->phys_data[0];
    unsigned long dst_phys = dst_frame->phys_data[0];

    rotate_mem(rotater, video_width, video_height, fb_fmt,
        (void *)src_phys, 0, (void *)dst_phys);

    video_frame_put(copy_frame);
}

static void rotate_phys_mem(struct hw_video_rotater *rotater, struct video_frame *src_frame, struct video_frame *dst_frame, enum fb_fmt fb_fmt)
{
    int video_width = src_frame->width;
    int video_height = src_frame->height;

    unsigned long src_phys = src_frame->phys_data[0];
    unsigned long dst_phys = dst_frame->phys_data[0];

    rotate_mem(rotater, video_width, video_height, fb_fmt,
        (void *)src_phys, 0, (void *)dst_phys);
}

static int is_phys_continuity(struct video_frame *src_frame, enum fb_fmt fb_fmt)
{
    int is_phys = 1;

    int video_width = src_frame->width;
    int video_height = src_frame->height;

    int linesize = bytes_per_line(video_width, fb_fmt);

    switch (fb_fmt) {
    case fb_fmt_ARGB8888:
        break;
    case fb_fmt_NV12: {
        if ((src_frame->phys_data[1] - src_frame->phys_data[0]) != linesize*video_height)
            is_phys = 0;
        break;
    }
    default:break;
    }

    return is_phys;
}

static enum fb_fmt get_fb_fmt(enum video_frame_format video_format)
{
    switch (video_format) {
    case VIDEO_bgra:
        return fb_fmt_ARGB8888;
    case VIDEO_yuv420p:
    case VIDEO_nv12:
        return fb_fmt_NV12;
    default:
        fprintf(stderr, "hw_video_rotater: not support this format, default selection as NV12\n" );
        return fb_fmt_NV12;
    }
}

int hw_video_rotater_convert_frame(struct video_rotater *rotater, struct video_frame *src_frame, struct video_frame **dst_frame)
{
    assert(src_frame);

    struct hw_video_rotater *hw_rotater = (void *)rotater;

    enum video_frame_format video_format = src_frame->format;

    enum fb_fmt fb_fmt = get_fb_fmt(video_format);

    int is_phys = is_phys_continuity(src_frame, fb_fmt);

    int video_width = src_frame->width;
    int video_height = src_frame->height;

    enum video_rotate_angle angle = hw_rotater->rotater.param.rotate_angle;
    if (angle == rotate_90 || angle == rotate_270) {
        video_width = src_frame->height;
        video_height = src_frame->width;
    }

    struct video_frame *frame = media_alloter_alloc_video_frame(NULL, video_width, video_height, video_format, 0);
    if (!frame) {
        fprintf(stderr, "hw rotater: faield to alloc rotate mem\n");
        return -1;
    }

    if (is_phys)
        rotate_phys_mem(hw_rotater, src_frame, frame, fb_fmt);
    else
        rotate_virt_mem(hw_rotater, src_frame, frame, fb_fmt);

    *dst_frame = frame;

    return 0;
}

struct video_rotater_cb hw_video_rotater_cb = {
    .open_rotater = hw_video_rotater_open,
    .close_rotater = hw_video_rotater_close,
    .convert_video_frame = hw_video_rotater_convert_frame,
};


void hw_video_rotater_init_param(struct video_rotater_param *param)
{
    param->cb = &hw_video_rotater_cb;
}