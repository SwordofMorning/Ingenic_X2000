#include "base/g2d.h"

#ifdef APP_awtk_WITH_INGENIC_LIB2D_G2D
#include <lib2d/ingenic2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

unsigned int lcd_linux_fb_get_size(int *width, int *height, int *linesize);
unsigned long lcd_linux_fb_get_phyaddr(void *fb_data, int *index);

struct ingenic_2d *ingenic_2d;
struct ingenic_2d_frame *dst_2d_frame[2];

static struct ingenic_2d_frame *get_dst_2d_frame(int index, unsigned long fb_phy_addr, void *fb_vir_addr)
{
    if (dst_2d_frame[index])
        return dst_2d_frame[index];

    int width, height,linesize, fb_size;
    fb_size = lcd_linux_fb_get_size(&width, &height, &linesize);

    dst_2d_frame[index] = ingenic_2d_alloc_frame_by_user(ingenic_2d, width, height, INGENIC_2D_ARGB8888, fb_phy_addr, fb_vir_addr, fb_size);

    return dst_2d_frame[index];
}


ret_t g2d_fill_rect(bitmap_t* fb, const rect_t* dst, color_t c)
{
    /* 先不使用硬件填充 */
    return RET_FAIL;

    int ret = 0;
    uint8_t *dst_data = NULL;
    unsigned long fb_phyaddr = 0;

    return_value_if_fail(fb != NULL && fb->buffer != NULL && dst != NULL, RET_BAD_PARAMS);
    return_value_if_fail(fb->format == BITMAP_FMT_BGRA8888, RET_BAD_PARAMS);

    dst_data = bitmap_lock_buffer_for_write(fb);
    return_value_if_fail(dst_data != NULL, RET_BAD_PARAMS);

    int index;
    fb_phyaddr = lcd_linux_fb_get_phyaddr(dst_data, &index);
    if (!fb_phyaddr)
        goto fail_unlock;

    if (!ingenic_2d) {
        ingenic_2d = ingenic_2d_open();
        if (!ingenic_2d)
            goto fail_unlock;
    }

    struct ingenic_2d_frame *dst_frame = get_dst_2d_frame(index, fb_phyaddr, dst_data);
    if (!dst_frame)
        goto fail_2d_close;

    int bgra_color = (c.rgba.a << 24) | (c.rgba.r << 16) | (c.rgba.g << 8) | (c.rgba.b << 0);

    struct ingenic_2d_rect dst_rect = ingenic_2d_rect_init(dst_frame, dst->x, dst->y, dst->w, dst->h);

    ret = ingenic_2d_fill_rect(ingenic_2d, &dst_rect, bgra_color);
    if (ret < 0)
        goto fail_2d_free;

    bitmap_unlock_buffer(fb);

    return RET_OK;

fail_2d_free:
fail_2d_close:
fail_unlock:
    bitmap_unlock_buffer(fb);

    return RET_FAIL;
}

ret_t g2d_copy_image(bitmap_t* fb, bitmap_t* img, const rect_t* src, xy_t dx, xy_t dy)
{
    return RET_FAIL;
}

ret_t g2d_rotate_image(bitmap_t* fb, bitmap_t* img, const rect_t* src, lcd_orientation_t o)
{
    return RET_FAIL;
}

