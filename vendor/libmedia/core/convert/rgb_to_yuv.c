#include <stdint.h>
#include <libmedia/rgb_to_yuv.h>

void pixel_rgb_to_yuv(int rgb, unsigned char *y, unsigned char *u, unsigned char *v)
{
    unsigned char *p = (void *)&rgb;
    unsigned char r = p[2];
    unsigned char g = p[1];
    unsigned char b = p[0];

    *y = to_y(r, g, b);
    *u = to_u(r, g, b);
    *v = to_v(r, g, b);
}

static void c_convert_bgr_to_nv12(void *src, int src_linesize, void *Y, int Y_linesize, void *UV, int UV_linesize, int width, int height)
{
    int i, j;

    for (i = 0; i < height; i+=2) {
        unsigned char *p0 = src + i*src_linesize;
        unsigned char *p1 = p0 + src_linesize;

        unsigned char *Y0 = Y + i*Y_linesize;
        unsigned char *Y1 = Y0 + Y_linesize;
        unsigned char *uv = UV + i*UV_linesize/2;

        for (j = 0; j < width; j += 2) {
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

            Y0[0] = y00;
            Y0[1] = y01;

            Y1[0] = y10;
            Y1[1] = y11;

            uv[0] = u00;
            uv[1] = v00;

            p0 += 8;
            p1 += 8;
            uv += 2;
            Y0 += 2;
            Y1 += 2;
        }
    }
}

static void c_convert_bgra_to_yuva420p(void *src, int src_linesize, void *Y, int Y_linesize,
                             void *U, int U_linesize, void *V, int V_linesize,
                             void *alpha, int alpha_linesize, int width, int height)
{
    int i, j;

    for (i = 0; i < height; i+=2) {
        unsigned char *p0 = src + i*src_linesize;
        unsigned char *p1 = p0 + src_linesize;

        unsigned char *Y0 = Y + i*Y_linesize;
        unsigned char *Y1 = Y0 + Y_linesize;
        unsigned char *U0 = U + i * U_linesize/2;
        unsigned char *V0 = V + i * V_linesize/2;

        unsigned char *A0 = alpha + i * alpha_linesize;
        unsigned char *A1 = A0 + alpha_linesize;


        for (j = 0; j < width; j += 2) {
            unsigned char a00 = p0[3];
            unsigned char r00 = p0[2];
            unsigned char g00 = p0[1];
            unsigned char b00 = p0[0];

            unsigned char a01 = p0[3+4];
            unsigned char r01 = p0[2+4];
            unsigned char g01 = p0[1+4];
            unsigned char b01 = p0[0+4];

            unsigned char a10 = p1[3];
            unsigned char r10 = p1[2];
            unsigned char g10 = p1[1];
            unsigned char b10 = p1[0];

            unsigned char a11 = p1[3+4];
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

            Y0[0] = y00;
            Y0[1] = y01;

            Y1[0] = y10;
            Y1[1] = y11;

            U0[0] = u00;
            V0[0] = v00;

            A0[0] = a00;
            A0[1] = a01;

            A1[0] = a10;
            A1[1] = a11;

            p0 += 8;
            p1 += 8;
            U0 += 1;
            V0 += 1;
            Y0 += 2;
            Y1 += 2;
            A0 += 2;
            A1 += 2;
        }
    }
}

#if defined(__mips_msa)

#include <msa.h>
#include "msa_convert.c"

v16i8 y_r_mut = {66,66,66,66,66,66,66,66,\
                 66,66,66,66,66,66,66,66};
v16i8 y_g_mut = {129,129,129,129,129,129,129,129,\
                 129,129,129,129,129,129,129,129};
v16i8 y_b_mut = {25,25,25,25,25,25,25,25,\
                 25,25,25,25,25,25,25,25};

v8i16 u_r_mut = {38,38,38,38,38,38,38,38};
v8i16 u_g_mut = {74,74,74,74,74,74,74,74};
v8i16 u_b_mut = {112,112,112,112,112,112,112,112};

