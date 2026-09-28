#include "base/g2d.h"

#ifdef APP_awtk_WITH_INGENIC_MSA_G2D
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LV_OPA_MIN 2
#define LV_OPA_MAX 252

#define blend_pixel_no_premulti(_sr, _sg, _sb, __sa, _alpha, load_dst, store_dst) \
do {                                              \
    uint8_t _r, _g, _b;                           \
    uint8_t _sa = __sa;                           \
                                                  \
    if (_alpha != 0xff)                           \
        _sa = (_sa * _alpha) >> 8;                \
                                                  \
    /* src_a < 0x8 时,src近乎全透明 */              \
    /* 所以底色还是底色 */                           \
    if (_sa <= LV_OPA_MIN)                        \
         break;                                   \
                                                  \
    /* src_a > 0xf8 时,src几乎不透明 */             \
    /* 所以直接覆盖底色 */                           \
    if (_sa >= LV_OPA_MAX) {                      \
        store_dst(_sr, _sg, _sb, 0xff);           \
        break;                                    \
    }                                             \
                                                  \
    uint8_t _dr, _dg, _db, _da;                   \
    load_dst(_dr, _dg, _db, _da);                 \
                                                  \
    /* _da < 0x08 可认为是 0x00, 可以简化计算公式 */  \
    if (_da <= LV_OPA_MIN) {                      \
        store_dst(_sr, _sg, _sb, _sa);            \
        break;                                    \
    }                                             \
                                                  \
    /* _da > 0xf4 可认为是 0xff, 可以简化计算公式*/   \
    if (_da >= LV_OPA_MAX) {                      \
        _da = 0xff - _sa;                         \
        _r = (_dr * _da + _sr * _sa) >> 8;        \
        _g = (_dg * _da + _sg * _sa) >> 8;        \
        _b = (_db * _da + _sb * _sa) >> 8;        \
        store_dst(_r, _g, _b, 0xff);              \
        break;                                    \
    }                                             \
                                                  \
    int out_a = _sa + _da - ((_sa * _da) >> 8);   \
    _da = (_da * (0xff - _sa)) >> 8;              \
                                                  \
    if (out_a > 255)                              \
        out_a = 255;                              \
                                                  \
    _r = (_dr * _da + _sr * _sa) / out_a;         \
    _g = (_dg * _da + _sg * _sa) / out_a;         \
    _b = (_db * _da + _sb * _sa) / out_a;         \
    store_dst(_r, _g, _b, out_a);                 \
} while (0)

#define load_d(_r, _g, _b, _a)  \
{                               \
    _b = d[0];                  \
    _g = d[1];                  \
    _r = d[2];                  \
    _a = d[3];                  \
}

#define store_d(_r, _g, _b, _a) \
{                               \
    d[0] = _b;                  \
    d[1] = _g;                  \
    d[2] = _r;                  \
    d[3] = _a;                  \
}

#ifdef __mips_msa
#include <msa.h>

#define msa_to_bgra(d0, d1, d2, d3, b, g, r, a)\
do {\
    v16i8 br00 = __msa_pckev_b(d1, d0);\
    v16i8 br01 = __msa_pckev_b(d3, d2);\
\
    v16i8 ga00 = __msa_pckod_b(d1, d0);\
    v16i8 ga01 = __msa_pckod_b(d3, d2);\
\
    b = __msa_pckev_b(br01, br00);\
    g = __msa_pckev_b(ga01, ga00);\
    r = __msa_pckod_b(br01, br00);\
    a = __msa_pckod_b(ga01, ga00);\
} while(0)

#define msa_ld_bgra(p, d0, d1, d2, d3, b, g, r, a)\
do {\
    d0 = __msa_ld_b(p, 0);\
    d1 = __msa_ld_b(p + 16, 0);\
    d2 = __msa_ld_b(p + 32, 0);\
    d3 = __msa_ld_b(p + 48, 0);\
\
    msa_to_bgra(d0, d1, d2, d3, b, g, r, a); \
} while(0)

