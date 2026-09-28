#if defined(__mips_msa)
#include <libmedia/video_frame.h>
#include <libmedia/rgb_to_yuv.h>
#include <msa.h>
#include "../convert/msa_convert.c"

static v16i8 sub_alpha = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

extern v16i8 y_r_mut;
extern v16i8 y_g_mut;
extern v16i8 y_b_mut;

extern v8i16 u_r_mut;
extern v8i16 u_g_mut;
extern v8i16 u_b_mut;

extern v8i16 v_r_mut;
extern v8i16 v_g_mut;
extern v8i16 v_b_mut;
extern v16i8 uv_add;

/**
 * MSA_UBLEND:将上层数据（f）, 根据其透明度（a），叠加到底层数据(b)
 *
 * v16i8 f, b, a;
 * v16u8 b_alpha;
 * v8u16 f0, f1, b0, b1;
 *
 * b = (f*a + (255 - a) * b) >> 8
 *
 * f*a:
 * __msa_mulur_h(f, a)  ---------> f0 = {f[0]*a[0], f[1]*a[1], ....., f[7]*a[7],}
 * __msa_mulul_h(f, a)  ---------> f1 = {f[8]*a[8], f[8]*a[8], ....., f[15]*a[15]}
 *
 * 255-a:
 * __msa_subs_u_b(sub_alpha, a) ------> b_alpha = {255-a[0], 255-a[1], 255-a[2], ....., 255-a[15]}
 *
 * (255-a)*b:
 * __msa_mulur_h(b, b_alpha)  ---------> b0 = {b[0]*b_alpha[0], b[1]*b_alpha[1], ....., b[7]*b_alpha[7],}
 * __msa_mulul_h(b, b_alpha)  ---------> b1 = {b[8]*b_alpha[8], b[8]*b_alpha[8], ....., b[15]*b_alpha[15]}
 *
 * (f*a) + ((255-a)*b):
 * __msa_addv_h(b0,f0) -------> f0 = {b0[0]+f0[0], b0[1]+f0[1], ......, b0[7]+f0[7]}
 * __msa_addv_h(b1,f1) -------> f1 = {b1[0]+f1[0], b1[1]+f1[1], ......, b1[7]+f1[7]}
 *
 * ((f*a) + ((255-a)*b)) >> 8:
 * 每16位只取高8位：
 * __msa_pckod_b(f1,f0) ------> b = {f0[1], f0[3], f0[5], ......, f0[15],
 *                                           f1[1], f1[3], f1[5], ......, f1[15]},
 */

#define MSA_UBLEND(f, b, a)\
do {\
    v8u16 f0 = __msa_mulur_h((v16u8)f, (v16u8)a);\
    v8u16 f1 = __msa_mulul_h((v16u8)f, (v16u8)a);\
\
    v16u8 b_alpha = __msa_subs_u_b((v16u8)sub_alpha, (v16u8)a);\
\
    v8u16 b0 = __msa_mulur_h((v16u8)b, (v16u8)b_alpha);\
    v8u16 b1 = __msa_mulul_h((v16u8)b, (v16u8)b_alpha);\
\
    f0 = (v8u16)__msa_addv_h((v8i16)b0, (v8i16)f0);\
    f1 = (v8u16)__msa_addv_h((v8i16)b1, (v8i16)f1);\
\
    b =  __msa_pckod_b((v16i8)f1, (v16i8)f0);\
}while(0)


/**
 * msa_blend_alpha: 计算叠加数据的透明度
 * (f_alpha * b_alpha) >> 8
 *
 * v16i8 f_alpha, b_alpha
 * v8u16 f_a0, f_a1
 *
 * f_alpha*b_alpha:
 * __msa_mulur_h ------> f_a0 = {f_alpha[0]*b_alpha[0], f_alpha[1]*b_alpha[1], ....., f_alpha[7]*b_alpha[7]}
 * __msa_mulul_h ------> f_a1 = {f_alpha[8]*b_alpha[8], f_alpha[9]*b_alpha[9], ....., f_alpha[15]*b_alpha[5]}
 *
 * (f_alpha * b_alpha) >> 8：
 * 每16位只取高8位：
 * __msa_pckod_b ------> d_alpha = {f_a0[1], f_a0[3], f_a0[5], ....., f_a0[15],
 *                                          f_a1[1], f_a1[3], f_a1[5], ....., f_a1[15]}
 *
 */
