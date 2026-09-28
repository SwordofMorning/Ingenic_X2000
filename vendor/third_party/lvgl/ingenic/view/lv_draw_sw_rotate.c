// #include "lvgl/src/draw/sw/lv_draw_sw.h"
#include "lvgl/lvgl.h"

#define ROTATE_BITS 32
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

#define ROTATE_BITS 24
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

#define USE_PIX_BILINEAR

#define ROTATE_BITS 32
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

#define ROTATE_BITS 24
#include "libutils2/rotate.h"
#undef rotate_base_type
#undef ROTATE_BITS

void lv_sw_rotate_cal_dst_area(int s_w, int s_h, int s_x, int s_y,
    int d_w, int d_h, int d_x, int d_y,
    int angle, int *new_d_w, int *new_d_h, int *new_d_x, int *new_d_y)
{
    rotate_cal_dst_area(s_w, s_h, s_x, s_y,
                        d_w, d_h, d_x, d_y,
                        angle/10.0, new_d_w, new_d_h, new_d_x, new_d_y);
}

typedef void (*rotate_func_t)(
    void *s, int s_w, int s_h, int s_x, int s_y, int s_linesz,
    void *d, int d_w, int d_h, int d_x, int d_y, int d_linesz,
    float angle, int border_bilinear,
    int rotate_version, rotate_get_fast_version_t get_fast_version);

void lv_draw_sw_rotate(lv_draw_ctx_t *draw_ctx,
                      const lv_area_t *src_blend_area,
                      const uint8_t *src_buf,
                      const lv_draw_img_dsc_t *draw_dsc)
{
    lv_area_t *buf_area = draw_ctx->buf_area;
    lv_coord_t dest_stride = lv_area_get_width(buf_area);

    lv_area_t blend_area;
    if(!_lv_area_intersect(&blend_area, src_blend_area, draw_ctx->clip_area)) return;

    // lv_disp_t * disp = _lv_refr_get_disp_refreshing();
    lv_color_t * dest_buf = draw_ctx->buf;

    dest_buf += dest_stride * (blend_area.y1 - buf_area->y1) + (blend_area.x1 - buf_area->x1);

    int xoff = blend_area.x1 - src_blend_area->x1;
    int yoff = blend_area.y1 - src_blend_area->y1;

    lv_area_move(&blend_area, -buf_area->x1, -buf_area->y1);

    int w = lv_area_get_width(&blend_area);
    int h = lv_area_get_height(&blend_area);

    rotate_dsc_t *r = draw_dsc->rotate_dsc;
    int d_x = r->d_x - xoff;
    int d_y = r->d_y - yoff;

    rotate_func_t rotate_func;
    if (r->ignore_alpha)
        rotate_func = r->bilinear ? rotate_func_uint24_bilinear : rotate_func_uint24;
    else
        rotate_func = r->bilinear ? rotate_func_uint32_bilinear : rotate_func_uint32;

    rotate_func(
        (void *)src_buf, r->s_w, r->s_h, r->s_x, r->s_y, r->s_w*sizeof(lv_color_t),
        (void *)dest_buf, w, h, d_x, d_y, dest_stride*sizeof(lv_color_t),
        r->angle/10.0, 1, 2, r->get_fast_version);
}