#define msa_st_bgra(p, b, g, r, a) \
do { \
    v16i8 ra0 = (v16i8)__msa_ilvr_b((v16i8)a, (v16i8)r); \
    v16i8 ra1 = (v16i8)__msa_ilvl_b((v16i8)a, (v16i8)r); \
    v16i8 bg0 = (v16i8)__msa_ilvr_b((v16i8)g, (v16i8)b); \
    v16i8 bg1 = (v16i8)__msa_ilvl_b((v16i8)g, (v16i8)b); \
 \
    v4i32 c0 = (v4i32)__msa_ilvr_h((v8i16)ra0, (v8i16)bg0); \
    v4i32 c1 = (v4i32)__msa_ilvl_h((v8i16)ra0, (v8i16)bg0); \
    v4i32 c2 = (v4i32)__msa_ilvr_h((v8i16)ra1, (v8i16)bg1); \
    v4i32 c3 = (v4i32)__msa_ilvl_h((v8i16)ra1, (v8i16)bg1); \
 \
    __msa_st_w((v4i32)c0, p, 0*16); \
    __msa_st_w((v4i32)c1, p, 1*16); \
    __msa_st_w((v4i32)c2, p, 2*16); \
    __msa_st_w((v4i32)c3, p, 3*16); \
 \
} while (0) \


