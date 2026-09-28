#include <libmedia/video_frame.h>
#include <libmedia/rgb_to_yuv.h>
#include <string.h>
#include <stdint.h>
#include <libmedia/blend_video.h>

#define ALIGN_DOWN(X, n) ((X) - ((X)%(n)))
#define UBLEND(f, b, a) ((((a)*f) + ((255 - a) * b)) >> 8)

static void copy_raw_bytes(void *dst, int dst_linesize, void *src, int src_linesize, int height, int width)
{
    if (dst_linesize == src_linesize && width == dst_linesize) {
        memcpy(dst, src, width*height);
        return;
    }

    int i;
    for (i = 0; i < height; i++) {
        memcpy(dst, src, width);
        src += src_linesize;
        dst += dst_linesize;
    }
}

static void c_blend_plane(uint8_t *src, int src_linesize, uint8_t *dst, int dst_linesize, int width, int height, uint8_t alpha)
{
    /* src 几乎全透明,所以对dst 没有影响
     */
    if (alpha <= 0x08)
        return;

    /* src 几乎不透明,所以直接覆盖dst
     */
    if (alpha >= 0xf8)
        return copy_raw_bytes(dst, dst_linesize, src, src_linesize, height, width);

    int i, j;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width-1; i+=2) {
            uint8_t s0 = src[i+0];
            uint8_t s1 = src[i+1];

            uint8_t d0 = dst[i+0];
            uint8_t d1 = dst[i+1];

            d0 = UBLEND(s0, d0, alpha);
            d1 = UBLEND(s1, d1, alpha);

            dst[i+0] = d0;
            dst[i+1] = d1;
        }
        if (width & 1) {
            uint8_t s0 = src[i+0];
            uint8_t d0 = dst[i+0];

            d0 = UBLEND(s0, d0, alpha);

            dst[i+0] = d0;
        }

        src += src_linesize;
        dst += dst_linesize;
    }

}

#include "msa_blend_yuv.c"

static void blend_plane(uint8_t *src, int src_linesize, uint8_t *dst, int dst_linesize, int width, int height, uint8_t alpha)
{
#if defined(__mips_msa)
    return msa_blend_plane(src, src_linesize, dst, dst_linesize, width, height, alpha);
#endif
    c_blend_plane(src, src_linesize, dst, dst_linesize, width, height, alpha);
}


void blend_y8(uint8_t **s_data, uint32_t *s_linesize,
              uint8_t **d_data, uint32_t *d_linesize,
              int width, int height, uint8_t alpha)
{
    void *sy = s_data[0];
    int sy_linesize = s_linesize[0];

    void *dy = d_data[0];
    int dy_linesize = d_linesize[0];

    blend_plane(sy, sy_linesize, dy, dy_linesize, width, height, alpha);
}

void blend_nv12(uint8_t **s_data, uint32_t *s_linesize,
                uint8_t **d_data, uint32_t *d_linesize,
                int width, int height, uint8_t alpha)
{
    void *sy = s_data[0];
    void *suv = s_data[1];
    int sy_linesize = s_linesize[0];
    int suv_linesize = s_linesize[1];

    void *dy = d_data[0];
    void *duv = d_data[1];
    int dy_linesize = d_linesize[0];
    int duv_linesize = d_linesize[1];

    blend_plane(sy, sy_linesize, dy, dy_linesize, width, height, alpha);
    blend_plane(suv, suv_linesize, duv, duv_linesize, width, height/2, alpha);
}

void blend_yuv420p(uint8_t **s_data, uint32_t *s_linesize,
                   uint8_t **d_data, uint32_t *d_linesize,
                   int width, int height, uint8_t alpha)
{
    void *sy = s_data[0];
    void *su = s_data[1];
    void *sv = s_data[2];
    int sy_linesize = s_linesize[0];
    int su_linesize = s_linesize[1];
    int sv_linesize = s_linesize[2];

    void *dy = d_data[0];
    void *du = d_data[1];
    void *dv = d_data[2];
    int dy_linesize = d_linesize[0];
    int du_linesize = d_linesize[1];
    int dv_linesize = d_linesize[2];

    blend_plane(sy, sy_linesize, dy, dy_linesize, width, height, alpha);
    blend_plane(su, su_linesize, du, du_linesize, width/2, height/2, alpha);
    blend_plane(sv, sv_linesize, dv, dv_linesize, width/2, height/2, alpha);
}

