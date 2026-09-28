#include <libmedia/video_frame.h>
#include <stdlib.h>
#include <string.h>

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) < (y) ? (x) : (y))
#endif

#define DEFAULT_BYTES_ALIGN       8

#define ALIGN(x, n) (((x) + (n) - 1) - ((x) + (n) - 1) % (n))

#if MAX_VIDEO_FRAME_BUF_CNT != 8
#error "MAX_VIDEO_FRAME_BUF_CNT must be 8"
#endif

#pragma  pack (push,1)
struct fmt_den {
    unsigned int w_cnt:3;
    unsigned int h_cnt:3;
    unsigned int multi:2;
} ;

struct fmt_info {
    uint8_t pixel_bytes;
    struct fmt_den dens[MAX_VIDEO_FRAME_BUF_CNT];
};
#pragma  pack (pop)

static const char *fmt_names[VIDEO_nums] = {
    [VIDEO_nv12] = "nv12",
    [VIDEO_yuyv] = "yuyv",
    [VIDEO_yuv420p] = "yuv420p",
    [VIDEO_yuva420p] = "yuva420p",
    [VIDEO_yuvj420p] = "yuvj420p",
    [VIDEO_yuv422p] = "yuvj422p",
    [VIDEO_yuv444p] = "yuvj444p",
    [VIDEO_yuv422p] = "yuv422p",
    [VIDEO_yuv444p] = "yuv444p",
    [VIDEO_yuv411p] = "yuv411p",
    [VIDEO_y8] = "y8",
    [VIDEO_y16] = "y16",
    [VIDEO_alpha] = "alpha",
    [VIDEO_bgra] = "bgra",
};

const char *video_fmt_name(enum video_frame_format fmt)
{
    return fmt_names[fmt] ? fmt_names[fmt] : "err-video-fmt";
}

static struct fmt_info fmts[VIDEO_nums] = {
    [VIDEO_nv12] = {1, { {1,1,1}, {2,2,2} } },
    [VIDEO_yuyv] = {1, { {1,1,2} } },
    [VIDEO_yuv420p] = {1, { {1,1,1}, {2,2,1}, {2,2,1} } },
    [VIDEO_yuva420p] = {1, { {1,1,1}, {2,2,1}, {2,2,1}, {1,1,1} } },
    [VIDEO_yuvj420p] = {1, { {1,1,1}, {2,2,1}, {2,2,1} } },
    [VIDEO_yuvj422p] = {1, { {1,1,1}, {2,1,1}, {2,1,1} } },
    [VIDEO_yuvj444p] = {1, { {1,1,1}, {1,1,1}, {1,1,1} } },
    [VIDEO_yuv422p] = {1, { {1,1,1}, {2,1,1}, {2,1,1} } },
    [VIDEO_yuv444p] = {1, { {1,1,1}, {1,1,1}, {1,1,1} } },
    [VIDEO_yuv411p] = {1, { {1,1,1}, {4,1,1}, {4,1,1} } },
    [VIDEO_y8] = {1, { {1,1,1} } },
    [VIDEO_y16] = {2, { {1,1,1} } },
    [VIDEO_alpha] = {1, { {1,1,1} } },
    [VIDEO_bgra] = {4, { {1,1,1} } },
};

int video_format_line_size(enum video_frame_format fmt, int planar_index, int width, int align)
{
    struct fmt_den d = fmts[fmt].dens[planar_index];
    if (!d.w_cnt)
        return 0;

    if (!align)
        align = DEFAULT_BYTES_ALIGN;

    width = width/d.w_cnt*d.multi*fmts[fmt].pixel_bytes;

    return ALIGN(width, align);
}

int video_format_line_cnt(enum video_frame_format fmt, int planar_index, int height)
{
    struct fmt_den d = fmts[fmt].dens[planar_index];
    return d.h_cnt ? height/d.h_cnt : 0;
}

int video_format_planar_size(
    enum video_frame_format fmt, int planar_index, int width, int height, int align)
{
    width = video_format_line_size(fmt, planar_index, width, align);
    height = video_format_line_cnt(fmt, planar_index, height);

    return width * height;
}

int video_format_size(
    enum video_frame_format fmt, int width, int height, int align)
{
    int i, size = 0;