static void blend_argb_full_msa(uint8_t *d, int w, int h, int d_stride,
     uint8_t *s, int s_stride, int alpha, int s_is_bgra, uint32_t color)
{
    int x, y;

    int s_delta = (s_stride-w)*4;
    int d_delta = (d_stride-w)*4;

    v16u8 a = {alpha, alpha, alpha, alpha,
               alpha, alpha, alpha, alpha,
               alpha, alpha, alpha, alpha,
               alpha, alpha, alpha, alpha};

    v16i8 aff = {0xff, 0xff, 0xff, 0xff,
                 0xff, 0xff, 0xff, 0xff,
                 0xff, 0xff, 0xff, 0xff,
                 0xff, 0xff, 0xff, 0xff};

    v16u8 amax = {LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1,
                  LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1,
                  LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1,
                  LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1};

    v8u16 a00ff = {0x00ff, 0x00ff,
                   0x00ff, 0x00ff,
                   0x00ff, 0x00ff,
                   0x00ff, 0x00ff,};

    v16i8 s0, s1, s2, s3;
    v16i8 s_b, s_g, s_r, s_a;

    int is_color = !s;
    v4i32 color_v = {color, color, color, color};

    if (is_color) { // 编译器会说或许没有初始化
        s0 = (v16i8)color_v; s1 = (v16i8)color_v,
        s2 = (v16i8)color_v; s3 = (v16i8)color_v;
        msa_to_bgra(s0,s1,s2,s3, s_b, s_g, s_r, s_a);
        if (alpha < LV_OPA_MAX) {
            v8u16 t0 = __msa_mulur_h((v16u8)s_a, a);
            v8u16 t1 = __msa_mulul_h((v16u8)s_a, a);
            s_a = __msa_pckod_b((v16i8)t1, (v16i8)t0);
        }
        if (alpha <= LV_OPA_MIN)
            return;
    }
    int save_alpha = alpha;
    if (!s_is_bgra)
        alpha = 0xff;

    for(y = 0; y < h; y++) {
        for(x = 0; x < w/16; x++) {
            v16i8 d_b, d_g, d_r, d_a;
            v16i8 d0, d1, d2, d3;
            v16i8 result;

            if (!is_color) {
                msa_ld_bgra(s, s0,s1,s2,s3, s_b, s_g, s_r, s_a);

                if (!s_is_bgra)
                    s_a = (v16i8)a;
                else if (alpha < LV_OPA_MAX) {
                    v8u16 t0 = __msa_mulur_h((v16u8)s_a, a);
                    v8u16 t1 = __msa_mulul_h((v16u8)s_a, a);
                    s_a = __msa_pckod_b((v16i8)t1, (v16i8)t0);
                }

                result = __msa_cle_u_b((v16u8)s_a, amax);
                if (__builtin_msa_bz_v((v16u8)result)) {
                    if (alpha < LV_OPA_MAX) {
                        msa_st_bgra(d, s_b, s_g, s_r, s_a);
                    } else {
                        __msa_st_w((v4i32)s0, d, 0*16);
                        __msa_st_w((v4i32)s1, d, 1*16);
                        __msa_st_w((v4i32)s2, d, 2*16);
                        __msa_st_w((v4i32)s3, d, 3*16);
                    }

                    d += 16*4;
                    s += 16*4;
                    continue;
                }

                result =  __msa_clei_u_b((v16u8)s_a, LV_OPA_MIN);
                if (__builtin_msa_bnz_b((v16u8)result)) {
                    d += 16*4;
                    s += 16*4;
                    continue;
                }
            }

            msa_ld_bgra(d, d0,d1,d2,d3, d_b, d_g, d_r, d_a);

            result =  __msa_clei_u_b((v16u8)d_a, LV_OPA_MIN);
            if (__builtin_msa_bnz_b((v16u8)result)) {
                if (alpha < LV_OPA_MAX) {
                    msa_st_bgra(d, s_b, s_g, s_r, s_a);
                } else {
                    __msa_st_w((v4i32)s0, d, 0*16);
                    __msa_st_w((v4i32)s1, d, 1*16);
                    __msa_st_w((v4i32)s2, d, 2*16);
                    __msa_st_w((v4i32)s3, d, 3*16);
                }

                d += 16*4;
                s += 16*4;
                continue;
            }

            result = __msa_cle_u_b((v16u8)d_a, amax);
            if (__builtin_msa_bz_v((v16u8)result)) {
                d_a = __msa_subv_b(aff, s_a);
                v8i16 sr0 = (v8i16)__msa_mulur_h((v16u8)s_r, (v16u8)s_a);
                v8i16 sr1 = (v8i16)__msa_mulul_h((v16u8)s_r, (v16u8)s_a);
                v8i16 sg0 = (v8i16)__msa_mulur_h((v16u8)s_g, (v16u8)s_a);
                v8i16 sg1 = (v8i16)__msa_mulul_h((v16u8)s_g, (v16u8)s_a);
                v8i16 sb0 = (v8i16)__msa_mulur_h((v16u8)s_b, (v16u8)s_a);
                v8i16 sb1 = (v8i16)__msa_mulul_h((v16u8)s_b, (v16u8)s_a);

                v8i16 dr0 = (v8i16)__msa_mulur_h((v16u8)d_r, (v16u8)d_a);
                v8i16 dr1 = (v8i16)__msa_mulul_h((v16u8)d_r, (v16u8)d_a);
                v8i16 dg0 = (v8i16)__msa_mulur_h((v16u8)d_g, (v16u8)d_a);
                v8i16 dg1 = (v8i16)__msa_mulul_h((v16u8)d_g, (v16u8)d_a);
                v8i16 db0 = (v8i16)__msa_mulur_h((v16u8)d_b, (v16u8)d_a);
                v8i16 db1 = (v8i16)__msa_mulul_h((v16u8)d_b, (v16u8)d_a);

                v8i16 r0 = __msa_addv_h(sr0, dr0);
                v8i16 r1 = __msa_addv_h(sr1, dr1);
                v8i16 g0 = __msa_addv_h(sg0, dg0);
                v8i16 g1 = __msa_addv_h(sg1, dg1);
                v8i16 b0 = __msa_addv_h(sb0, db0);
                v8i16 b1 = __msa_addv_h(sb1, db1);

                r0 = (v8i16)__msa_pckod_b((v16i8)r1, (v16i8)r0);
                g0 = (v8i16)__msa_pckod_b((v16i8)g1, (v16i8)g0);
                b0 = (v8i16)__msa_pckod_b((v16i8)b1, (v16i8)b0);

                msa_st_bgra(d, b0, g0, r0, aff);

                d += 16*4;
                s += 16*4;
                continue;
            }

            v16i8 t0 = __msa_subv_b(aff, s_a);
            v8i16 t1 = (v8i16)__msa_mulur_h((v16u8)d_a, (v16u8)s_a);
            v8i16 t2 = (v8i16)__msa_mulul_h((v16u8)d_a, (v16u8)s_a);

            v8i16 t3 = (v8i16)__msa_addur_h((v16u8)d_a, (v16u8)s_a);
            v8i16 t4 = (v8i16)__msa_addul_h((v16u8)d_a, (v16u8)s_a);

            v8i16 t5 = (v8i16)__msa_mulur_h((v16u8)d_a, (v16u8)t0);
            v8i16 t6 = (v8i16)__msa_mulul_h((v16u8)d_a, (v16u8)t0);

            t1 = __msa_srli_h(t1, 8);
            t2 = __msa_srli_h(t2, 8);
            t3 = __msa_subv_h(t3, t1);
            t4 = __msa_subv_h(t4, t2);

            v8u16 ca0_s = __msa_min_u_h((v8u16)t3, a00ff);
            v8u16 ca1_s = __msa_min_u_h((v8u16)t4, a00ff);
            v8u16 ca0 = __msa_maxi_u_h((v8u16)ca0_s, 1);
            v8u16 ca1 = __msa_maxi_u_h((v8u16)ca1_s, 1);

            d_a = __msa_pckod_b((v16i8)t6, (v16i8)t5); // _da

            {
                v8i16 sr0 = (v8i16)__msa_mulur_h((v16u8)s_r, (v16u8)s_a);
                v8i16 sr1 = (v8i16)__msa_mulul_h((v16u8)s_r, (v16u8)s_a);
                v8i16 sg0 = (v8i16)__msa_mulur_h((v16u8)s_g, (v16u8)s_a);
                v8i16 sg1 = (v8i16)__msa_mulul_h((v16u8)s_g, (v16u8)s_a);
                v8i16 sb0 = (v8i16)__msa_mulur_h((v16u8)s_b, (v16u8)s_a);
                v8i16 sb1 = (v8i16)__msa_mulul_h((v16u8)s_b, (v16u8)s_a);

                v8i16 dr0 = (v8i16)__msa_mulur_h((v16u8)d_r, (v16u8)d_a);
                v8i16 dr1 = (v8i16)__msa_mulul_h((v16u8)d_r, (v16u8)d_a);
                v8i16 dg0 = (v8i16)__msa_mulur_h((v16u8)d_g, (v16u8)d_a);
                v8i16 dg1 = (v8i16)__msa_mulul_h((v16u8)d_g, (v16u8)d_a);
                v8i16 db0 = (v8i16)__msa_mulur_h((v16u8)d_b, (v16u8)d_a);
                v8i16 db1 = (v8i16)__msa_mulul_h((v16u8)d_b, (v16u8)d_a);

                v8i16 r0 = __msa_addv_h(sr0, dr0);
                v8i16 r1 = __msa_addv_h(sr1, dr1);
                v8i16 g0 = __msa_addv_h(sg0, dg0);
                v8i16 g1 = __msa_addv_h(sg1, dg1);
                v8i16 b0 = __msa_addv_h(sb0, db0);
                v8i16 b1 = __msa_addv_h(sb1, db1);

                r0 = (v8i16)__msa_div_u_h((v8u16)r0, (v8u16)ca0);
                r1 = (v8i16)__msa_div_u_h((v8u16)r1, (v8u16)ca1);
                g0 = (v8i16)__msa_div_u_h((v8u16)g0, (v8u16)ca0);
                g1 = (v8i16)__msa_div_u_h((v8u16)g1, (v8u16)ca1);
                b0 = (v8i16)__msa_div_u_h((v8u16)b0, (v8u16)ca0);
                b1 = (v8i16)__msa_div_u_h((v8u16)b1, (v8u16)ca1);

                ca0 = (v8u16)__msa_pckev_b((v16i8)ca1_s, (v16i8)ca0_s);
                r0 = (v8i16)__msa_pckev_b((v16i8)r1, (v16i8)r0);
                g0 = (v8i16)__msa_pckev_b((v16i8)g1, (v16i8)g0);
                b0 = (v8i16)__msa_pckev_b((v16i8)b1, (v16i8)b0);

                msa_st_bgra(d, b0, g0, r0, ca0);

                d += 16*4;
                s += 16*4;
            }
        }

        for (x = 0; x < w%16; x++) {
            uint8_t s_b, s_g, s_r, s_a;

            if (is_color) {
                s_b = ((uint8_t *)&color)[0];
                s_g = ((uint8_t *)&color)[1];
                s_r = ((uint8_t *)&color)[2];
                s_a = ((uint8_t *)&color)[3];
            } else {
                s_b = s[0];
                s_g = s[1];
                s_r = s[2];
                s_a = s_is_bgra ? s[3] : save_alpha;
            }

            blend_pixel_no_premulti(s_r, s_g, s_b, s_a, alpha, load_d, store_d);
            d += 4;
            s += 4;
        }

        d += d_delta;
        s += s_delta;
    }

}