void blend_yuv411p(uint8_t **s_data, uint32_t *s_linesize,
                   uint8_t **d_data, uint32_t *d_linesize,
                   int width, int height, uint8_t alpha)
{
    void *sy = s_data[0];
    void *su = s_data[1];
    void *sv = s_data[2];
    int sy_linesize = s_linesize[0];
    int su_linesize = s_linesize[1];
    int sv_linesize = s_linesize[2];

    void *dy = d_data[0];
    void *du = d_data[1];
    void *dv = d_data[2];
    int dy_linesize = d_linesize[0];
    int du_linesize = d_linesize[1];
    int dv_linesize = d_linesize[2];

    blend_plane(sy, sy_linesize, dy, dy_linesize, width, height, alpha);
    blend_plane(su, su_linesize, du, du_linesize, width/4, height, alpha);
    blend_plane(sv, sv_linesize, dv, dv_linesize, width/4, height, alpha);
}

void blend_yuv422p(uint8_t **s_data, uint32_t *s_linesize,
                   uint8_t **d_data, uint32_t *d_linesize,
                   int width, int height, uint8_t alpha)
{
    void *sy = s_data[0];
    void *su = s_data[1];
    void *sv = s_data[2];
    int sy_linesize = s_linesize[0];
    int su_linesize = s_linesize[1];
    int sv_linesize = s_linesize[2];

    void *dy = d_data[0];
    void *du = d_data[1];
    void *dv = d_data[2];
    int dy_linesize = d_linesize[0];
    int du_linesize = d_linesize[1];
    int dv_linesize = d_linesize[2];


    blend_plane(sy, sy_linesize, dy, dy_linesize, width, height, alpha);
    blend_plane(su, su_linesize, du, du_linesize, width/2, height, alpha);
    blend_plane(sv, sv_linesize, dv, dv_linesize, width/2, height, alpha);
}

void blend_yuv444p(uint8_t **s_data, uint32_t *s_linesize,
                   uint8_t **d_data, uint32_t *d_linesize,
                   int width, int height, uint8_t alpha)
{
    void *sy = s_data[0];
    void *su = s_data[1];
    void *sv = s_data[2];
    int sy_linesize = s_linesize[0];
    int su_linesize = s_linesize[1];
    int sv_linesize = s_linesize[2];

    void *dy = d_data[0];
    void *du = d_data[1];
    void *dv = d_data[2];
    int dy_linesize = d_linesize[0];
    int du_linesize = d_linesize[1];
    int dv_linesize = d_linesize[2];

    blend_plane(sy, sy_linesize, dy, dy_linesize, width, height, alpha);
    blend_plane(su, su_linesize, du, du_linesize, width, height, alpha);
    blend_plane(sv, sv_linesize, dv, dv_linesize, width, height, alpha);
}