    for (i = 0; i < MAX_VIDEO_FRAME_BUF_CNT; i++) {
        struct fmt_den d = fmts[fmt].dens[i];
        if (!d.w_cnt)
            break;
        size += video_format_planar_size(fmt, i, width, height, align);
    }

    return size;
}

int line_size_to_align_bytes(int line_size, int width)
{
    int n = 2;
    do {
        if (line_size == ALIGN(width, n))
            return n;
    } while (n <= 4096);

    return DEFAULT_BYTES_ALIGN;
}

struct video_frame* video_frame_alloc(void)
{
    struct video_frame *frame = malloc(sizeof(*frame));
    assert(frame);

    memset(frame, 0 , sizeof(*frame));

    return frame;
}

void video_frame_free(struct video_frame *frame)
{
    if(!frame)
        return;

    assert(!frame->user_cnt);
    free(frame);
}

void video_frame_put(struct video_frame *frame)
{
    if(!frame)
        return;

    frame->user_cnt--;

    assert(frame->user_cnt >= 0);

    if (!frame->user_cnt && frame->put_frame)
        frame->put_frame(frame->handle, frame);
}

void video_frame_get(struct video_frame *frame)
{
    frame->user_cnt++;
}

int video_frame_init(struct video_frame *frame, int width, int height,
                         enum video_frame_format format, int align,
                         void *data, unsigned long phy_data, void *handle,
                         void (*put_frame)(void *, struct video_frame *))
{
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "media:failed to init video frame, width or height is err\n");
        return -1;
    }

    if (!data) {
        fprintf(stderr, "media:failed to init video frame, data mem is NULL\n");
        return -1;
    }

    frame->width = width;
    frame->height = height;
    frame->format = format;
    frame->total_size = 0;

    int i;
    for (i = 0; i < MAX_VIDEO_FRAME_BUF_CNT; i++) {
        int size = video_format_planar_size(format, i, width, height, align);
        frame->data[i] = size ? data : NULL;
        frame->size[i] = size;
        frame->linesize[i] = video_format_line_size(format, i, width, align);
        data += size;
        frame->total_size += size;
        frame->phys_data[i] = phy_data;
        if (phy_data)
            phy_data += size;
    }

    frame->is_phys = !!phy_data;
    frame->user_cnt = 0;

    frame->handle = handle;
    frame->put_frame = put_frame;

    return 0;
}

static void m_put_frame(void *handle, struct video_frame *frame)
{
    free(frame->data[0]);
    free(frame);
}

struct video_frame *video_frame_alloc_init(int width, int height,
                         enum video_frame_format format, int align)
{
    int size = video_format_size(format, width, height, align);

    void *data = malloc(size);
    if (!data) {
        fprintf(stderr, "media: failed to allocate: %d\n", size);
        return NULL;
    }

    struct video_frame *frame = video_frame_alloc();
    video_frame_init(frame, width, height, format, align, data, 0, NULL, m_put_frame);

    video_frame_get(frame);

    return frame;
}

void video_format_get_pixel_align(
    enum video_frame_format fmt, int *w_align, int *h_align)
{
    int i;
    int max_w_cnt = 0, max_h_cnt = 0;

    for (i = 0; i < MAX_VIDEO_FRAME_BUF_CNT; i++) {
        struct fmt_den d = fmts[fmt].dens[i];
        if (!d.w_cnt)
            break;
        if (d.w_cnt > max_w_cnt)
            max_w_cnt = d.w_cnt;
        if (d.h_cnt > max_h_cnt)
            max_h_cnt = d.h_cnt;
    }

    assert (max_w_cnt);
    assert (max_h_cnt);

    if (w_align)
        *w_align = max_w_cnt;
    if (h_align)
        *h_align = max_h_cnt;
}

static void crop_frame(struct video_frame *src, struct video_frame *dst, int x, int y, int w, int h)
{
    int i;

    memset(dst, 0, sizeof(*dst));

    dst->width = w;
    dst->height = h;
    dst->format = src->format;
    dst->total_size = 0;

    for (i = 0; i < MAX_VIDEO_FRAME_BUF_CNT; i++) {
        struct fmt_den d = fmts[src->format].dens[i];
        if (!d.w_cnt)
            break;

        int offset = video_format_line_cnt(src->format, i, y) * src->linesize[i] +
                     video_format_line_size(src->format, i, x, 1);
        int size = video_format_line_cnt(src->format, i, h) * src->linesize[i];

        dst->data[i] = src->data[i] + offset;
        dst->size[i] = size;
        dst->linesize[i] = src->linesize[i];

        dst->total_size += size;
        if (src->phys_data[i])
            dst->phys_data[i] = src->phys_data[i] + offset;
    }
}