static void blend_argb_mask_msa(uint8_t *d, int w, int h, int d_stride,
    uint8_t *s, int s_stride, uint8_t *mask, int m_stride, uint32_t color)
{
    int x, y;

    int s_delta = (s_stride-w)*4;
    int d_delta = (d_stride-w)*4;
    int m_delta = (m_stride-w);

    uint8_t alpha = color>>24;

    v16i8 aff = {0xff, 0xff, 0xff, 0xff,
                 0xff, 0xff, 0xff, 0xff,
                 0xff, 0xff, 0xff, 0xff,
                 0xff, 0xff, 0xff, 0xff};

    v16u8 amax = {LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1,
                  LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1,
                  LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1,
                  LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1, LV_OPA_MAX-1};

    v8u16 a00ff = {0x00ff, 0x00ff,
                   0x00ff, 0x00ff,
                   0x00ff, 0x00ff,
                   0x00ff, 0x00ff,};

    v16i8 s0, s1, s2, s3;
    v16i8 s_b, s_g, s_r, s_a, save_a;

    int is_color = !s;
    if (is_color) { // 编译器会说或许没有初始化
        v4i32 color_v = {color, color, color, color};
        s0 = (v16i8)color_v; s1 = (v16i8)color_v,
        s2 = (v16i8)color_v; s3 = (v16i8)color_v;
        msa_to_bgra(s0,s1,s2,s3, s_b, s_g, s_r, save_a);
    }

    for(y = 0; y < h; y++) {
        for(x = 0; x < 0; x++) {
            v16i8 d_b, d_g, d_r, d_a;
            v16i8 d0, d1, d2, d3;
            v16i8 result;

            v16u8 a = (v16u8)__msa_ld_b(mask, 0);
            mask += 16;

            if (!is_color) {
                msa_ld_bgra(s, s0,s1,s2,s3, s_b, s_g, s_r, s_a);

                result = __msa_clei_u_b((v16u8)s_a, LV_OPA_MIN);
                if (__builtin_msa_bnz_b((v16u8)result)) {
                    d += 16*4;
                    s += 16*4;
                    continue;
                }

                result = __msa_cle_u_b((v16u8)s_a, amax);
                if (__builtin_msa_bz_v((v16u8)result))
                    s_a = (v16i8)a;
                else {
                    v8u16 t0 = __msa_mulur_h((v16u8)s_a, a);
                    v8u16 t1 = __msa_mulul_h((v16u8)s_a, a);
                    s_a = __msa_pckod_b((v16i8)t1, (v16i8)t0);
                }
            } else {
                if (alpha < LV_OPA_MAX) {
                    v8u16 t0 = __msa_mulur_h((v16u8)save_a, a);
                    v8u16 t1 = __msa_mulul_h((v16u8)save_a, a);
                    s_a = __msa_pckod_b((v16i8)t1, (v16i8)t0);
                } else
                    s_a = (v16i8)a;
            }

            result = __msa_cle_u_b((v16u8)s_a, amax);
            if (__builtin_msa_bz_v((v16u8)result)) {
                msa_st_bgra(d, s_b, s_g, s_r, s_a);
                d += 16*4;
                s += 16*4;
                continue;
            }

            result = __msa_clei_u_b((v16u8)s_a, LV_OPA_MIN);
            if (__builtin_msa_bnz_b((v16u8)result)) {
                d += 16*4;
                s += 16*4;
                continue;
            }

            msa_ld_bgra(d, d0,d1,d2,d3, d_b, d_g, d_r, d_a);

            result =  __msa_clei_u_b((v16u8)d_a, LV_OPA_MIN);
            if (__builtin_msa_bnz_b((v16u8)result)) {
                msa_st_bgra(d, s_b, s_g, s_r, s_a);

                d += 16*4;
                s += 16*4;
                continue;
            }

            result = __msa_cle_u_b((v16u8)d_a, amax);
            if (__builtin_msa_bz_v((v16u8)result)) {
                d_a = __msa_subv_b(aff, s_a);
                v8i16 sr0 = (v8i16)__msa_mulur_h((v16u8)s_r, (v16u8)s_a);
                v8i16 sr1 = (v8i16)__msa_mulul_h((v16u8)s_r, (v16u8)s_a);
                v8i16 sg0 = (v8i16)__msa_mulur_h((v16u8)s_g, (v16u8)s_a);
                v8i16 sg1 = (v8i16)__msa_mulul_h((v16u8)s_g, (v16u8)s_a);
                v8i16 sb0 = (v8i16)__msa_mulur_h((v16u8)s_b, (v16u8)s_a);
                v8i16 sb1 = (v8i16)__msa_mulul_h((v16u8)s_b, (v16u8)s_a);

                v8i16 dr0 = (v8i16)__msa_mulur_h((v16u8)d_r, (v16u8)d_a);
                v8i16 dr1 = (v8i16)__msa_mulul_h((v16u8)d_r, (v16u8)d_a);
                v8i16 dg0 = (v8i16)__msa_mulur_h((v16u8)d_g, (v16u8)d_a);
                v8i16 dg1 = (v8i16)__msa_mulul_h((v16u8)d_g, (v16u8)d_a);
                v8i16 db0 = (v8i16)__msa_mulur_h((v16u8)d_b, (v16u8)d_a);
                v8i16 db1 = (v8i16)__msa_mulul_h((v16u8)d_b, (v16u8)d_a);

                v8i16 r0 = __msa_addv_h(sr0, dr0);
                v8i16 r1 = __msa_addv_h(sr1, dr1);
                v8i16 g0 = __msa_addv_h(sg0, dg0);
                v8i16 g1 = __msa_addv_h(sg1, dg1);
                v8i16 b0 = __msa_addv_h(sb0, db0);
                v8i16 b1 = __msa_addv_h(sb1, db1);

                r0 = (v8i16)__msa_pckod_b((v16i8)r1, (v16i8)r0);
                g0 = (v8i16)__msa_pckod_b((v16i8)g1, (v16i8)g0);
                b0 = (v8i16)__msa_pckod_b((v16i8)b1, (v16i8)b0);

                msa_st_bgra(d, b0, g0, r0, aff);

                d += 16*4;
                s += 16*4;
                continue;
            }

            v16i8 t0 = __msa_subv_b(aff, s_a);
            v8i16 t1 = (v8i16)__msa_mulur_h((v16u8)d_a, (v16u8)s_a);
            v8i16 t2 = (v8i16)__msa_mulul_h((v16u8)d_a, (v16u8)s_a);

            v8i16 t3 = (v8i16)__msa_addur_h((v16u8)d_a, (v16u8)s_a);
            v8i16 t4 = (v8i16)__msa_addul_h((v16u8)d_a, (v16u8)s_a);

            v8i16 t5 = (v8i16)__msa_mulur_h((v16u8)d_a, (v16u8)t0);
            v8i16 t6 = (v8i16)__msa_mulul_h((v16u8)d_a, (v16u8)t0);

            t1 = __msa_srli_h(t1, 8);
            t2 = __msa_srli_h(t2, 8);
            t3 = __msa_subv_h(t3, t1);
            t4 = __msa_subv_h(t4, t2);

            v8u16 ca0_s = __msa_min_u_h((v8u16)t3, a00ff);
            v8u16 ca1_s = __msa_min_u_h((v8u16)t4, a00ff);
            v8u16 ca0 = __msa_maxi_u_h((v8u16)ca0_s, 1);
            v8u16 ca1 = __msa_maxi_u_h((v8u16)ca1_s, 1);

            d_a = __msa_pckod_b((v16i8)t6, (v16i8)t5); // _da

            {
                v8i16 sr0 = (v8i16)__msa_mulur_h((v16u8)s_r, (v16u8)s_a);
                v8i16 sr1 = (v8i16)__msa_mulul_h((v16u8)s_r, (v16u8)s_a);
                v8i16 sg0 = (v8i16)__msa_mulur_h((v16u8)s_g, (v16u8)s_a);
                v8i16 sg1 = (v8i16)__msa_mulul_h((v16u8)s_g, (v16u8)s_a);
                v8i16 sb0 = (v8i16)__msa_mulur_h((v16u8)s_b, (v16u8)s_a);
                v8i16 sb1 = (v8i16)__msa_mulul_h((v16u8)s_b, (v16u8)s_a);

                v8i16 dr0 = (v8i16)__msa_mulur_h((v16u8)d_r, (v16u8)d_a);
                v8i16 dr1 = (v8i16)__msa_mulul_h((v16u8)d_r, (v16u8)d_a);
                v8i16 dg0 = (v8i16)__msa_mulur_h((v16u8)d_g, (v16u8)d_a);
                v8i16 dg1 = (v8i16)__msa_mulul_h((v16u8)d_g, (v16u8)d_a);
                v8i16 db0 = (v8i16)__msa_mulur_h((v16u8)d_b, (v16u8)d_a);
                v8i16 db1 = (v8i16)__msa_mulul_h((v16u8)d_b, (v16u8)d_a);

                v8i16 r0 = __msa_addv_h(sr0, dr0);
                v8i16 r1 = __msa_addv_h(sr1, dr1);
                v8i16 g0 = __msa_addv_h(sg0, dg0);
                v8i16 g1 = __msa_addv_h(sg1, dg1);
                v8i16 b0 = __msa_addv_h(sb0, db0);
                v8i16 b1 = __msa_addv_h(sb1, db1);

                r0 = (v8i16)__msa_div_u_h((v8u16)r0, (v8u16)ca0);
                r1 = (v8i16)__msa_div_u_h((v8u16)r1, (v8u16)ca1);
                g0 = (v8i16)__msa_div_u_h((v8u16)g0, (v8u16)ca0);
                g1 = (v8i16)__msa_div_u_h((v8u16)g1, (v8u16)ca1);
                b0 = (v8i16)__msa_div_u_h((v8u16)b0, (v8u16)ca0);
                b1 = (v8i16)__msa_div_u_h((v8u16)b1, (v8u16)ca1);

                ca0 = (v8u16)__msa_pckev_b((v16i8)ca1_s, (v16i8)ca0_s);
                r0 = (v8i16)__msa_pckev_b((v16i8)r1, (v16i8)r0);
                g0 = (v8i16)__msa_pckev_b((v16i8)g1, (v16i8)g0);
                b0 = (v8i16)__msa_pckev_b((v16i8)b1, (v16i8)b0);

                msa_st_bgra(d, b0, g0, r0, ca0);

                d += 16*4;
                s += 16*4;
            }
        }

        for (x = 0; x < w; x++) {
            uint8_t s_b, s_g, s_r, s_a;

            if (is_color) {
                s_b = ((uint8_t *)&color)[0];
                s_g = ((uint8_t *)&color)[1];
                s_r = ((uint8_t *)&color)[2];
                s_a = ((uint8_t *)&color)[3];
            } else {
                s_b = s[0];
                s_g = s[1];
                s_r = s[2];
                s_a = s[3];
            }

            blend_pixel_no_premulti(s_r, s_g, s_b, s_a, mask[0], load_d, store_d);
            d += 4;
            s += 4;
            mask += 1;
        }

        d += d_delta;
        s += s_delta;
        mask += m_delta;
    }
}