static void c_blend_bgra_to_nv12(void *src_bgra, int src_linesize,
                          void *dst_y, int dst_y_linesize,
                          void *dst_uv, int dst_uv_linesize,
                          int width, int height, uint8_t alpha)
{
    int i, j;

    for (i = 0; i < height; i+=2) {
        unsigned char *p0 = src_bgra + i*src_linesize;
        unsigned char *p1 = p0 + src_linesize;

        unsigned char *Y0 = dst_y + i * dst_y_linesize;
        unsigned char *Y1 = Y0 + dst_y_linesize;
        unsigned char *uv = dst_uv + i*dst_uv_linesize/2;

        for (j = 0; j < width; j += 2) {
            unsigned char a00 = p0[3];
            unsigned char a01 = p0[3 + 4];
            unsigned char a10 = p1[3];
            unsigned char a11 = p1[3 + 4];

            if (alpha < 0xf8) {
                a00 = (a00 * alpha) >> 8;
                a01 = (a01 * alpha) >> 8;
                a10 = (a10 * alpha) >> 8;
                a11 = (a11 * alpha) >> 8;
            }

            unsigned char r00 = p0[2];
            unsigned char g00 = p0[1];
            unsigned char b00 = p0[0];

            unsigned char r01 = p0[2+4];
            unsigned char g01 = p0[1+4];
            unsigned char b01 = p0[0+4];

            unsigned char r10 = p1[2];
            unsigned char g10 = p1[1];
            unsigned char b10 = p1[0];

            unsigned char r11 = p1[2+4];
            unsigned char g11 = p1[1+4];
            unsigned char b11 = p1[0+4];

            do {
                if (a00 < 0x8)
                    break;
                unsigned char y00 = to_y(r00, g00, b00);
                if (a00 < 0xf8)
                    y00 = UBLEND(y00, Y0[0], a00);
                Y0[0] = y00;
            } while (0);

            do {
                if (a01 < 0x8)
                    break;
                unsigned char y01 = to_y(r01, g01, b01);
                if (a01 < 0xf8)
                    y01 = UBLEND(y01, Y0[1], a01);
                Y0[1] = y01;
            } while (0);

            do {
                if (a10 < 0x8)
                    break;
                unsigned char y10 = to_y(r10, g10, b10);
                if (a10 < 0xf8)
                    y10 = UBLEND(y10, Y1[0], a10);
                Y1[0] = y10;
            } while (0);

            do {
                if (a11 < 0x8)
                    break;
                unsigned char y11 = to_y(r11, g11, b11);
                if (a11 < 0xf8)
                    y11 = UBLEND(y11, Y1[1], a11);
                Y1[1] = y11;
            } while (0);


            a00 = ((unsigned int) a00 + a01 + a10 + a11)/4;

            do {
                if (a00 < 0x8)
                    break;
                r00 = ((unsigned int) r00 + r01 + r10 + r11)/4;
                g00 = ((unsigned int) g00 + g01 + g10 + g11)/4;
                b00 = ((unsigned int) b00 + b01 + b10 + b11)/4;

                unsigned char u00 = to_u(r00, g00, b00);
                unsigned char v00 = to_v(r00, g00, b00);

                if (a00 < 0xf8) {
                    u00 = UBLEND(u00, uv[0], a00);
                    v00 = UBLEND(v00, uv[1], a00);
                }
                uv[0] = u00;
                uv[1] = v00;
            } while (0);

            p0 += 8;
            p1 += 8;
            uv += 2;
            Y0 += 2;
            Y1 += 2;
        }
    }
}

static void c_blend_yuva420p_to_nv12(void *src_y, int src_y_linesize,
                              void *src_u, int src_u_linesize,
                              void *src_v, int src_v_linesize,
                              void *src_a, int src_a_linesize,
                              void *dst_y, int dst_y_linesize,
                              void *dst_uv, int dst_uv_linesize,
                              int width, int height, uint8_t alpha)
{
    int i, j;
    uint8_t *uv = dst_uv;
    uint8_t *u = src_u;
    uint8_t *v = src_v;

    for (i = 0; i < height; i+=2) {
        uint8_t *dst_y0 = dst_y + i * dst_y_linesize;
        uint8_t *dst_y1 = dst_y0 + dst_y_linesize;

        uint8_t *src_y0 = src_y + i * src_y_linesize;
        uint8_t *src_y1 = src_y0 + src_y_linesize;

        uint8_t *src_a0 = src_a + i * src_a_linesize;
        uint8_t *src_a1 = src_a0 +  src_a_linesize;


        for (j = 0; j < width; j+=2) {
            unsigned char a00 = src_a0[j];
            unsigned char a01 = src_a0[j + 1];
            unsigned char a10 = src_a1[j];
            unsigned char a11 = src_a1[j + 1];

            if (alpha < 0xf8) {
                a00 = (a00 * alpha) >> 8;
                a01 = (a01 * alpha) >> 8;
                a10 = (a10 * alpha) >> 8;
                a11 = (a11 * alpha) >> 8;
            }

            dst_y0[j] = UBLEND(src_y0[j], dst_y0[j], a00);
            dst_y0[j + 1] = UBLEND(src_y0[j + 1], dst_y0[j + 1], a01);

            dst_y1[j] = UBLEND(src_y1[j], dst_y1[j], a10);
            dst_y1[j + 1] = UBLEND(src_y1[j + 1], dst_y1[j + 1], a11);

            unsigned char a_uv = ((unsigned int) a00 + a01 + a10 + a11) / 4;

            uv[j] = UBLEND(u[j / 2], uv[j], a_uv);
            uv[j + 1] = UBLEND(v[j / 2], uv[j + 1], a_uv);
        }

        v += src_u_linesize;
        u += src_v_linesize;
        uv += dst_uv_linesize;
    }

}