ret_t g2d_blend_image(bitmap_t* fb, bitmap_t* img, const rect_t* dst, const rect_t* src,
                      uint8_t global_alpha)
{
    int ret = 0;
    int i = 0;
    int is_scale = 0;
    uint8_t *src_data = NULL;
    uint8_t *dst_data = NULL;

    int src_width = bitmap_get_physical_width(img);
    int src_height = bitmap_get_physical_height(img);
    int src_linesize = bitmap_get_physical_line_length(img);

    enum ingenic_2d_format src_format = -1;
    enum ingenic_2d_format dst_format = -1;

    unsigned long dst_phyaddr = 0;

    // return_value_if_fail(global_alpha == 0xff, RET_BAD_PARAMS);
    return_value_if_fail(img != NULL && img->buffer != NULL && src != NULL, RET_BAD_PARAMS);
    return_value_if_fail(fb != NULL && fb->buffer != NULL && dst != NULL, RET_BAD_PARAMS);
    return_value_if_fail(img->format == BITMAP_FMT_BGRA8888, RET_BAD_PARAMS);
    return_value_if_fail(fb->format == BITMAP_FMT_BGRA8888, RET_BAD_PARAMS);

    if (img->format == BITMAP_FMT_BGRA8888) {
        src_format = INGENIC_2D_ARGB8888;
    }

    if (fb->format == BITMAP_FMT_BGRA8888) {
        dst_format = INGENIC_2D_ARGB8888;
    }

    if (src->w != dst->w || src->h != dst->h)
        is_scale = 1;

    src_data = bitmap_lock_buffer_for_write(img);
    dst_data = bitmap_lock_buffer_for_write(fb);
    return_value_if_fail(src_data != NULL && dst_data != NULL, RET_BAD_PARAMS);

    int index = 0;
    dst_phyaddr = lcd_linux_fb_get_phyaddr(dst_data, &index);
    if (!dst_phyaddr)
        goto fail_unlock;

    if (!ingenic_2d) {
        ingenic_2d = ingenic_2d_open();
        if (!ingenic_2d)
            goto fail_unlock;
    }

    struct ingenic_2d_frame *src_frame = NULL;
    struct ingenic_2d_frame *src_scale_frame = NULL;
    struct ingenic_2d_frame *dst_frame = NULL;
    struct ingenic_2d_rect src_rect;
    struct ingenic_2d_rect src_scale_rect;
    struct ingenic_2d_rect dst_rect;

    struct ingenic_2d_rect blend_rect;

    src_frame = ingenic_2d_alloc_frame(ingenic_2d, src_width, src_height, src_format);
    if (!src_frame)
        goto fail_2d_close;
    for (i = 0; i < src_height; i++) {
        int m_off = src_linesize * i;
        int n_off = src_frame->stride * i;
        memcpy(src_frame->addr[0] + n_off, src_data + m_off, src_linesize);
    }

    src_rect = ingenic_2d_rect_init(src_frame, src->x, src->y, src->w, src->h);

    blend_rect = src_rect;

    if (is_scale) {
        /* 缩放功能没在awtk验证，暂不开放 */
        // fprintf(stderr, "ingenic_g2d: g2d scale before blend\n");
        goto fail_2d_free;

        src_scale_frame = ingenic_2d_alloc_frame(ingenic_2d, dst->w, dst->h, dst_format);
        if (!src_scale_frame)
            goto fail_2d_free;

        src_scale_rect = ingenic_2d_rect_init(src_scale_frame, 0, 0, dst->w, dst->h);

        ret = ingenic_2d_scale(ingenic_2d, &src_rect, &src_scale_rect);
        if (ret < 0)
            goto fail_2d_free;

        blend_rect = src_scale_rect;
    }

    dst_frame = get_dst_2d_frame(index, dst_phyaddr, dst_data);
    if (!dst_frame)
        goto fail_2d_free;

    dst_rect = ingenic_2d_rect_init(dst_frame, dst->x, dst->y, dst->w, dst->h);

    ret = ingenic_2d_blend(ingenic_2d, &blend_rect, &dst_rect, global_alpha);
    if (ret < 0)
        goto fail_2d_free;

    if (src_frame)
        ingenic_2d_free_frame(ingenic_2d, src_frame);
    if (src_scale_frame)
        ingenic_2d_free_frame(ingenic_2d, src_scale_frame);

    bitmap_unlock_buffer(img);
    bitmap_unlock_buffer(fb);

    return RET_OK;

fail_2d_free:
    if (src_frame)
        ingenic_2d_free_frame(ingenic_2d, src_frame);
    if (src_scale_frame)
        ingenic_2d_free_frame(ingenic_2d, src_scale_frame);
fail_2d_close:
fail_unlock:
    bitmap_unlock_buffer(img);
    bitmap_unlock_buffer(fb);

    return RET_FAIL;
}

#endif