#endif

static void blend_argb_full_c(uint8_t *d, int w, int h, int d_stride,
     uint8_t *s, int s_stride, int alpha, int s_is_bgra, uint32_t color)
{
    int x, y;

    int s_delta = (s_stride-w)*4;
    int d_delta = (d_stride-w)*4;

    int is_color = !s;

    if (is_color && alpha <= LV_OPA_MIN)
            return;

    int save_alpha = alpha;
    if (!s_is_bgra)
        alpha = 0xff;
    uint8_t s_b, s_g, s_r, s_a;
    // if (is_color) { // 编译器会说或许没有初始化
        s_b = ((uint8_t *)&color)[0];
        s_g = ((uint8_t *)&color)[1];
        s_r = ((uint8_t *)&color)[2];
        s_a = ((uint8_t *)&color)[3];
    // }

    for(y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (!is_color) {
                s_b = s[0];
                s_g = s[1];
                s_r = s[2];
                s_a = s_is_bgra ? s[3] : save_alpha;
            }

            blend_pixel_no_premulti(s_r, s_g, s_b, s_a, alpha, load_d, store_d);
            d += 4;
            s += 4;
        }

        d += d_delta;
        s += s_delta;
    }
}

static void blend_argb_mask_c(uint8_t *d, int w, int h, int d_stride,
    uint8_t *s, int s_stride, uint8_t *mask, int m_stride, uint32_t color)
{
    int x, y;

    int s_delta = (s_stride-w)*4;
    int d_delta = (d_stride-w)*4;
    int m_delta = (m_stride-w);

    int is_color = !s;
    uint8_t s_b, s_g, s_r, s_a;

    // if (is_color) { // 编译器会说或许没有初始化
        s_b = ((uint8_t *)&color)[0];
        s_g = ((uint8_t *)&color)[1];
        s_r = ((uint8_t *)&color)[2];
        s_a = ((uint8_t *)&color)[3];
    // }

    for(y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (!is_color) {
                s_b = s[0];
                s_g = s[1];
                s_r = s[2];
                s_a = s[3];
            }

            blend_pixel_no_premulti(s_r, s_g, s_b, s_a, mask[0], load_d, store_d);
            d += 4;
            s += 4;
            mask += 1;
        }

        d += d_delta;
        s += s_delta;
        mask += m_delta;
    }
}