void blend_bgra_to_nv12(uint8_t **s_data, uint32_t *s_linesize,
                        uint8_t **d_data, uint32_t *d_linesize,
                        int width, int height, uint8_t alpha)
{
    void *src_bgra = s_data[0];
    int src_linesize = s_linesize[0];

    void *dst_y = d_data[0];
    void *dst_uv = d_data[1];
    int dst_y_linesize = d_linesize[0];
    int dst_uv_linesize = d_linesize[1];

#if defined (__mips_msa)

    msa_blend_bgra_to_nv12(src_bgra, src_linesize, dst_y, dst_y_linesize,
                           dst_uv, dst_uv_linesize, width, height, alpha);
    return;
#endif

    c_blend_bgra_to_nv12(src_bgra, src_linesize, dst_y, dst_y_linesize,
                         dst_uv, dst_uv_linesize, width, height, alpha);

}


void blend_yuva420p_to_nv12(uint8_t **s_data, uint32_t *s_linesize,
                            uint8_t **d_data, uint32_t *d_linesize,
                            int width, int height, uint8_t alpha)
{
    void *src_y = s_data[0];
    void *src_u = s_data[1];
    void *src_v = s_data[2];
    void *src_a = s_data[3];
    int src_y_linesize = s_linesize[0];
    int src_u_linesize = s_linesize[1];
    int src_v_linesize = s_linesize[2];
    int src_a_linesize = s_linesize[3];

    void *dst_y = d_data[0];
    void *dst_uv = d_data[1];
    int dst_y_linesize = d_linesize[0];
    int dst_uv_linesize = d_linesize[1];

#if defined (__mips_msa)
    msa_blend_yuva420p_to_nv12(src_y, src_y_linesize, src_u, src_u_linesize,
                               src_v, src_v_linesize, src_a, src_a_linesize,
                               dst_y, dst_y_linesize, dst_uv, dst_uv_linesize,
                               width, height, alpha);

    return;
#endif
    c_blend_yuva420p_to_nv12(src_y, src_y_linesize, src_u, src_u_linesize,
                             src_v, src_v_linesize, src_a, src_a_linesize,
                             dst_y, dst_y_linesize, dst_uv, dst_uv_linesize,
                             width, height, alpha);
}

static void c_blend_alpha_color_y(
    uint8_t *src, int src_linesize,
    uint8_t *dst, int dst_linesize,
    int width, int height, uint8_t y_color, uint8_t alpha)
{
    int i, j;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width-1; i+=2) {
            uint8_t a0 = src[i+0];
            uint8_t a1 = src[i+1];

            uint8_t d0 = dst[i+0];
            uint8_t d1 = dst[i+1];

            if (alpha != 0xff) {
                a0 = (a0 * alpha) >> 8;
                a1 = (a1 * alpha) >> 8;
            }

            do {
                if (a0 < 0x8)
                    break;
                if (a0 < 0xf8)
                    d0 = UBLEND(y_color, d0, a0);
                else
                    d0 = y_color;
                dst[i+0] = d0;
            } while (0);

            do {
                if (a1 < 0x8)
                    break;
                if (a1 < 0xf8)
                    d1 = UBLEND(y_color, d1, a1);
                else
                    d1 = y_color;
                dst[i+1] = d1;
            } while (0);
        }
        if (width & 1) {
            uint8_t a0 = src[i+0];
            uint8_t d0 = dst[i+0];

            if (alpha != 0xff)
                a0 = (a0 * alpha) >> 8;

            d0 = UBLEND(y_color, d0, a0);
            dst[i+0] = d0;
        }

        src += src_linesize;
        dst += dst_linesize;
    }

}

static void c_blend_alpha_color_uv(
    uint8_t *src, int src_linesize, uint8_t *dst, int dst_linesize,
    int width, int height, uint8_t u_color, uint8_t v_color, uint8_t alpha)
{
    int i, j;

    uint8_t *s0 = src;
    uint8_t *s1 = src + src_linesize;

    for (j = 0; j < height; j+=2) {
        for (i = 0; i < width; i+=2) {
            uint8_t a0 = s0[i+0];
            uint8_t a1 = s0[i+1];
            uint8_t a2 = s1[i+0];
            uint8_t a3 = s1[i+1];

            uint8_t u = dst[i+0];
            uint8_t v = dst[i+1];

            a0 = ((uint32_t)a0 + a1 + a2 + a3) / 4;

            if (alpha != 0xff)
                a0 = (a0 * alpha) >> 8;

            if (a0 < 0x8)
                continue;

            if (a0 < 0xf8) {
                u = UBLEND(u_color, u, a0);
                v = UBLEND(v_color, v, a0);
                dst[i+0] = u;
                dst[i+1] = v;
            } else {
                dst[i+0] = u_color;
                dst[i+1] = v_color;
            }
        }

        s0 += src_linesize*2;
        s1 += src_linesize*2;
        dst += dst_linesize;
    }
}

