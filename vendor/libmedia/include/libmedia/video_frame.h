#ifndef __VIDEO_FRAME_H__
#define __VIDEO_FRAME_H__

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define MAX_VIDEO_FRAME_BUF_CNT  8

enum video_frame_format {
    VIDEO_nv12,
    VIDEO_yuyv,
    VIDEO_yuv420p,
    VIDEO_yuva420p,
    VIDEO_yuvj420p,
    VIDEO_yuvj422p,
    VIDEO_yuvj444p,
    VIDEO_yuv422p,
    VIDEO_yuv444p,
    VIDEO_yuv411p,
    VIDEO_y8,
    VIDEO_y16,
    VIDEO_alpha,
    VIDEO_bgra,

    VIDEO_nums,
};

struct video_frame {
    int width;
    int height;
    enum video_frame_format format;
    int is_phys;

    uint8_t *data[MAX_VIDEO_FRAME_BUF_CNT];
    uint32_t size[MAX_VIDEO_FRAME_BUF_CNT];
    uint32_t linesize[MAX_VIDEO_FRAME_BUF_CNT];

    uint32_t total_size;

    unsigned long phys_data[MAX_VIDEO_FRAME_BUF_CNT];
    void *pdata;
    int user_cnt;
    int64_t timestamp_us;

    void *handle;
    void (*put_frame)(void *handle, struct video_frame *frame);
};



const char *video_fmt_name(enum video_frame_format fmt);

void video_frame_put(struct video_frame *frame);
void video_frame_get(struct video_frame *frame);
struct video_frame* video_frame_alloc(void);
void video_frame_free(struct video_frame *frame);

int video_frame_init(struct video_frame *frame, int width, int height,
                         enum video_frame_format format, int align,
                         void *data, unsigned long phy_data, void *handle,
                         void (*put_frame)(void *, struct video_frame *));


struct video_frame *video_frame_alloc_init(int width, int height,
                         enum video_frame_format format, int align);

int video_frame_copy(
    struct video_frame *src_frame, struct video_frame *dst_frame);

int video_format_line_size(
    enum video_frame_format fmt, int planar_index, int width, int align);

int video_format_line_cnt(
    enum video_frame_format fmt, int planar_index, int height);

int video_format_planar_size(
    enum video_frame_format fmt, int planar_index, int width, int height, int align);

int video_format_size(
    enum video_frame_format fmt, int width, int height, int align);

int line_size_to_align_bytes(int line_size, int width);

void video_format_get_pixel_align(
    enum video_frame_format fmt, int *w_align, int *h_align);

void video_frame_crop(struct video_frame *src, struct video_frame *dst, int x, int y, int w, int h);

int video_frame_crop_src_dst(
    struct video_frame *src, struct video_frame *dst, int x, int y, int w, int h,
    struct video_frame *src_crop, struct video_frame *dst_crop);

int video_frame_check_crop_src_dst(
    enum video_frame_format src_fmt, int src_w, int src_h,
    enum video_frame_format dst_fmt, int dst_w, int dst_h,
    int x, int y, int w, int h);

void video_frame_blend_alpha_color(
    struct video_frame *src, unsigned int color, struct video_frame *dst, int x, int y);

void video_frame_blend(
    struct video_frame *src, struct video_frame *dst, int x, int y, uint8_t alpha);

int video_frame_data_copy(enum video_frame_format s_format, uint8_t **s_data, uint32_t *s_linesize,
                          enum video_frame_format d_format, uint8_t **d_data, uint32_t *d_linesize, int width, int height);


#endif