#define msa_blend_alpha(f_alpha, b_alpha, d_alpha)\
do {\
    v8u16 f_a0 = __msa_mulur_h((v16u8)f_alpha, (v16u8)b_alpha);\
    v8u16 f_a1 = __msa_mulul_h((v16u8)f_alpha, (v16u8)b_alpha);\
\
    d_alpha = __msa_pckod_b((v16i8)f_a1, (v16i8)f_a0);\
\
}while(0)


/**
 * msa_aver_uv_alpha: 计算uv 透明度的平均值
 *
 * (a00 + a01 + a10 + a11) / 4
 *
 * a00 + a01:
 * __msa_hadd_u_h(a0, a0) -----> uv_a0 = {a0[0]+a0[1], a0[2]+a0[3], a0[4]+a0[5], ....., a0[14]+a0[15]}
 *
 * a10 + a11:
 * __msa_hadd_u_h(a1, a1) -----> uv_a1 = {a1[0]+a1[1], a1[2]+a1[3], a1[4]+a1[5], ....., a1[14]+a1[15]}
 *
 * (a00 + a01) + (a10 + a11):
 * __msa_addv_h(uv_a0, uv_a1) ------> uv_a0 = {uv_a0[0]+uv_a1[0], uv_a0[1]+uv_a1[1], ......, uv_a0[7]+uv_a1[7]}
 *
 * ((a00 + a01) + (a10 + a11)) / 4:
 * __msa_srai_h(uv_a0, 2) ------> uv_a0 = {uv_a0[0]>>2, uv_a0[1]>>2, uv_a0[2]>>2, ......, uv_a0[7]>>2}
 *
 * u v的透明度是一样的：
 * 每16位只取低8位：
 * __msa_ilvev_b(uv_a0, uv_a0) ------> dst_a = {uv_a0[0], uv_a0[0], uv_a0[2], uv_a0[2], ....., uv_a0[7], uv_a0[7]}
 *
*/
#define msa_aver_uv_alpha(a0, a1, dst_a)\
do {\
    v8u16 uv_a0 = __msa_hadd_u_h((v16u8)a0, (v16u8)a0);\
    v8u16 uv_a1 = __msa_hadd_u_h((v16u8)a1, (v16u8)a1);\
    uv_a0 = (v8u16)__msa_addv_h((v8i16)uv_a0, (v8i16)uv_a1);\
    uv_a0 = (v8u16)__msa_srai_h((v8i16)uv_a0, 2);\
    dst_a = __msa_ilvev_b((v16i8)uv_a0, (v16i8)uv_a0);\
\
}while(0)

static void msa_blend_plane(uint8_t *src, int src_linesize, uint8_t *dst, int dst_linesize, int width, int height, uint8_t alpha)
{
    /* src 几乎全透明,所以对dst 没有影响
     */
    if (alpha <= 0x08)
        return;

    /* src 几乎不透明,所以直接覆盖dst
     */
    if (alpha >= 0xf8)
        return copy_raw_bytes(dst, dst_linesize, src, src_linesize, height, width);

    v16i8 a0 = __msa_fill_b(alpha);

    int i, j;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i+=16) {
            v16i8 s0 = __msa_ld_b(src + i, 0);
            v16i8 d0 = __msa_ld_b(dst + i, 0);

            MSA_UBLEND(s0, d0, a0);

            __msa_st_b(d0, dst + i, 0);
        }

        for (;i < width; i++) {
            uint8_t s = src[i+0];
            uint8_t d = dst[i+0];

            d = UBLEND(s, d, alpha);
            dst[i] = d;
        }

        src += src_linesize;
        dst += dst_linesize;
    }

}