static void c_blend_alpha_color_nv12(
    uint8_t *src, int src_linesize,
    uint8_t *y, int y_linesize,
    uint8_t *uv, int uv_linesize,
    int width, int height, unsigned int color)
{
    int i, j;

    unsigned char alpha = (color >> 24) & 0xff;
    unsigned char r = (color >> 16) & 0xff;
    unsigned char g = (color >> 8) & 0xff;
    unsigned char b = (color >> 0) & 0xff;

    uint8_t y_color = to_y(r, g, b);
    uint8_t u_color = to_u(r, g, b);
    uint8_t v_color = to_v(r, g, b);

    uint8_t *s0 = src;
    uint8_t *s1 = src + src_linesize;

    uint8_t *y0 = y;
    uint8_t *y1 = y + y_linesize;

    for (j = 0; j < height; j+=2) {
        for (i = 0; i < width; i+=2) {
            uint8_t a0 = s0[i+0];
            uint8_t a1 = s0[i+1];
            uint8_t a2 = s1[i+0];
            uint8_t a3 = s1[i+1];

            uint8_t d0 = y0[i+0];
            uint8_t d1 = y0[i+1];
            uint8_t d2 = y1[i+0];
            uint8_t d3 = y1[i+1];

            uint8_t u = uv[i+0];
            uint8_t v = uv[i+1];

            if (alpha < 0xf8) {
                a0 = (a0 * alpha) >> 8;
                a1 = (a1 * alpha) >> 8;
                a2 = (a2 * alpha) >> 8;
                a3 = (a3 * alpha) >> 8;
            }

            do {
                if (a0 < 0x8)
                    break;
                if (a0 < 0xf8)
                    d0 = UBLEND(y_color, d0, a0);
                else
                    d0 = y_color;
                y0[i+0] = d0;
            } while (0);

            do {
                if (a1 < 0x8)
                    break;
                if (a1 < 0xf8)
                    d1 = UBLEND(y_color, d1, a1);
                else
                    d1 = y_color;
                y0[i+1] = d1;
            } while (0);

            do {
                if (a2 < 0x8)
                    break;
                if (a2 < 0xf8)
                    d2 = UBLEND(y_color, d2, a2);
                else
                    d2 = y_color;
                y1[i+0] = d2;
            } while (0);

            do {
                if (a3 < 0x8)
                    break;
                if (a3 < 0xf8)
                    d3 = UBLEND(y_color, d3, a3);
                else
                    d3 = y_color;
                y1[i+1] = d3;
            } while (0);

            a0 = ((uint32_t)a0 + a1 + a2 + a3) / 4;

            if (alpha != 0xff)
                a0 = (a0 * alpha) >> 8;

            if (a0 < 0x8)
                continue;

            if (a0 < 0xf8) {
                u = UBLEND(u_color, u, a0);
                v = UBLEND(v_color, v, a0);
                uv[i+0] = u;
                uv[i+1] = v;
            } else {
                uv[i+0] = u_color;
                uv[i+1] = v_color;
            }
        }

        s0 += 2*src_linesize;
        s1 += 2*src_linesize;
        y0 += 2*y_linesize;
        y1 += 2*y_linesize;
        uv += uv_linesize;
    }
}

void blend_alpha_color_y(
    uint8_t *src, int src_linesize,
    uint8_t *dst, int dst_linesize,
    int width, int height, uint8_t y_color, uint8_t alpha)
{
    c_blend_alpha_color_y(src, src_linesize, dst, dst_linesize,
                          width, height, y_color, alpha);
}

void blend_alpha_color_uv(
    uint8_t *src, int src_linesize, uint8_t *dst, int dst_linesize,
    int width, int height, uint8_t u_color, uint8_t v_color, uint8_t alpha)
{
    c_blend_alpha_color_uv(src, src_linesize, dst, dst_linesize,
                           width, height, u_color, v_color, alpha);
}

void blend_alpha_color_nv12(
    uint8_t *src, int src_linesize,
    uint8_t *y, int y_linesize,
    uint8_t *uv, int uv_linesize,
    int width, int height, unsigned int color)
{
#if defined (__mips_msa)
    msa_blend_alpha_color_nv12(src, src_linesize, y, y_linesize,
                               uv, uv_linesize, width, height, color);

    return;
#endif

    c_blend_alpha_color_nv12(src, src_linesize, y, y_linesize,
                             uv, uv_linesize, width, height, color);
}