v8i16 v_r_mut = {112,112,112,112,112,112,112,112};
v8i16 v_g_mut = {94,94,94,94,94,94,94,94};
v8i16 v_b_mut = {18,18,18,18,18,18,18,18};

v16i8 uv_add = {128,128,128,128,128,128,128,128,\
                128,128,128,128,128,128,128,128};

static void msa_convert_bgr_to_nv12(void *src, int src_linesize, void *Y, int Y_linesize,
                             void *UV, int UV_linesize, int width, int height)
{
    int i, j;
    for (i = 0; i < height; i+=2) {
        unsigned char *p0 = src + i*src_linesize;
        unsigned char *p1 = p0 + src_linesize;

        unsigned char *Y0 = Y + i*Y_linesize;
        unsigned char *Y1 = Y0 + Y_linesize;

        unsigned char *uv = UV + i*UV_linesize/2;

        for (j = 0; j < width; j += 16) {
            v16i8 b0, b1;
            v16i8 g0, g1;
            v16i8 r0, r1;

            msa_ld_bgr(p0, b0, g0, r0);
            msa_ld_bgr(p1, b1, g1, r1);

            v16i8 st_data0;
            v16i8 st_data1;

            msa_to_y(b0, g0, r0, st_data0);
            msa_to_y(b1, g1, r1, st_data1);

            __msa_st_b(st_data0, Y0, 0);
            __msa_st_b(st_data1, Y1, 0);

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

            msa_to_uv(bh0, gh0, rh0, st_data0);
            __msa_st_b(st_data0, uv, 0);

            p0 += 64;
            p1 += 64;
            Y0 += 16;
            Y1 += 16;
            uv += 16;
        }

        for(j = 0;j < width % 16; j += 2) {
            unsigned char n_r00 = p0[2];
            unsigned char n_g00 = p0[1];
            unsigned char n_b00 = p0[0];
            unsigned char n_r01 = p0[2+4];
            unsigned char n_g01 = p0[1+4];
            unsigned char n_b01 = p0[0+4];

            unsigned char n_r10 = p1[2];
            unsigned char n_g10 = p1[1];
            unsigned char n_b10 = p1[0];
            unsigned char n_r11 = p1[2+4];
            unsigned char n_g11 = p1[1+4];
            unsigned char n_b11 = p1[0+4];

            unsigned char y00 = to_y(n_r00, n_g00, n_b00);
            unsigned char y01 = to_y(n_r01, n_g01, n_b01);
            unsigned char y10 = to_y(n_r10, n_g10, n_b10);
            unsigned char y11 = to_y(n_r11, n_g11, n_b11);

            n_r00 = ((unsigned int) n_r00 + n_r01 + n_r10 + n_r11)/4;
            n_g00 = ((unsigned int) n_g00 + n_g01 + n_g10 + n_g11)/4;
            n_b00 = ((unsigned int) n_b00 + n_b01 + n_b10 + n_b11)/4;

            unsigned char u00 = to_u(n_r00, n_g00, n_b00);
            unsigned char v00 = to_v(n_r00, n_g00, n_b00);

            Y0[0] = y00;
            Y0[1] = y01;

            Y1[0] = y10;
            Y1[1] = y11;

            uv[0] = u00;
            uv[1] = v00;

            p0 += 8;
            p1 += 8;
            uv += 2;
            Y0 += 2;
            Y1 += 2;
        }
    }

}