static void fill_color_argb(uint8_t *d_, int w, int h, int stride, uint32_t color)
{
    uint32_t *d = (void *)d_;
    stride = stride - (w-w%8);
    int x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w/8; x++) {
            d[0] = color;
            d[1] = color;
            d[2] = color;
            d[3] = color;
            d[4] = color;
            d[5] = color;
            d[6] = color;
            d[7] = color;
            d += 8;
        }
        for (x = 0; x < w%8; x++)
            d[x] = color;
        d += stride;
    }
}

static void fill_src_argb(uint8_t *d_, int w, int h, int d_stride, uint8_t *s_, int s_stride, uint32_t color)
{
    uint32_t *d = (void *)d_;
    uint32_t *s = (void *)s_;

    d_stride = d_stride - (w-w%8);
    s_stride = s_stride - (w-w%8);
    int x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w/8; x++) {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            d[3] = s[3];
            d[4] = s[4];
            d[5] = s[5];
            d[6] = s[6];
            d[7] = s[7];
            d += 8;
            s += 8;
        }
        for (x = 0; x < w%8; x++)
            d[x] = color;
        d += d_stride;
        s += s_stride;
    }
}

#ifdef __mips_msa
#define blend_argb_full_func blend_argb_full_msa
#define blend_argb_mask_func blend_argb_mask_msa
#else
#define blend_argb_full_func blend_argb_full_c
#define blend_argb_mask_func blend_argb_mask_c
#endif