void msa_blend_alpha_color_nv12(
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

    v16i8 msa_y_color = __msa_fill_b(y_color);
    v16i8 msa_u_color = __msa_fill_b(u_color);
    v16i8 msa_v_color = __msa_fill_b(v_color);
    v16i8 msa_uv_color = __msa_ilvev_b(msa_v_color, msa_u_color);
    v16i8 msa_alpha = __msa_fill_b(alpha);

    uint8_t *s0 = src;
    uint8_t *s1 = src + src_linesize;

    uint8_t *y0 = y;
    uint8_t *y1 = y + y_linesize;

    for (j = 0; j < height; j+=2) {
        for (i = 0; i < width; i+=16) {
            v16i8 a0 = __msa_ld_b(s0 + i, 0);
            v16i8 a1 = __msa_ld_b(s1 + i, 0);

            v16i8 d0 = __msa_ld_b(y0 + i, 0);
            v16i8 d1 = __msa_ld_b(y1 + i, 0);

            v16i8 y_a0, y_a1;
            if (alpha < 0xf8) {
                msa_blend_alpha(a0, msa_alpha, y_a0);
                msa_blend_alpha(a1, msa_alpha, y_a1);
            } else {
                y_a0 = a0;
                y_a1 = a1;
            }

            MSA_UBLEND(msa_y_color, d0, y_a0);
            MSA_UBLEND(msa_y_color, d1, y_a1);

            __msa_st_b(d0, y0 + i, 0);
            __msa_st_b(d1, y1 + i, 0);

            v16i8 uv_a;
            msa_aver_uv_alpha(y_a0, y_a1, uv_a);

            d0 = __msa_ld_b(uv + i, 0);
            MSA_UBLEND(msa_uv_color, d0, uv_a);

            __msa_st_b(d0, uv + i, 0);
        }

        for (;i < width; i+=2) {
            uint8_t a00 = s0[i+0];
            uint8_t a01 = s0[i+1];
            uint8_t a10 = s1[i+0];
            uint8_t a11 = s1[i+1];

            uint8_t d00 = y0[i+0];
            uint8_t d01 = y0[i+1];
            uint8_t d10 = y1[i+0];
            uint8_t d11 = y1[i+1];

            uint8_t u00 = uv[i+0];
            uint8_t v00 = uv[i+1];

            if (alpha < 0xf8) {
                a00 = (a00 * alpha) >> 8;
                a01 = (a01 * alpha) >> 8;
                a10 = (a10 * alpha) >> 8;
                a11 = (a11 * alpha) >> 8;
            }

            do {
                if (a00 < 0x8)
                    break;
                if (a00 < 0xf8)
                    d00 = UBLEND(y_color, d00, a00);
                else
                    d00 = y_color;
                y0[i+0] = d00;
            } while (0);

            do {
                if (a01 < 0x8)
                    break;
                if (a01 < 0xf8)
                    d01 = UBLEND(y_color, d01, a01);
                else
                    d01 = y_color;
                y0[i+1] = d01;
            } while (0);

            do {
                if (a10 < 0x8)
                    break;
                if (a10 < 0xf8)
                    d10 = UBLEND(y_color, d10, a10);
                else
                    d10 = y_color;
                y1[i+0] = d10;
            } while (0);

            do {
                if (a11 < 0x8)
                    break;
                if (a11 < 0xf8)
                    d11 = UBLEND(y_color, d11, a11);
                else
                    d11 = y_color;
                y1[i+1] = d11;
            } while (0);

            a00 = ((uint32_t)a00 + a01 + a10 + a11) / 4;

            if (alpha != 0xff)
                a00 = (a00 * alpha) >> 8;

            if (a00 < 0x8)
                continue;

            if (a00 < 0xf8) {
                u00 = UBLEND(u_color, u00, a00);
                v00 = UBLEND(v_color, v00, a00);
                uv[i+0] = u00;
                uv[i+1] = v00;
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


void msa_blend_bgra_to_nv12(void *src_bgra, int src_linesize,
                        void *dst_y, int dst_y_linesize,
                        void *dst_uv, int dst_uv_linesize,
                        int width, int height, uint8_t alpha)
{
    int i, j;

    v16i8 f_alpha = __msa_fill_b(alpha);

    for (i = 0; i < height; i+=2) {
        unsigned char *p0 = src_bgra + i*src_linesize;
        unsigned char *p1 = p0 + src_linesize;

        unsigned char *Y0 = dst_y + i * dst_y_linesize;
        unsigned char *Y1 = Y0 + dst_y_linesize;
        unsigned char *uv = dst_uv + i*dst_uv_linesize/2;

        for (j = 0; j < width; j += 16) {
            v16i8 b0, b1;
            v16i8 g0, g1;
            v16i8 r0, r1;
            v16i8 a0, a1;

            msa_ld_bgra(p0, b0, g0, r0, a0);
            msa_ld_bgra(p1, b1, g1, r1, a1);

            v16i8 y_a0, y_a1;
            if (alpha < 0xf8) {
                msa_blend_alpha(f_alpha, a0, y_a0);
                msa_blend_alpha(f_alpha, a1, y_a1);
            } else {
                y_a0 = a0;
                y_a1 = a1;
            }

            v16i8 s_y0, s_y1;
            msa_to_y(b0, g0, r0, s_y0);
            msa_to_y(b1, g1, r1, s_y1);

            v16i8 d_y0 = __msa_ld_b(Y0, 0);
            v16i8 d_y1 = __msa_ld_b(Y1, 0);

            MSA_UBLEND(s_y0, d_y0, y_a0);
            MSA_UBLEND(s_y1, d_y1, y_a1);

            __msa_st_b(d_y0, Y0, 0);
            __msa_st_b(d_y1, Y1, 0);

            /*b数据以4个为单位相加(单位：2byte)，unsigned short = unsigned char + unsigned char*/
            v8u16 bh0 = __msa_hadd_u_h((v16u8)b0, (v16u8)b0);
            v8u16 bh1 = __msa_hadd_u_h((v16u8)b1, (v16u8)b1);
            bh0 = (v8u16)__msa_addv_h((v8i16)bh0, (v8i16)bh1);

            /*g数据以4个为单位相加（单位：2byte），unsigned short = unsigned char + unsigned char*/
            v8u16 gh0 = __msa_hadd_u_h((v16u8)g0, (v16u8)g0);
            v8u16 gh1 = __msa_hadd_u_h((v16u8)g1, (v16u8)g1);
            gh0 = (v8u16)__msa_addv_h((v8i16)gh0, (v8i16)gh1);

            /*数据以4个为单位相加（单位：2byte），unsigned short = unsigned char + unsigned char*/
            v8u16 rh0 = __msa_hadd_u_h((v16u8)r0, (v16u8)r0);
            v8u16 rh1 = __msa_hadd_u_h((v16u8)r1, (v16u8)r1);
            rh0 = (v8u16)__msa_addv_h((v8i16)rh0, (v8i16)rh1);

            /*r、g、b分别取平均值（除以4）（单位：2byte）*/
            bh0 = (v8u16)__msa_srai_h((v8i16)bh0, 2);
            gh0 = (v8u16)__msa_srai_h((v8i16)gh0, 2);
            rh0 = (v8u16)__msa_srai_h((v8i16)rh0, 2);

            v16i8 s_uv;
            msa_to_uv(bh0, gh0, rh0, s_uv);

            v16i8 uv_a;
            msa_aver_uv_alpha(y_a0, y_a1, uv_a);

            v16i8 d_uv = __msa_ld_b(uv, 0);
            MSA_UBLEND(s_uv, d_uv, uv_a);

            __msa_st_b(d_uv, uv, 0);

            p0 += 64;
            p1 += 64;
            Y0 += 16;
            Y1 += 16;
            uv += 16;
        }

        for (j = 0; j < width % 16; j += 2) {
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

            unsigned char y00 = to_y(r00, g00, b00);
            unsigned char y01 = to_y(r01, g01, b01);
            unsigned char y10 = to_y(r10, g10, b10);
            unsigned char y11 = to_y(r11, g11, b11);

            r00 = ((unsigned int) r00 + r01 + r10 + r11)/4;
            g00 = ((unsigned int) g00 + g01 + g10 + g11)/4;
            b00 = ((unsigned int) b00 + b01 + b10 + b11)/4;

            unsigned char u00 = to_u(r00, g00, b00);
            unsigned char v00 = to_v(r00, g00, b00);

            Y0[0] = UBLEND(y00, Y0[0], a00);
            Y0[1] = UBLEND(y01, Y0[1], a01);

            Y1[0] = UBLEND(y10, Y1[0], a10);
            Y1[1] = UBLEND(y11, Y1[1], a11);

            a00 = ((unsigned int) a00 + a01 + a10 + a11)/4;

            uv[0] = UBLEND(u00, uv[0], a00);
            uv[1] = UBLEND(v00, uv[1], a00);

            p0 += 8;
            p1 += 8;
            uv += 2;
            Y0 += 2;
            Y1 += 2;
        }
    }
}

void msa_blend_yuva420p_to_nv12(void *src_y, int src_y_linesize,
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

    v16i8 f_alpha = __msa_fill_b(alpha);

    for (i = 0; i < height; i+=2) {
        uint8_t *dst_y0 = dst_y + i * dst_y_linesize;
        uint8_t *dst_y1 = dst_y0 + dst_y_linesize;

        uint8_t *src_y0 = src_y + i * src_y_linesize;
        uint8_t *src_y1 = src_y0 + src_y_linesize;

        uint8_t *src_a0 = src_a + i * src_a_linesize;
        uint8_t *src_a1 = src_a0 +  src_a_linesize;

        for (j = 0; j < width; j+=16) {
            v16i8 s_a0 = __msa_ld_b(src_a0 + j, 0);
            v16i8 s_a1 = __msa_ld_b(src_a1 + j, 0);

            v16i8 s_y0 = __msa_ld_b(src_y0 + j, 0);
            v16i8 s_y1 = __msa_ld_b(src_y1 + j, 0);

            v16i8 d_y0 = __msa_ld_b(dst_y0 + j, 0);
            v16i8 d_y1 = __msa_ld_b(dst_y1 + j, 0);

            v16i8 y_a0, y_a1;
            if (alpha < 0xf8) {
                msa_blend_alpha(f_alpha, s_a0, y_a0);
                msa_blend_alpha(f_alpha, s_a1, y_a1);
            } else {
                y_a0 = s_a0;
                y_a1 = s_a1;
            }

            MSA_UBLEND(s_y0, d_y0, y_a0);
            MSA_UBLEND(s_y1, d_y1, y_a1);

            __msa_st_b(d_y0, dst_y0 + j, 0);
            __msa_st_b(d_y1, dst_y1 + j, 0);

            v16i8 uv_a;
            msa_aver_uv_alpha(y_a0, y_a1, uv_a);

            v2i64 s_u = {0};
            v2i64 s_v = {0};
            s_u = __msa_ldins_d(s_u, 0, u + j / 2, 0);
            s_v = __msa_ldins_d(s_v, 0, v + j / 2, 0);
            v16i8 s_uv = __msa_ilvr_b((v16i8)s_v, (v16i8)s_u);

            v16i8 d_uv = __msa_ld_b(uv + j, 0);

            MSA_UBLEND(s_uv, d_uv, uv_a);
            __msa_st_b(d_uv, uv + j, 0);
        }

        for (;j < width; j+=2) {
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
#endif
