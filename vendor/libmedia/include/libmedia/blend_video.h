#ifndef _LIBMEDIA_BLEND_VIDEO_H_
#define _LIBMEDIA_BLEND_VIDEO_H_

#include <stdint.h>

void blend_y8(
    uint8_t **s_data, uint32_t *s_linesize,
    uint8_t **d_data, uint32_t *d_linesize,
    int width, int height, uint8_t alpha);

void blend_nv12(
    uint8_t **s_data, uint32_t *s_linesize,
    uint8_t **d_data, uint32_t *d_linesize,
    int width, int height, uint8_t alpha);

void blend_yuv420p(
    uint8_t **s_data, uint32_t *s_linesize,
    uint8_t **d_data, uint32_t *d_linesize,
    int width, int height, uint8_t alpha);

void blend_yuv411p(
    uint8_t **s_data, uint32_t *s_linesize,
    uint8_t **d_data, uint32_t *d_linesize,
    int width, int height, uint8_t alpha);

void blend_yuv422p(
    uint8_t **s_data, uint32_t *s_linesize,
    uint8_t **d_data, uint32_t *d_linesize,
    int width, int height, uint8_t alpha);

void blend_yuv444p(
    uint8_t **s_data, uint32_t *s_linesize,
    uint8_t **d_data, uint32_t *d_linesize,
    int width, int height, uint8_t alpha);

void blend_yuva420p_to_nv12(
    uint8_t **s_data, uint32_t *s_linesize,
    uint8_t **d_data, uint32_t *d_linesize,
    int width, int height, uint8_t alpha);

void blend_bgra_to_nv12(
    uint8_t **s_data, uint32_t *s_linesize,
    uint8_t **d_data, uint32_t *d_linesize,
    int width, int height, uint8_t alpha);

void blend_alpha_color_y(
    uint8_t *src, int src_linesize,
    uint8_t *dst, int dst_linesize,
    int width, int height, uint8_t y_color, uint8_t alpha);

void blend_alpha_color_uv(
    uint8_t *src, int src_linesize, uint8_t *dst, int dst_linesize,
    int width, int height, uint8_t u_color, uint8_t v_color, uint8_t alpha);

void blend_alpha_color_nv12(
    uint8_t *src, int src_linesize,
    uint8_t *y, int y_linesize,
    uint8_t *uv, int uv_linesize,
    int width, int height, unsigned int color);

void blend_bgra_to_bgra(
    uint8_t **s_data, uint32_t *s_linesize,
    uint8_t **d_data, uint32_t *d_linesize,
    int width, int height, uint8_t alpha);

void blend_alpha_color_bgra(uint8_t *src, int src_linesize,
                            uint8_t *dst, int dst_linesize,
                 int width, int height, unsigned int color);

#endif /* _LIBMEDIA_BLEND_VIDEO_H_ */