void video_frame_crop(struct video_frame *src, struct video_frame *dst, int x, int y, int w, int h)
{
    int w_align, h_align;

    video_format_get_pixel_align(src->format, &w_align, &h_align);

    x = ALIGN(x, w_align);
    y = ALIGN(y, h_align);

    w = ALIGN(w, w_align);
    h = ALIGN(h, h_align);

    if (x+w > src->width)
        w = src->width - x;
    if (y+h > src->height)
        h = src->height - y;

    crop_frame(src, dst, x, y, w, h);
}

static int check_crop(
    enum video_frame_format src_fmt, int src_w, int src_h,
    enum video_frame_format dst_fmt, int dst_w, int dst_h,
    int x, int y, int w, int h,
    int *sx_p, int *sy_p, int *dx_p, int *dy_p, int *w_p, int *h_p)
{
    if (x+w <= 0 || y+h <= 0)
        return -1;

    int s_x = x < 0 ? -x : 0;
    int s_y = y < 0 ? -y : 0;
    int s_w = src_w - s_x;
    int s_h = src_h - s_y;

    int d_x = x < 0 ? 0 : x;
    int d_y = y < 0 ? 0 : y;
    int d_w = dst_w - d_x;
    int d_h = dst_h - d_y;

    w = min(min(s_w, d_w), w);
    h = min(min(s_h, d_h), h);

    if (w <= 0 || h <= 0)
        return -1;

    int w_a[2], h_a[2];

    video_format_get_pixel_align(src_fmt, &w_a[0], &h_a[0]);
    video_format_get_pixel_align(dst_fmt, &w_a[1], &h_a[1]);

    int w_align = max(w_a[0], w_a[1]);
    int h_align = max(h_a[0], h_a[1]);

    s_x = ALIGN(s_x, w_align);
    s_y = ALIGN(s_y, h_align);

    s_w = ALIGN(w, w_align);
    s_h = ALIGN(h, h_align);

    if (s_x+s_w > src_w)
        s_w = src_w - x;
    if (s_y+s_h > src_h)
        s_h = src_h - y;

    d_x = ALIGN(d_x, w_align);
    d_y = ALIGN(d_y, h_align);

    d_w = ALIGN(w, w_align);
    d_h = ALIGN(h, h_align);

    if (d_x+d_w > dst_w)
        d_w = dst_w - x;
    if (d_y+d_h > dst_h)
        d_h = dst_h - y;

    w = min(s_w, d_w);
    h = min(s_h, d_h);

    if (w <= 0 || h <= 0)
        return -1;

    *sx_p = s_x;
    *sy_p = s_y;
    *dx_p = d_x;
    *dy_p = d_y;
    *w_p = w;
    *h_p = h;

    return 0;
}

int video_frame_crop_src_dst(
    struct video_frame *src, struct video_frame *dst, int x, int y, int w, int h,
    struct video_frame *src_crop, struct video_frame *dst_crop)
{
    int s_x, s_y, d_x, d_y;
    int ret = check_crop(
        src->format, src->width, src->height,
        dst->format, dst->width, dst->height,
        x, y, w, h, &s_x, &s_y, &d_x, &d_y, &w, &h);
    if (ret)
        return ret;

    crop_frame(src, src_crop, s_x, s_y, w, h);
    crop_frame(dst, dst_crop, d_x, d_y, w, h);

    return 0;
}

int video_frame_check_crop_src_dst(
    enum video_frame_format src_fmt, int src_w, int src_h,
    enum video_frame_format dst_fmt, int dst_w, int dst_h,
    int x, int y, int w, int h)
{
    int s_x, s_y, d_x, d_y;
    return check_crop(
        src_fmt, src_w, src_h, dst_fmt, dst_w, dst_h,
        x, y, w, h, &s_x, &s_y, &d_x, &d_y, &w, &h);
}