ret_t g2d_blend_image(bitmap_t* fb, bitmap_t* img, const rect_t* dst, const rect_t* src,
                      uint8_t global_alpha)
{
    // return RET_FAIL;
    uint8_t *src_data = NULL;
    uint8_t *dst_data = NULL;

    int src_stride = bitmap_get_physical_line_length(img)/4;
    int dst_stride = bitmap_get_physical_line_length(fb)/4;

    return_value_if_fail(img != NULL && img->buffer != NULL && src != NULL, RET_BAD_PARAMS);
    return_value_if_fail(fb != NULL && fb->buffer != NULL && dst != NULL, RET_BAD_PARAMS);
    return_value_if_fail(img->format == BITMAP_FMT_BGRA8888, RET_BAD_PARAMS);
    return_value_if_fail(fb->format == BITMAP_FMT_BGRA8888, RET_BAD_PARAMS);
    if (src->w != dst->w || src->h != dst->h)   return RET_BAD_PARAMS; // not support scale

    src_data = bitmap_lock_buffer_for_write(img);
    dst_data = bitmap_lock_buffer_for_write(fb);
    return_value_if_fail(src_data != NULL && dst_data != NULL, RET_BAD_PARAMS);

    src_data = src_data + (src_stride*src->y + src->x)*4;
    dst_data = dst_data + (dst_stride*dst->y + dst->x)*4;
    blend_argb_full_func(dst_data, dst->w, dst->h, dst_stride,
                         src_data, src_stride, global_alpha, 1, 0);

    bitmap_unlock_buffer(img);
    bitmap_unlock_buffer(fb);

    return RET_OK;
}

ret_t g2d_fill_rect(bitmap_t* fb, const rect_t* dst, color_t c)
{
    // return RET_FAIL;
    uint8_t *dst_data = NULL;
    int dst_stride = bitmap_get_physical_line_length(fb) / 4;

    return_value_if_fail(fb != NULL && fb->buffer != NULL && dst != NULL, RET_BAD_PARAMS);
    return_value_if_fail(fb->format == BITMAP_FMT_BGRA8888, RET_BAD_PARAMS);

    dst_data = bitmap_lock_buffer_for_write(fb);
    return_value_if_fail(dst_data != NULL, RET_BAD_PARAMS);

    int bgra_color = (c.rgba.a << 24) | (c.rgba.r << 16) | (c.rgba.g << 8) | (c.rgba.b << 0);
    int global_alpha = 0xff;

    dst_data = dst_data + (dst_stride*dst->y + dst->x)*4;
    blend_argb_full_func(dst_data, dst->w, dst->h, dst_stride,
                         NULL, dst_stride, global_alpha, 1, bgra_color);

    bitmap_unlock_buffer(fb);

    return RET_OK;
}

#endif