static void msa_convert_bgra_to_yuva420p(void *src, int src_linesize, void *Y, int Y_linesize,
                             void *U, int U_linesize, void *V, int V_linesize,
                             void *alpha, int alpha_linesize, int width, int height)
{
    int i, j;

    for (i = 0; i < height; i+=2) {
        unsigned char *p0 = src + i*src_linesize;
        unsigned char *p1 = p0 + src_linesize;

        unsigned char *Y0 = Y + i*Y_linesize;
        unsigned char *Y1 = Y0 + Y_linesize;
        unsigned char *U0 = U + i * U_linesize/2;
        unsigned char *V0 = V + i * V_linesize/2;

        unsigned char *A0 = alpha + i * alpha_linesize;
        unsigned char *A1 = A0 + alpha_linesize;


        for (j = 0; j < width; j += 16) {
            v16i8 b0, b1;
            v16i8 g0, g1;
            v16i8 r0, r1;
            v16i8 a0, a1;

            msa_ld_bgra(p0, b0, g0, r0, a0);
            msa_ld_bgra(p1, b1, g1, r1, a1);

            v16i8 st_data0;
            v16i8 st_data1;

            msa_to_y(b0, g0, r0, st_data0);
            msa_to_y(b1, g1, r1, st_data1);

            __msa_st_b(st_data0, Y0, 0);
            __msa_st_b(st_data1, Y1, 0);
            __msa_st_b(a0, A0, 0);
            __msa_st_b(a1, A1, 0);

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

            msa_to_u(bh0, gh0, rh0, st_data0);
            msa_to_v(bh0, gh0, rh0, st_data1);

            st_data0 = __msa_pckod_b(st_data1, st_data0);

            __msa_stext_d((v2i64)st_data0, 0, U0, 0);
            __msa_stext_d((v2i64)st_data0, 1, V0, 0);

            p0 += 64;
            p1 += 64;
            U0 += 8;
            V0 += 8;
            Y0 += 16;
            Y1 += 16;
            A0 += 16;
            A1 += 16;
        }

        for(j = 0;j < width % 16; j += 2) {
            unsigned char a00 = p0[3];
            unsigned char r00 = p0[2];
            unsigned char g00 = p0[1];
            unsigned char b00 = p0[0];

            unsigned char a01 = p0[3+4];
            unsigned char r01 = p0[2+4];
            unsigned char g01 = p0[1+4];
            unsigned char b01 = p0[0+4];

            unsigned char a10 = p1[3];
            unsigned char r10 = p1[2];
            unsigned char g10 = p1[1];
            unsigned char b10 = p1[0];

            unsigned char a11 = p1[3+4];
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

            Y0[0] = y00;
            Y0[1] = y01;

            Y1[0] = y10;
            Y1[1] = y11;

            U0[0] = u00;
            V0[0] = v00;

            A0[0] = a00;
            A0[1] = a01;

            A1[0] = a10;
            A1[1] = a11;

            p0 += 8;
            p1 += 8;
            U0 += 1;
            V0 += 1;
            Y0 += 2;
            Y1 += 2;
            A0 += 2;
            A1 += 2;
        }
    }
}
#endif

void convert_bgra_to_yuva420p(void *src, int src_linesize, void *Y, int Y_linesize,
                             void *U, int U_linesize, void *V, int V_linesize,
                             void *alpha, int alpha_linesize, int width, int height)
{
#if defined(__mips_msa)
    msa_convert_bgra_to_yuva420p(src, src_linesize, Y, Y_linesize,
                                 U, U_linesize, V, V_linesize,
                                 alpha, alpha_linesize, width, height);

    return;
#endif
    c_convert_bgra_to_yuva420p(src, src_linesize, Y, Y_linesize,
                             U, U_linesize, V, V_linesize,
                             alpha, alpha_linesize, width, height);

}


void convert_bgr_to_nv12(void *src, int src_linesize, void *Y, int Y_linesize,
                         void *UV, int UV_linesize, int width, int height)
{
#if defined(__mips_msa)
    msa_convert_bgr_to_nv12(src, src_linesize, Y, Y_linesize,
                            UV, UV_linesize, width, height);

    return;
#endif

    c_convert_bgr_to_nv12(src, src_linesize, Y, Y_linesize,
                            UV, UV_linesize, width, height);

}