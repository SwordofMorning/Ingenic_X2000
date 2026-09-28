#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#ifndef ROTATE_BITS
#define ROTATE_BITS 32
#endif

#ifndef ROTATE_CLEAR_COLOR
#define ROTATE_CLEAR_COLOR 0x0
#endif

#ifndef USE_PIX_BILINEAR
#define rotate_def(name, type)  name##_##type
#else
#define rotate_def(name, type)  name##_##type##_bilinear
#endif

#if ROTATE_BITS == 8
#define rotate_base_type uint8_t
#define rotate_func_func_t rotate_def(rotate_func_func_t, uint8)
#define rotate_func_pix rotate_def(rotate_func_pix, uint8)
#define rotate_func_clear_line rotate_def(rotate_func_clear_line, uint8)
#define rotate_func_clear_rect rotate_def(rotate_func_clear_rect, uint8)
#define rotate_func_clear rotate_def(rotate_func_clear, uint8)
#define rotate_func_v0 rotate_def(rotate_func_v0, uint8)
#define rotate_func rotate_def(rotate_func, uint8)
#define rotate_func_funcs rotate_def(rotate_func_funcs, uint8)
#define rotate_func_border_pix rotate_def(rotate_func_border_pix, uint8)
#define rotate_func_border rotate_def(rotate_func_border, uint8)
#define rotate_func_v1 rotate_def(rotate_func_v1, uint8)
#define rotate_func_v2 rotate_def(rotate_func_v2, uint8)
#define rotate_func_v3 rotate_def(rotate_func_v3, uint8)
#define rotate_func_v4 rotate_def(rotate_func_v4, uint8)
#endif

#if ROTATE_BITS == 16
#define rotate_base_type uint16_t
#define rotate_func_func_t rotate_def(rotate_func_func_t, uint16)
#define rotate_func_pix rotate_def(rotate_func_pix, uint16)
#define rotate_func_clear_line rotate_def(rotate_func_clear_line, uint16)
#define rotate_func_clear_rect rotate_def(rotate_func_clear_rect, uint16)
#define rotate_func_clear rotate_def(rotate_func_clear, uint16)
#define rotate_func rotate_def(rotate_func, uint16)
#define rotate_func_funcs rotate_def(rotate_func_funcs, uint16)
#define rotate_func_border_pix rotate_def(rotate_func_border_pix, uint16)
#define rotate_func_border rotate_def(rotate_func_border, uint16)
#define rotate_func_v0 rotate_def(rotate_func_v0, uint16)
#define rotate_func_v1 rotate_def(rotate_func_v1, uint16)
#define rotate_func_v2 rotate_def(rotate_func_v2, uint16)
#define rotate_func_v3 rotate_def(rotate_func_v3, uint16)
#define rotate_func_v4 rotate_def(rotate_func_v4, uint16)
#endif

#if ROTATE_BITS == 24
#define rotate_base_type uint32_t
#define rotate_func_func_t rotate_def(rotate_func_func_t, uint24)
#define rotate_func_pix rotate_def(rotate_func_pix, uint24)
#define rotate_func_clear_line rotate_def(rotate_func_clear_line, uint24)
#define rotate_func_clear_rect rotate_def(rotate_func_clear_rect, uint24)
#define rotate_func_clear rotate_def(rotate_func_clear, uint24)
#define rotate_func rotate_def(rotate_func, uint24)
#define rotate_func_funcs rotate_def(rotate_func_funcs, uint24)
#define rotate_func_border_pix rotate_def(rotate_func_border_pix, uint24)
#define rotate_func_border rotate_def(rotate_func_border, uint24)
#define rotate_func_v0 rotate_def(rotate_func_v0, uint24)
#define rotate_func_v1 rotate_def(rotate_func_v1, uint24)
#define rotate_func_v2 rotate_def(rotate_func_v2, uint24)
#define rotate_func_v3 rotate_def(rotate_func_v3, uint24)
#define rotate_func_v4 rotate_def(rotate_func_v4, uint24)
#endif

#if ROTATE_BITS == 32
#define rotate_base_type uint32_t
#define rotate_func_func_t rotate_def(rotate_func_func_t, uint32)
#define rotate_func_pix rotate_def(rotate_func_pix, uint32)
#define rotate_func_clear_line rotate_def(rotate_func_clear_line, uint32)
#define rotate_func_clear_rect rotate_def(rotate_func_clear_rect, uint32)
#define rotate_func_clear rotate_def(rotate_func_clear, uint32)
#define rotate_func rotate_def(rotate_func, uint32)
#define rotate_func_funcs rotate_def(rotate_func_funcs, uint32)
#define rotate_func_border_pix rotate_def(rotate_func_border_pix, uint32)
#define rotate_func_border rotate_def(rotate_func_border, uint32)
#define rotate_func_v0 rotate_def(rotate_func_v0, uint32)
#define rotate_func_v1 rotate_def(rotate_func_v1, uint32)
#define rotate_func_v2 rotate_def(rotate_func_v2, uint32)
#define rotate_func_v3 rotate_def(rotate_func_v3, uint32)
#define rotate_func_v4 rotate_def(rotate_func_v4, uint32)
#endif

#ifndef FAST_DEN_NUM
#define FAST_DEN_NUM 1024
#endif

#ifndef ROTATE_MACRO_DEFINED
#define ROTATE_MACRO_DEFINED

typedef int (*rotate_get_fast_version_t)(int angle);

// floor: 返回最大的 且 小于当前值的整数 (地板)
// floor(0.5)=0  floor(1.5)=1 floor(-0.5)=-1 floor(-1.5)=-2

#define FAST_floor(v) ((v)&~(FAST_DEN_NUM-1))

// ceil: 返回最小的 且 大于当前值的整数 (天花板)
// ceil(0.5)=1  ceil(1.5)=2 ceil(-0.5)=0 ceil(-1.5)=-1
#define FAST_ceil(v) (((v)+FAST_DEN_NUM-1)&~(FAST_DEN_NUM-1))

#define min(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define max(_a, _b) ((_a) > (_b) ? (_a) : (_b))

// 从 src 到 dst 的计算公式
static inline int to_rotate_x(int x, int y, int cos_a, int sin_a)
{
    return x*cos_a + y*sin_a;
}

static inline int to_rotate_y(int x, int y, int cos_a, int sin_a)
{
    return -x*sin_a + y*cos_a;
}

static void rotate_cal_dst_area(
    int s_w, int s_h, int s_x, int s_y,
    int d_w, int d_h, int d_x, int d_y,
    float angle, int *new_d_w, int *new_d_h, int *new_d_x, int *new_d_y)
{
    int cos_a = cos((360-angle)*M_PI/180)*FAST_DEN_NUM;
    int sin_a = sin((360-angle)*M_PI/180)*FAST_DEN_NUM;

    // p0,p1,p2,p3
    int tx0 = to_rotate_x(0-s_x, 0-s_y, cos_a, sin_a);
    int ty0 = to_rotate_y(0-s_x, 0-s_y, cos_a, sin_a);

    int tx1 = to_rotate_x(0-s_x, s_h-s_y, cos_a, sin_a);
    int ty1 = to_rotate_y(0-s_x, s_h-s_y, cos_a, sin_a);

    int tx2 = to_rotate_x(s_w-s_x, 0-s_y, cos_a, sin_a);
    int ty2 = to_rotate_y(s_w-s_x, 0-s_y, cos_a, sin_a);

    int tx3 = to_rotate_x(s_w-s_x, s_h-s_y, cos_a, sin_a);
    int ty3 = to_rotate_y(s_w-s_x, s_h-s_y, cos_a, sin_a);

    // 找出最小dx0,dy0,dx1,dy1
    int dx0 = FAST_floor(min(min(tx0, tx1), min(tx2, tx3)))/FAST_DEN_NUM;
    int dy0 = FAST_floor(min(min(ty0, ty1), min(ty2, ty3)))/FAST_DEN_NUM;
    int dx1 = FAST_ceil(max(max(tx0, tx1), max(tx2, tx3)))/FAST_DEN_NUM;
    int dy1 = FAST_ceil(max(max(ty0, ty1), max(ty2, ty3)))/FAST_DEN_NUM;

    dx0 = max(dx0-1+d_x, 0);
    dy0 = max(dy0-1+d_y, 0);

    dx1 = min(dx1+1+d_x, d_w);
    dy1 = min(dy1+1+d_y, d_h);

    if (new_d_w)
        *new_d_w = dx1-dx0;
    if (new_d_h)
        *new_d_h = dy1-dy0;
    if (new_d_x)
        *new_d_x = d_x - dx0;
    if (new_d_y)
        *new_d_y = d_y - dy0;
}

// 找到最大,最小的两点的index
static inline void find_min_max(int *v, int len, int *min_, int *max_)
{
    int i;
    int min = 0, max = 0;
    for (i = 1; i < len; i++) {
        if (v[i] < v[min])
            min = i;
        else if (v[i] >= v[max])
            max = i;
    }
    *min_ = min;
    *max_ = max;
}

#define get_rgb_16(v, b0,b1,b2) \
do { \
    b0=(uint16_t)(v)&0x1f; \
    b1=((uint16_t)(v)>>5)&0x3f; \
    b2=((uint16_t)(v)>>11)&0x1f; \
} while (0)

#define to_rgb_16(b0,b1,b2) \
  ({ \
    uint16_t v; \
    v = (uint16_t)(b0) << 0 | \
        (uint16_t)(b1) << 5 | \
        (uint16_t)(b2) << 11; \
    v;\
  })

#define get_bytes_32(v, b0,b1,b2,b3) \
do { \
    b0=(uint32_t)(v)&0xff; \
    b1=((uint32_t)(v)>>8)&0xff; \
    b2=((uint32_t)(v)>>16)&0xff; \
    b3=((uint32_t)(v)>>24)&0xff; \
} while (0)

#define to_uint32(b0,b1,b2,b3) \
  ({ \
    uint32_t v;\
    v = (uint32_t)(b0) << 0 | \
        (uint32_t)(b1) << 8 | \
        (uint32_t)(b2) << 16 | \
        (uint32_t)(b3) << 24; \
    v;\
  })

#define uint8_linear(p0,p1,v0,v1) \
({ \
    (v1*p0 + v0*p1)/FAST_DEN_NUM; \
})

#define uint8_bilinear(p0,p1,p2,p3,u0,u1,v0,v1) \
({ \
    (u1*v1*p0 + u0*v1*p1 + u1*v0*p2 + u0*v0*p3)/FAST_DEN_NUM/FAST_DEN_NUM; \
})

#define uint16_linear(p0,p1,v0,v1) \
({ \
    uint32_t p00,p01,p02; \
    uint32_t p10,p11,p12; \
 \
    get_rgb_16(p0,p00,p01,p02); \
    get_rgb_16(p1,p10,p11,p12); \
 \
    uint32_t r0 = (v1*p00 + v0*p10)/FAST_DEN_NUM; \
    uint32_t r1 = (v1*p01 + v0*p11)/FAST_DEN_NUM; \
    uint32_t r2 = (v1*p02 + v0*p12)/FAST_DEN_NUM; \
    to_rgb_16(r0,r1,r2); \
})

#define uint16_bilinear(p0,p1,p2,p3,u0,u1,v0,v1) \
({ \
    uint32_t p00,p01,p02; \
    uint32_t p10,p11,p12; \
    uint32_t p20,p21,p22; \
    uint32_t p30,p31,p32; \
\
    get_rgb_16(p0,p00,p01,p02); \
    get_rgb_16(p1,p10,p11,p12); \
    get_rgb_16(p2,p20,p21,p22); \
    get_rgb_16(p3,p30,p31,p32); \
    uint32_t r0 = (u1*v1*p00 + u0*v1*p10 + u1*v0*p20 + u0*v0*p30)/FAST_DEN_NUM/FAST_DEN_NUM; \
    uint32_t r1 = (u1*v1*p01 + u0*v1*p11 + u1*v0*p21 + u0*v0*p31)/FAST_DEN_NUM/FAST_DEN_NUM; \
    uint32_t r2 = (u1*v1*p02 + u0*v1*p12 + u1*v0*p22 + u0*v0*p32)/FAST_DEN_NUM/FAST_DEN_NUM; \
    to_rgb_16(r0,r1,r2); \
})

#define uint24_linear(p0,p1,v0,v1) \
({ \
    uint32_t p00,p01,p02,p03; \
    uint32_t p10,p11,p12,p13; \
 \
    get_bytes_32(p0,p00,p01,p02,p03); \
    get_bytes_32(p1,p10,p11,p12,p13); \
 \
    uint32_t r0 = (v1*p00 + v0*p10)/FAST_DEN_NUM; \
    uint32_t r1 = (v1*p01 + v0*p11)/FAST_DEN_NUM; \
    uint32_t r2 = (v1*p02 + v0*p12)/FAST_DEN_NUM; \
 \
    to_uint32(r0,r1,r2,p03); \
})

#define uint24_bilinear(p0,p1,p2,p3,u0,u1,v0,v1) \
({ \
    uint32_t p00,p01,p02,p03; \
    uint32_t p10,p11,p12,p13; \
    uint32_t p20,p21,p22,p23; \
    uint32_t p30,p31,p32,p33; \
 \
    get_bytes_32(p0,p00,p01,p02,p03); \
    get_bytes_32(p1,p10,p11,p12,p13); \
    get_bytes_32(p2,p20,p21,p22,p23); \
    get_bytes_32(p3,p30,p31,p32,p33); \
 \
    uint32_t r0 = (u1*v1*p00 + u0*v1*p10 + u1*v0*p20 + u0*v0*p30)/FAST_DEN_NUM/FAST_DEN_NUM; \
    uint32_t r1 = (u1*v1*p01 + u0*v1*p11 + u1*v0*p21 + u0*v0*p31)/FAST_DEN_NUM/FAST_DEN_NUM; \
    uint32_t r2 = (u1*v1*p02 + u0*v1*p12 + u1*v0*p22 + u0*v0*p32)/FAST_DEN_NUM/FAST_DEN_NUM; \
 \
    to_uint32(r0,r1,r2,p03); \
})

#define uint32_linear(p0,p1,v0,v1) \
({ \
    uint32_t p00,p01,p02,p03; \
    uint32_t p10,p11,p12,p13; \
 \
    get_bytes_32(p0,p00,p01,p02,p03); \
    get_bytes_32(p1,p10,p11,p12,p13); \
 \
    uint32_t r0 = (v1*p00 + v0*p10)/FAST_DEN_NUM; \
    uint32_t r1 = (v1*p01 + v0*p11)/FAST_DEN_NUM; \
    uint32_t r2 = (v1*p02 + v0*p12)/FAST_DEN_NUM; \
    uint32_t r3 = (v1*p03 + v0*p13)/FAST_DEN_NUM; \
 \
    to_uint32(r0,r1,r2,r3); \
})

#define uint32_bilinear(p0,p1,p2,p3,u0,u1,v0,v1) \
({ \
    uint32_t p00,p01,p02,p03; \
    uint32_t p10,p11,p12,p13; \
    uint32_t p20,p21,p22,p23; \
    uint32_t p30,p31,p32,p33; \
 \
    get_bytes_32(p0,p00,p01,p02,p03); \
    get_bytes_32(p1,p10,p11,p12,p13); \
    get_bytes_32(p2,p20,p21,p22,p23); \
    get_bytes_32(p3,p30,p31,p32,p33); \
 \
    uint32_t r0 = (u1*v1*p00 + u0*v1*p10 + u1*v0*p20 + u0*v0*p30)/FAST_DEN_NUM/FAST_DEN_NUM; \
    uint32_t r1 = (u1*v1*p01 + u0*v1*p11 + u1*v0*p21 + u0*v0*p31)/FAST_DEN_NUM/FAST_DEN_NUM; \
    uint32_t r2 = (u1*v1*p02 + u0*v1*p12 + u1*v0*p22 + u0*v0*p32)/FAST_DEN_NUM/FAST_DEN_NUM; \
    uint32_t r3 = (u1*v1*p03 + u0*v1*p13 + u1*v0*p23 + u0*v0*p33)/FAST_DEN_NUM/FAST_DEN_NUM; \
 \
    to_uint32(r0,r1,r2,r3); \
})

static inline void blend_pix32(uint32_t *d, uint32_t s)
{
    uint8_t s_a = (s >> 24)&0xff;
    if (s_a >= 0xf8) {
        d[0] = s;
        return;
    }
    if (s_a <= 0x08)
        return;

    uint8_t s_r, s_g, s_b;
    uint8_t d_a, d_r, d_g, d_b;

    get_bytes_32(s, s_b, s_g, s_r, s_a);
    get_bytes_32(d[0], d_b, d_g, d_r, d_a);

    uint8_t r, g, b;

    /* d_a < 0x08 可认为是 0x00, 可以简化计算公式
     */
    if (d_a < 0x08)
        d[0] = s;

    /* d_a > 0xf4 可认为是 0xff, 可以简化计算公式
     */
    if (d_a > 0xf4) {
        d_a = 0xff - s_a;
        r = (d_r * d_a + s_r * s_a) >> 8;
        g = (d_g * d_a + s_g * s_a) >> 8;
        b = (d_b * d_a + s_b * s_a) >> 8;
        d[0] = to_uint32(b, g, r, 0xff);
        return;
    }

    int out_a = s_a + d_a - ((s_a * d_a) >> 8);
    d_a = (d_a * (0xff - s_a)) >> 8;

    if (out_a > 255)
        out_a = 255;

    if (out_a < 0)
        out_a = 0;

    if (out_a) {
        r = (d_r * d_a + s_r * s_a) / out_a;
        g = (d_g * d_a + s_g * s_a) / out_a;
        b = (d_b * d_a + s_b * s_a) / out_a;
        d[0] = to_uint32(b, g, r, out_a);
    } else
        d[0] = to_uint32(0, 0, 0, 0);
}

#endif

#if ROTATE_BITS == 8
#define rotate_linear_(p0,p1,v0,v1) uint8_linear(p0,p1,v0,v1)
#elif ROTATE_BITS == 16
#define rotate_linear_(p0,p1,v0,v1) uint16_linear(p0,p1,v0,v1)
#elif ROTATE_BITS == 24
#define rotate_linear_(p0,p1,v0,v1) uint24_linear(p0,p1,v0,v1)
#elif ROTATE_BITS == 32
#define rotate_linear_(p0,p1,v0,v1) uint32_linear(p0,p1,v0,v1)
#endif

#if ROTATE_BITS == 8
#define rotate_bilinear_(p0,p1,p2,p3,u0,u1,v0,v1) uint8_bilinear(p0,p1,p2,p3,u0,u1,v0,v1)
#elif ROTATE_BITS == 16
#define rotate_bilinear_(p0,p1,p2,p3,u0,u1,v0,v1) uint16_bilinear(p0,p1,p2,p3,u0,u1,v0,v1)
#elif ROTATE_BITS == 24
#define rotate_bilinear_(p0,p1,p2,p3,u0,u1,v0,v1) uint24_bilinear(p0,p1,p2,p3,u0,u1,v0,v1)
#elif ROTATE_BITS == 32
#define rotate_bilinear_(p0,p1,p2,p3,u0,u1,v0,v1) uint32_bilinear(p0,p1,p2,p3,u0,u1,v0,v1)
#endif

#define rotate_linear(p0,p1,v0,v1) \
({ \
   (p0==p1) ? p0 : rotate_linear_(p0,p1,v0,v1); \
})

#define rotate_bilinear(p0,p1,p2,p3,u0,u1,v0,v1) \
({ \
    (p0==p1&&p2==p3&&p0==p2) ? p0 : rotate_bilinear_(p0,p1,p2,p3,u0,u1,v0,v1); \
})

#ifndef ROTATE_CLEAR_DST
#define get_bg_pix(d) ((d)[0])
#define set_bg_pix(d) do {} while(0)
#else
#define get_bg_pix(d) ROTATE_CLEAR_COLOR
#define set_bg_pix(d) do {(d)[0] = ROTATE_CLEAR_COLOR;} while(0)
#endif

#if ROTATE_BITS == 32
#define blend_pix(d, s)  blend_pix32(d, s)
#else
#define blend_pix(d, s)  ({d[0] = s;})
#endif

#ifdef USE_PIX_BILINEAR
#define rotate_pix(s,s_w,s_h,s_lw,d,x0,y0) rotate_func_pix(s,s_w,s_h,s_lw,d,x0,y0)
#else
#define rotate_pix(s,s_w,s_h,s_lw,d,x0,y0) blend_pix((d), (s)[((y0)/FAST_DEN_NUM)*s_lw+(x0)/FAST_DEN_NUM])
#endif

#ifdef USE_PIX_BILINEAR
static void rotate_func_pix(
    rotate_base_type *s, int s_w, int s_h, int s_lw,
    rotate_base_type *d,  int x0, int y0)
{
    int u0 = x0-FAST_floor(x0);
    int v0 = y0-FAST_floor(y0);
    int u1 = u0 ? FAST_DEN_NUM - u0 : FAST_DEN_NUM;
    int v1 = v0 ? FAST_DEN_NUM - v0 : FAST_DEN_NUM;

    rotate_base_type p0;
    rotate_base_type p1;
    rotate_base_type p2;
    rotate_base_type p3;

    x0 = FAST_floor(x0)/FAST_DEN_NUM;
    y0 = FAST_floor(y0)/FAST_DEN_NUM;

    int s0;
    if (u0 == 0) {
        if (v0 == 0) {
            p0 = s[y0*s_lw + x0];
            s0 = p0;
        } else {
            p0 = s[y0*s_lw + x0];
            p1 = s[(y0+1)*s_lw + x0];
            s0 = rotate_linear(p0,p1,v0,v1);
        }
    } else if (v0 == 0) {
        p0 = s[y0*s_lw + x0];
        p1 = s[y0*s_lw + x0+1];
        s0 = rotate_linear(p0,p1,u0,u1);
    } else {
        p0 = s[y0*s_lw + x0];
        p1 = s[y0*s_lw + x0+1];
        p2 = s[(y0+1)*s_lw + x0];
        p3 = s[(y0+1)*s_lw + x0+1];
        s0 = rotate_bilinear(p0,p1,p2,p3,u0,u1,v0,v1);
    }
    blend_pix(d, s0);
}
#endif


#if ROTATE_BITS == 32
static int rotate_func_border_pix(
    rotate_base_type *s, int s_w, int s_h, int s_lw,
    rotate_base_type *d,  int x0, int y0)
{
    rotate_base_type p0;
    rotate_base_type p1;
    rotate_base_type p2;
    rotate_base_type p3;

    int u0 = x0-FAST_floor(x0);
    int v0 = y0-FAST_floor(y0);
    int u1 = u0 ? FAST_DEN_NUM - u0 : FAST_DEN_NUM;
    int v1 = v0 ? FAST_DEN_NUM - v0 : FAST_DEN_NUM;

    x0 = FAST_floor(x0)/FAST_DEN_NUM;
    y0 = FAST_floor(y0)/FAST_DEN_NUM;

    uint8_t alpha;
    uint32_t s0;

    if (x0 == -1) {
        if (y0 == -1) {
            p3 = s[(y0+1)*s_lw + x0+1];
            alpha = (p3 >> 24)&0xff;
            if (alpha == 0)
                return 0;
            alpha = uint8_bilinear(0,0,0,alpha,u0,u1,v0,v1);
            s0 = (p3 & 0x00ffffff) | (alpha << 24);
        } else if (y0 == s_h-1) {
            p1 = s[y0*s_lw + x0+1];
            alpha = (p1 >> 24)&0xff;
            if (alpha == 0)
                return 0;
            alpha = uint8_bilinear(0,alpha,0,0,u0,u1,v0,v1);
            s0 = (p1 & 0x00ffffff) | (alpha << 24);
        } else {
            p1 = s[y0*s_lw + x0+1];
            p3 = s[(y0+1)*s_lw + x0+1];
            p1 = uint32_linear(p1, p3, v0, v1);
            alpha = (p1 >> 24)&0xff;
            if (alpha == 0)
                return 0;
            alpha = uint8_bilinear(0,alpha,0,alpha,u0,u1,v0,v1);
            s0 = (p1 & 0x00ffffff) | (alpha << 24);
        }
    } else if (x0 == s_w-1) {
        if (y0 == -1) {
            p2 = s[(y0+1)*s_lw + x0];
            alpha = (p2 >> 24)&0xff;
            if (alpha == 0)
                return 0;
            alpha = uint8_bilinear(0,0,alpha,0,u0,u1,v0,v1);
            s0 = (p2 & 0x00ffffff) | (alpha << 24);
        } else if (y0 == s_h-1) {
            p0 = s[y0*s_lw + x0];
            alpha = (p0 >> 24)&0xff;
            if (alpha == 0)
                return 0;
            alpha = uint8_bilinear(alpha,0,0,0,u0,u1,v0,v1);
            s0 = (p0 & 0x00ffffff) | (alpha << 24);
        } else {
            p0 = s[y0*s_lw + x0];
            p2 = s[(y0+1)*s_lw + x0];
            p0 = uint32_linear(p0, p2, v0, v1);
            alpha = (p0 >> 24)&0xff;
            if (alpha == 0)
                return 0;
            alpha = uint8_bilinear(alpha,0,alpha,0,u0,u1,v0,v1);
            s0 = (p0 & 0x00ffffff) | (alpha << 24);
        }
    } else if (y0 == -1) {
        p2 = s[(y0+1)*s_lw + x0];
        p3 = s[(y0+1)*s_lw + x0+1];
        p2 = uint32_linear(p2, p3, u0, u1);
        alpha = (p2 >> 24)&0xff;
        if (alpha == 0)
            return 0;
        alpha = uint8_bilinear(0,0,alpha,alpha,u0,u1,v0,v1);
        s0 = (p2 & 0x00ffffff) | (alpha << 24);
    } else if (y0 == s_h-1) {
        p0 = s[y0*s_lw + x0];
        p1 = s[y0*s_lw + x0+1];
        p0 = uint32_linear(p0, p1, u0, u1);
        alpha = (p0 >> 24)&0xff;
        if (alpha == 0)
            return 0;
        alpha = uint8_bilinear(alpha,alpha,0,0,u0,u1,v0,v1);
        s0 = (p0 & 0x00ffffff) | (alpha << 24);
    } else
        return -1;

    blend_pix32(d, s0);
    return 0;
}
#else
static int rotate_func_border_pix(
    rotate_base_type *s, int s_w, int s_h, int s_lw,
    rotate_base_type *d,  int x0, int y0)
{
    rotate_base_type p0;
    rotate_base_type p1;
    rotate_base_type p2;
    rotate_base_type p3;

    int u0 = x0-FAST_floor(x0);
    int v0 = y0-FAST_floor(y0);
    int u1 = u0 ? FAST_DEN_NUM - u0 : FAST_DEN_NUM;
    int v1 = v0 ? FAST_DEN_NUM - v0 : FAST_DEN_NUM;

    x0 = FAST_floor(x0)/FAST_DEN_NUM;
    y0 = FAST_floor(y0)/FAST_DEN_NUM;

    if (x0 == -1) {
        p0 = get_bg_pix(d);
        p2 = get_bg_pix(d);
        if (y0 == -1) {
            p1 = get_bg_pix(d);
            p3 = s[(y0+1)*s_lw + x0+1];
        } else if (y0 == s_h-1) {
            p1 = s[y0*s_lw + x0+1];
            p3 = get_bg_pix(d);
        } else {
            p1 = s[y0*s_lw + x0+1];
            p3 = s[(y0+1)*s_lw + x0+1];
        }
    } else if (x0 == s_w-1) {
        p1 = get_bg_pix(d);
        p3 = get_bg_pix(d);
        if (y0 == -1) {
            p0 = get_bg_pix(d);
            p2 = s[(y0+1)*s_lw + x0];
        } else if (y0 == s_h-1) {
            p0 = s[y0*s_lw + x0];
            p2 = get_bg_pix(d);
        } else {
            p0 = s[y0*s_lw + x0];
            p2 = s[(y0+1)*s_lw + x0];
        }
    } else if (y0 == -1) {

        p0 = get_bg_pix(d);
        p1 = get_bg_pix(d);
        p2 = s[(y0+1)*s_lw + x0];
        p3 = s[(y0+1)*s_lw + x0+1];
    } else if (y0 == s_h-1) {
        p0 = s[y0*s_lw + x0];
        p1 = s[y0*s_lw + x0+1];
        p2 = get_bg_pix(d);
        p3 = get_bg_pix(d);
    } else
        return -1;

    d[0] = rotate_bilinear(p0,p1,p2,p3,u0,u1,v0,v1);
    return 0;
}
#endif

#define rotate_border_pix(s,s_w,s_h,s_lw,d,x0,y0) rotate_func_border_pix(s,s_w,s_h,s_lw,d,x0,y0)

static void rotate_func_border(
    rotate_base_type *s, int s_w, int s_h, int s_lw, int s_x, int s_y,
    rotate_base_type *d, int d_w, int d_h, int d_lw, int d_x, int d_y,
    int cos_a, int sin_a, int tan_a, int cot_a)
{
    int x, y;
    int tx[4];
    int ty[4];

    // p0,p1,p2,p3
    tx[0] = to_rotate_x(0-s_x, 0-s_y, cos_a, sin_a);
    ty[0] = to_rotate_y(0-s_x, 0-s_y, cos_a, sin_a);

    tx[1] = to_rotate_x(0-s_x, s_h-s_y, cos_a, sin_a);
    ty[1] = to_rotate_y(0-s_x, s_h-s_y, cos_a, sin_a);

    tx[2] = to_rotate_x(s_w-s_x, 0-s_y, cos_a, sin_a);
    ty[2] = to_rotate_y(s_w-s_x, 0-s_y, cos_a, sin_a);

    tx[3] = to_rotate_x(s_w-s_x, s_h-s_y, cos_a, sin_a);
    ty[3] = to_rotate_y(s_w-s_x, s_h-s_y, cos_a, sin_a);

    int min_x, min_y, max_x, max_y;
    find_min_max(tx, 4, &min_x, &max_x);
    find_min_max(ty, 4, &min_y, &max_y);

    // dx0那一列上与菱形相交的点的p1.y
    // dx1那一列上与菱形相交的点的p2.y
    int dyl0 = ty[min_x]/FAST_DEN_NUM;
    int dyl1 = ty[max_x]/FAST_DEN_NUM;

    int dx0 = FAST_floor(tx[min_x])/FAST_DEN_NUM;
    int dy0 = FAST_floor(ty[min_y])/FAST_DEN_NUM;
    int dx1 = FAST_ceil(tx[max_x])/FAST_DEN_NUM;
    int dy1 = FAST_ceil(ty[max_y])/FAST_DEN_NUM;

    dx0 -= 1;
    dy0 -= 1;

    dx1 += 1;
    dy1 += 1;

    int fdx0 = max(dx0, 0-d_x);
    int fdy0 = max(dy0, 0-d_y);
    int fdx1 = min(dx1, d_w-d_x);
    int fdy1 = min(dy1, d_h-d_y);

    d += (fdy0+d_y)*d_lw + d_x;

    s_x = s_x * FAST_DEN_NUM;
    s_y = s_y * FAST_DEN_NUM;

    for (y = fdy0; y < fdy1; y++) {
        // 理论上菱形的有效区域就是 dx0+a,dx1-b 之间
        int a, b;
        a = y < dyl0 ? (dyl0-y)*tan_a : (y-dyl0)*cot_a;
        b = y < dyl1 ? (dyl1-y)*cot_a : (y-dyl1)*tan_a;

        a = FAST_floor(a)/FAST_DEN_NUM;
        b = FAST_floor(b)/FAST_DEN_NUM;

        int x_start = dx0+a;
        int x_end = dx1-b;

        // 接下来的一长片代码都是为了消除整数计算带来误差

        if (x_start < fdx0)
            x_start = fdx0;
        else if (x_start >= fdx1)
            x_start = fdx1-1;
        if (x_end < fdx0)
            x_end = fdx0;
        else if (x_end >= fdx1)
            x_end = fdx1-1;

        // 误差可能导致 x_start > x_end,所以纠正回起点/终点
        if (x_start > x_end) {
            x_start = fdx0;
            x_end = fdx1-1;
        }

        // 往起点搜寻x_start,看是否有边界点
        for (x = x_start; x >= fdx0; x--) {
            int x1 = (cos_a*x - sin_a*y + s_x);
            int y1 = (sin_a*x + cos_a*y + s_y);

            if (x1 <= -FAST_DEN_NUM || x1 >= s_w*FAST_DEN_NUM ||
                y1 <= -FAST_DEN_NUM || y1 >= s_h*FAST_DEN_NUM)
                break;

            rotate_border_pix(s, s_w, s_h, s_lw, d+x, x1, y1);
        }

        // 往终点搜寻x_start,看是否有边界点
        for (x = x_start+1; x < x_end; x++) {
            int x1 = (cos_a*x - sin_a*y + s_x);
            int y1 = (sin_a*x + cos_a*y + s_y);

            if (x1 <= -FAST_DEN_NUM || x1 >= s_w*FAST_DEN_NUM ||
                y1 <= -FAST_DEN_NUM || y1 >= s_h*FAST_DEN_NUM)
                continue;
            if (rotate_border_pix(s, s_w, s_h, s_lw, d+x, x1, y1))
                break;
        }
        x_start = x;

        // 往终点搜寻x_end,看是否有边界点
        for (x = x_end; x < fdx1; x++) {
            int x1 = (cos_a*x - sin_a*y + s_x);
            int y1 = (sin_a*x + cos_a*y + s_y);

            if (x1 <= -FAST_DEN_NUM || x1 >= s_w*FAST_DEN_NUM ||
                y1 <= -FAST_DEN_NUM || y1 >= s_h*FAST_DEN_NUM)
                break;
            rotate_border_pix(s, s_w, s_h, s_lw, d+x, x1, y1);
        }

        // 往起点搜寻x_end,看是否有边界点
        for (x = x_end-1; x > x_start; x--) {
            int x1 = (cos_a*x - sin_a*y + s_x);
            int y1 = (sin_a*x + cos_a*y + s_y);

            if (x1 <= -FAST_DEN_NUM || x1 >= s_w*FAST_DEN_NUM ||
                y1 <= -FAST_DEN_NUM || y1 >= s_h*FAST_DEN_NUM)
                continue;
            if (rotate_border_pix(s, s_w, s_h, s_lw, d+x, x1, y1))
                break;
        }
        d += d_lw;
    }
}

#ifdef ROTATE_CLEAR_DST
static void rotate_func_clear_line(rotate_base_type *d, int x0)
{
    int i;
    for (i = 0; i < x0; i++) {
        set_bg_pix(d+i);
    }
}

static void rotate_func_clear_rect(rotate_base_type *d, int w, int h, int lw)
{
    int i, j;
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            set_bg_pix(d+i);
        }
        d += lw;
    }
}

static void rotate_func_clear(
    rotate_base_type *d, int w, int h, int dx, int dy, int x0, int y0, int x1, int y1,  int lw)
{
    x0 += dx;
    y0 += dy;
    x1 += dx;
    y1 += dy;
    if (y0 > 0)
        rotate_func_clear_rect(d, w, y0, lw);
    if (x0 > 0)
        rotate_func_clear_rect(d, x0, h, lw);
    if (x1 < w)
        rotate_func_clear_rect(d+x1, w-x1, h, lw);
    if (y1 < h)
        rotate_func_clear_rect(d+y1*lw, w, h-y1, lw);
}
#else
static void rotate_func_clear_line(rotate_base_type *d, int x0)
{

}

static void rotate_func_clear_rect(rotate_base_type *d, int w, int h, int lw)
{

}

static void rotate_func_clear(
    rotate_base_type *d, int w, int h, int dx, int dy, int x0, int y0, int x1, int y1,  int lw)
{

}
#endif

/**
 * [rotate 算法 V0版本]
 *
 *  0 旋转的数学公式推导过程请百度,注释不方便画图.
 *  1 采用整数定点计算三角函数,定点值采用256,512,1024,,,2的n次方,将除法优化成右移(编译器自动)
 *  2 实际旋转过程由dst逆推src,从而赋值,这样不会漏掉dst中的点
 *  3 不过由于像素点是整数,一定会有小数被舍弃,
 *    没有经过双线性插值等算法从src中取值的话,旋转的效果仍然不够平滑
 *
 * 数学计算上 一般以 0,0 点为中心点 这样方便计算
 *
 * 假设 一个矩形以0,0点为中心点旋转, 那么得到的目标矩形坐标系也是以0,0点为中心
 *                                           *************
 *                                           *           *
 *  *************************                *           *
 *  *         (0,0)         *    rotate 90   *    (0,0)  *
 *  *          *            *    ----->      *     *     *
 *  *                       *                *           *
 *  *************************                *           *
 *                                           *           *
 *                                           *************
 *
 * 实际代码中先以 0,0 坐标系计算, 然后再把结果分别加上中心点 (d_x,d_y) 或者 (s_x,s_y) 即可
 * 比如 目标矩形 宽x高 是 100x80, 假设以 50,40 为中心旋转
 * 那么计算的时候,将中心点平移到(0,0) 
 * 原来的矩形左上角(0,0) 点就变成 (0-50,0-40), 右下角(100,80) 变成了(100-50,80-40)
 * 即 从 (-50,-40) 到 (50,40) 逐行依次计算,
 *
 * 计算出的结果,坐标系的中心点是 (0,0)
 * 所以得到的点要加上源矩形的中心点,进行坐标转换
 */
static void rotate_func_v0(
    rotate_base_type *s, int s_w, int s_h, int s_lw, int s_x, int s_y,
    rotate_base_type *d, int d_w, int d_h, int d_lw, int d_x, int d_y,
    int cos_a, int sin_a)
{
    int x, y;

    // 计算的时候是以(0,0)点为中心的坐标系
    // 下面要用 d[x] 进行数组操作,
    // 所以这里要先加上d_x,这样就相当于提前做了x+d_x的坐标系转换了
    d += d_x;

    s_x = s_x * FAST_DEN_NUM;
    s_y = s_y * FAST_DEN_NUM;

    for (y = 0-d_y; y < d_h-d_y; y++) {
        for (x = 0-d_x; x < d_w-d_x; x++) {
            // 逆推源数据的坐标,且以s_x,s_y为中心转换坐标系
            int x0 = (cos_a*x - sin_a*y + s_x);
            int y0 = (sin_a*x + cos_a*y + s_y);

            // 如果不在源数据的矩形范围内,那么表示该点不需要赋值
            if (x0 < 0 || x0 > (s_w-1)*FAST_DEN_NUM ||
                y0 < 0 || y0 > (s_h-1)*FAST_DEN_NUM) {
                set_bg_pix(d+x);
                continue;
            }

            rotate_pix(s, s_w, s_h, s_lw, d+x, x0, y0);
        }
        d += d_lw;
    }
}

/**
 * [rotate 算法 V1版本] (以V0版本为基础优化)
 *
 *    通过计算旋转后的 p0,p1,p2,p3 四个点
 *    然后求得 最小dx0,dy0,最大dx1,dx1
 *    得到四个实际在dst中需要搜寻的四个顶点,从而减少搜寻面积,提速
 *     d0(dx0,dy0) d1(dx1,dy0) d2(dx0,dy1) d3(dx1,dy1)
 *
 *                                    ********************************
 *                                    *  d0          p0        d1    *
 *                                    *     +*******+********+       *
 *                                    *     *      *         *       *
 *                                    *     *    *    *      *       *
 *  ***************                   *     *  *        *    *       *
 *  *             *   rotate 45       *  p1 +*            *  *       *
 *  *             *   ------------>   *     *  *            *+ p2    *
 *  *             *                   *     *    *         * *       *
 *  ***************                   *     *      *     *   *       *
 *                                    *     *        * *     *       *
 *                                    * d2  +*********+******+  d3   *
 *                                    *              p3              *
 *                                    ********************************
 */
static void rotate_func_v1(
    rotate_base_type *s, int s_w, int s_h, int s_lw, int s_x, int s_y,
    rotate_base_type *d, int d_w, int d_h, int d_lw, int d_x, int d_y,
    int cos_a, int sin_a)
{
    int x, y;

    // p0,p1,p2,p3
    int tx0 = to_rotate_x(0-s_x, 0-s_y, cos_a, sin_a);
    int ty0 = to_rotate_y(0-s_x, 0-s_y, cos_a, sin_a);

    int tx1 = to_rotate_x(0-s_x, s_h-s_y, cos_a, sin_a);
    int ty1 = to_rotate_y(0-s_x, s_h-s_y, cos_a, sin_a);

    int tx2 = to_rotate_x(s_w-s_x, 0-s_y, cos_a, sin_a);
    int ty2 = to_rotate_y(s_w-s_x, 0-s_y, cos_a, sin_a);

    int tx3 = to_rotate_x(s_w-s_x, s_h-s_y, cos_a, sin_a);
    int ty3 = to_rotate_y(s_w-s_x, s_h-s_y, cos_a, sin_a);

    // 找出最小dx0,dy0,dx1,dy1
    int dx0 = FAST_floor(min(min(tx0, tx1), min(tx2, tx3)))/FAST_DEN_NUM;
    int dy0 = FAST_floor(min(min(ty0, ty1), min(ty2, ty3)))/FAST_DEN_NUM;
    int dx1 = FAST_ceil(max(max(tx0, tx1), max(tx2, tx3)))/FAST_DEN_NUM;
    int dy1 = FAST_ceil(max(max(ty0, ty1), max(ty2, ty3)))/FAST_DEN_NUM;

    dx0 = max(dx0-1, 0-d_x);
    dy0 = max(dy0-1, 0-d_y);

    dx1 = min(dx1+1, d_w-d_x);
    dy1 = min(dy1+1, d_h-d_y);

    rotate_func_clear(d, d_w, d_h, d_x, d_y, dx0, dy0, dx1, dy1, d_lw);

    d += (dy0+d_y)*d_lw + d_x;

    s_x = s_x * FAST_DEN_NUM;
    s_y = s_y * FAST_DEN_NUM;

    for (y = dy0; y < dy1; y++) {
        for (x = dx0; x < dx1; x++) {
            int x0 = (cos_a*x - sin_a*y + s_x);
            int y0 = (sin_a*x + cos_a*y + s_y);

            // 如果不在源数据的矩形范围内,那么表示该点不需要赋值
            if (x0 < 0 || x0 > (s_w-1)*FAST_DEN_NUM ||
                y0 < 0 || y0 > (s_h-1)*FAST_DEN_NUM) {
                set_bg_pix(d+x);
                continue;
            }

            // printf("set: %d,%d %x %p\n", ((y0)/FAST_DEN_NUM), (x0)/FAST_DEN_NUM,
            //     s[((y0)/FAST_DEN_NUM)*s_lw+(x0)/FAST_DEN_NUM], 
            //     &s[((y0)/FAST_DEN_NUM)*s_lw+(x0)/FAST_DEN_NUM]);

            rotate_pix(s, s_w, s_h, s_lw, d+x, x0, y0);
        }
        d += d_lw;
    }
}

/**
 * [rotate 算法 V2版本] (以V1版本为基础优化)
 *
 *    v1版本的算法产生了缩小的矩形,减少的dst的搜寻面积
 *    如下图所示 d0,d1,d2,d3组成的矩形明显大于 p0,p1,p2,p3的菱形面积
 *    所以可以在通过三角公式计算出菱形在矩形中每一行的start和end,
 *    每一行从start计算到end(图中的虚线),从而相对于v1版本算法只计算菱形的点数
 *
 * 此版本的算法在小尺寸图像旋转,以及600x600以下,
 * 以及 10x1080 到600x1080, 以及1920x10 到 1920x200 效果最好
 *
 *    cond_0 当 p1.y < p2.y
 *    cond_1 当 p1.y > p2.y
 *    请用纸笔,画出更陡峭一些的旋转角度,方便推算/理解公式
 *
 *                                    ********************************
 *                                    *  d0          p0        d1    *
 *                                    *     +*******+********+       *
 *                                    *     *      *         *       *
 *                                    *     *    *----*      *       *
 *  ***************    cond_0         *     *  *--------*    *       *
 *  *             *   rotate 45       *  p1 +*------------*  *       *
 *  *             *   ------------>   *     *  *------------*+ p2    *
 *  *             *                   *     *    *-----------*       *
 *  ***************                   *     *      *-----*   *       *
 *                                    *     *        * *     *       *
 *                                    * d2  +*********+******+  d3   *
 *                                    *              p3              *
 *                                    ********************************
 *
 *                                    ********************************
 *                                    *  d0          p0        d1    *
 *                                    *     +********+*******+       *
 *                                    *     *      *----*    *       *
 *                                    *     *    *---------* *       *
 *  ***************    cond_1         *     *  *------------*+ p2    *
 *  *             *   rotate 135      *     **------------ * *       *
 *  *             *   ------------>   *  p1 +----------- *   *       *
 *  *             *                   *     * *------- *     *       *
 *  ***************                   *     *   *--- *       *       *
 *                                    *     *     *          *       *
 *                                    * d2  +*****+**********+  d3   *
 *                                    *              p3              *
 *                                    ********************************
 */
static void rotate_func_v2(
    rotate_base_type *s, int s_w, int s_h, int s_lw, int s_x, int s_y,
    rotate_base_type *d, int d_w, int d_h, int d_lw, int d_x, int d_y,
    int cos_a, int sin_a, int tan_a, int cot_a, int border_bilinear)
{
    int x, y;
    int tx[4];
    int ty[4];

    // p0,p1,p2,p3
    tx[0] = to_rotate_x(0-s_x, 0-s_y, cos_a, sin_a);
    ty[0] = to_rotate_y(0-s_x, 0-s_y, cos_a, sin_a);

    tx[1] = to_rotate_x(0-s_x, s_h-s_y, cos_a, sin_a);
    ty[1] = to_rotate_y(0-s_x, s_h-s_y, cos_a, sin_a);

    tx[2] = to_rotate_x(s_w-s_x, 0-s_y, cos_a, sin_a);
    ty[2] = to_rotate_y(s_w-s_x, 0-s_y, cos_a, sin_a);

    tx[3] = to_rotate_x(s_w-s_x, s_h-s_y, cos_a, sin_a);
    ty[3] = to_rotate_y(s_w-s_x, s_h-s_y, cos_a, sin_a);

    int min_x, min_y, max_x, max_y;
    find_min_max(tx, 4, &min_x, &max_x);
    find_min_max(ty, 4, &min_y, &max_y);

    // dx0那一列上与菱形相交的点的p1.y
    // dx1那一列上与菱形相交的点的p2.y
    int dyl0 = ty[min_x]/FAST_DEN_NUM;
    int dyl1 = ty[max_x]/FAST_DEN_NUM;

    int dx0 = FAST_floor(tx[min_x])/FAST_DEN_NUM;
    int dy0 = FAST_floor(ty[min_y])/FAST_DEN_NUM;
    int dx1 = FAST_ceil(tx[max_x])/FAST_DEN_NUM;
    int dy1 = FAST_ceil(ty[max_y])/FAST_DEN_NUM;

    dx0 -= 1;
    dy0 -= 1;

    dx1 += 1;
    dy1 += 1;

    int fdx0 = max(dx0, 0-d_x);
    int fdy0 = max(dy0, 0-d_y);
    int fdx1 = min(dx1, d_w-d_x);
    int fdy1 = min(dy1, d_h-d_y);

    rotate_base_type *dst = d;

    d += (fdy0+d_y)*d_lw + d_x;

    s_x = s_x * FAST_DEN_NUM;
    s_y = s_y * FAST_DEN_NUM;

    for (y = fdy0; y < fdy1; y++) {
        // 理论上菱形的有效区域就是 dx0+a,dx1-b 之间
        // 理论上菱形的有效区域就是 dx0+a,dx1-b 之间
        int a, b;
        a = y < dyl0 ? (dyl0-y)*tan_a : (y-dyl0)*cot_a;
        b = y < dyl1 ? (dyl1-y)*cot_a : (y-dyl1)*tan_a;

        a = FAST_floor(a)/FAST_DEN_NUM;
        b = FAST_floor(b)/FAST_DEN_NUM;

        int x_start = dx0+a;
        int x_end = dx1-b;

        // 接下来的一长片代码都是为了消除整数计算带来误差

        if (x_start < fdx0)
            x_start = fdx0;
        else if (x_start >= fdx1)
            x_start = fdx1-1;
        if (x_end < fdx0)
            x_end = fdx0;
        else if (x_end >= fdx1)
            x_end = fdx1-1;

        // 误差可能导致 x_start > x_end,所以纠正回起点/终点
        if (x_start >= x_end) {
            x_start = fdx0;
            x_end = fdx1-1;
        }

        // 往起点搜寻x_start,看是否有边界点
        for (x = x_start; x >= fdx0; x--) {
            int x1 = (cos_a*x - sin_a*y + s_x);
            int y1 = (sin_a*x + cos_a*y + s_y);

            if (x1 <= -FAST_DEN_NUM || x1 >= s_w*FAST_DEN_NUM ||
                y1 <= -FAST_DEN_NUM || y1 >= s_h*FAST_DEN_NUM) {
                rotate_func_clear_line(d-d_x, x+d_x+1);
                break;
            }

            if (!border_bilinear || rotate_border_pix(s, s_w, s_h, s_lw, d+x, x1, y1))
                rotate_pix(s, s_w, s_h, s_lw, d+x, x1, y1);
        }

        // 往终点搜寻x_start,看是否有边界点
        for (x = x_start+1; x < x_end; x++) {
            int x1 = (cos_a*x - sin_a*y + s_x);
            int y1 = (sin_a*x + cos_a*y + s_y);

            if (x1 <= -FAST_DEN_NUM || x1 >= s_w*FAST_DEN_NUM ||
                y1 <= -FAST_DEN_NUM || y1 >= s_h*FAST_DEN_NUM) {
                set_bg_pix(d+x);
                continue;
            }
            if (!border_bilinear || rotate_border_pix(s, s_w, s_h, s_lw, d+x, x1, y1)) {
                rotate_pix(s, s_w, s_h, s_lw, d+x, x1, y1);
                break;
            }
        }
        x_start = x;

        // 往终点搜寻x_end,看是否有边界点
        for (x = x_end; x < fdx1; x++) {
            int x1 = (cos_a*x - sin_a*y + s_x);
            int y1 = (sin_a*x + cos_a*y + s_y);

            if (x1 <= -FAST_DEN_NUM || x1 >= s_w*FAST_DEN_NUM ||
                y1 <= -FAST_DEN_NUM || y1 >= s_h*FAST_DEN_NUM) {
                rotate_func_clear_line(d+x, (d_w-d_x)-x);
                break;
            }
            if (!border_bilinear || rotate_border_pix(s, s_w, s_h, s_lw, d+x, x1, y1))
                rotate_pix(s, s_w, s_h, s_lw, d+x, x1, y1);
        }

        // 往起点搜寻x_end,看是否有边界点
        for (x = x_end-1; x > x_start; x--) {
            int x1 = (cos_a*x - sin_a*y + s_x);
            int y1 = (sin_a*x + cos_a*y + s_y);

            if (x1 <= -FAST_DEN_NUM || x1 >= s_w*FAST_DEN_NUM ||
                y1 <= -FAST_DEN_NUM || y1 >= s_h*FAST_DEN_NUM) {
                set_bg_pix(d+x);
                continue;
            }
            if (!border_bilinear || rotate_border_pix(s, s_w, s_h, s_lw, d+x, x1, y1)) {
                rotate_pix(s, s_w, s_h, s_lw, d+x, x1, y1);
                break;
            }
        }
        x_end = x;

        // 前面的代码都是为了消除误差和搜寻边边角角
        // 最终,我们得到了可以毫无顾忌直接计算并且赋值的x_start,x_end
        for (x = x_start+1; x < x_end; x++) {
            int x1 = (cos_a*x - sin_a*y + s_x);
            int y1 = (sin_a*x + cos_a*y + s_y);

            // assert (!(x1 < 0 || x1 >= s_w || y1 < 0 || y1 >= s_h));
            rotate_pix(s, s_w, s_h, s_lw, d+x, x1, y1);
        }

        d += d_lw;
    }
}

#if 0

static inline void copy_rectN(rotate_base_type *s, rotate_base_type *d, int w, int h, int d_lw)
{
    int i, j;
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            s[i] = d[i];
        }
        d += d_lw;
        s += w;
    }
}

#define N 32

/**
 * [rotate 算法 不完全稳定版本v1] (以V1版本为基础优化)
 *
 * 由于旋转的时候读 cache miss 很严重,所以把src分成多个NxN的小方块旋转到目标上去
 * 这个算法现在有点缺陷,就是目标在特定角度总是有一两个点没法覆盖到
 */
static void rotate_func_not_stable_v1(
    rotate_base_type *s, int s_w, int s_h, int s_lw, int s_x, int s_y,
    rotate_base_type *d, int d_w, int d_h, int d_lw, int d_x, int d_y,
    int cos_a, int sin_a, int tan_a, int cot_a)
{
    rotate_base_type tmp[N*N];

    rotate_base_type *save_s = s;

    int i, j;
    for (j = 0; j < s_h; j += N-1) {
        int h = s_h - j;
        if (h > N)
            h = N;
        for (i = 0; i < s_w; i += N-1) {
            int w = s_w - i;
            if (w > N)
                w = N;
            copy_rectN(tmp, s+i, w, h, s_lw);
            rotate_func_v1(tmp, w, h, w, s_x-i, s_y-j, d, d_w, d_h, d_lw, d_x, d_y, cos_a, sin_a);

        }
        s += (N-1)*s_lw;
    }
}
#undef N
#endif

/**
 * [rotate 算法 V3版本] (以V1版本为基础优化)
 *
 * rotate 的时候读的 cache miss 太严重,所以一次处理目标的四个点减少一些cache miss
 * 为啥一次处理目标的四个点要选同一个x,四个y而不是同一个y,四个x? 测试出来的,同一个x,4个y效果好
 *
 * 为啥不以v2版本为基础呢? v2版本边边角角太多了,展开过于麻烦...
 */
static void rotate_func_v3(
    rotate_base_type *s, int s_w, int s_h, int s_lw, int s_x, int s_y,
    rotate_base_type *d, int d_w, int d_h, int d_lw, int d_x, int d_y,
    int cos_a, int sin_a)
{
    int x, y;

    int tx0 = to_rotate_x(0-s_x, 0-s_y, cos_a, sin_a);
    int ty0 = to_rotate_y(0-s_x, 0-s_y, cos_a, sin_a);

    int tx1 = to_rotate_x(0-s_x, s_h-s_y, cos_a, sin_a);
    int ty1 = to_rotate_y(0-s_x, s_h-s_y, cos_a, sin_a);

    int tx2 = to_rotate_x(s_w-s_x, 0-s_y, cos_a, sin_a);
    int ty2 = to_rotate_y(s_w-s_x, 0-s_y, cos_a, sin_a);

    int tx3 = to_rotate_x(s_w-s_x, s_h-s_y, cos_a, sin_a);
    int ty3 = to_rotate_y(s_w-s_x, s_h-s_y, cos_a, sin_a);

    int dx0 = FAST_floor(min(min(tx0, tx1), min(tx2, tx3)))/FAST_DEN_NUM;
    int dy0 = FAST_floor(min(min(ty0, ty1), min(ty2, ty3)))/FAST_DEN_NUM;
    int dx1 = FAST_ceil(max(max(tx0, tx1), max(tx2, tx3)))/FAST_DEN_NUM;
    int dy1 = FAST_ceil(max(max(ty0, ty1), max(ty2, ty3)))/FAST_DEN_NUM;

    dx0 = max(dx0-1, 0-d_x);
    dy0 = max(dy0-1, 0-d_y);

    dx1 = min(dx1+1, d_w-d_x);
    dy1 = min(dy1+1, d_h-d_y);

    rotate_func_clear(d, d_w, d_h, d_x, d_y, dx0, dy0, dx1, dy1, d_lw);

    d += (dy0+d_y)*d_lw + d_x;

    s_x = s_x * FAST_DEN_NUM;
    s_y = s_y * FAST_DEN_NUM;

    for (y = dy0; y < dy1-(dy1-dy0)%4; y += 4) {
        int cos_y0 = cos_a*(y+0);
        int cos_y1 = cos_a*(y+1);
        int cos_y2 = cos_a*(y+2);
        int cos_y3 = cos_a*(y+3);

        int sin_y0 = sin_a*(y+0);
        int sin_y1 = sin_a*(y+1);
        int sin_y2 = sin_a*(y+2);
        int sin_y3 = sin_a*(y+3);

        for (x = dx0; x < dx1; x++) {
            int x0 = (cos_a*x - sin_y0 + s_x);
            int y0 = (sin_a*x + cos_y0 + s_y);

            int x1 = (cos_a*x - sin_y1 + s_x);
            int y1 = (sin_a*x + cos_y1 + s_y);

            int x2 = (cos_a*x - sin_y2 + s_x);
            int y2 = (sin_a*x + cos_y2 + s_y);

            int x3 = (cos_a*x - sin_y3 + s_x);
            int y3 = (sin_a*x + cos_y3 + s_y);

            if (x0 < 0 || x0 > (s_w-1)*FAST_DEN_NUM ||
                y0 < 0 || y0 > (s_h-1)*FAST_DEN_NUM)
                set_bg_pix(d+x);
            else
                rotate_pix(s, s_w, s_h, s_lw, d+x, x0, y0);
            
            if (x1 < 0 || x1 > (s_w-1)*FAST_DEN_NUM ||
                y1 < 0 || y1 > (s_h-1)*FAST_DEN_NUM)
                set_bg_pix(d+1*d_lw+x);
            else
                rotate_pix(s, s_w, s_h, s_lw, d+1*d_lw+x, x1, y1);
            
            if (x2 < 0 || x2 > (s_w-1)*FAST_DEN_NUM ||
                y2 < 0 || y2 > (s_h-1)*FAST_DEN_NUM)
                set_bg_pix(d+2*d_lw+x);
            else
                rotate_pix(s, s_w, s_h, s_lw, d+2*d_lw+x, x2, y2);

            if (x3 < 0 || x3 > (s_w-1)*FAST_DEN_NUM ||
                y3 < 0 || y3 > (s_h-1)*FAST_DEN_NUM)
                set_bg_pix(d+3*d_lw+x);
            else
                rotate_pix(s, s_w, s_h, s_lw, d+3*d_lw+x, x3, y3);
        }
        d += 4*d_lw;
    }

    for ( ; y < dy1; y++) {
        for (x = dx0; x < dx1; x++) {
            int x0 = (cos_a*x - sin_a*y + s_x);
            int y0 = (sin_a*x + cos_a*y + s_y);

            if (x0 < 0 || x0 > (s_w-1)*FAST_DEN_NUM ||
                y0 < 0 || y0 > (s_h-1)*FAST_DEN_NUM) {
                continue;
            }

            rotate_pix(s, s_w, s_h, s_lw, d+x, x0, y0);
        }
        d += d_lw;
    }
}

#define N 64

/**
 * [rotate 算法 V4版本] (以V1 v2版本为基础优化)
 *
 * 由于旋转的时候读 cache miss 很严重,所以把src分成多个NxN的小方块旋转到目标上去
 * 这个算法现在有点缺陷,就是目标在特定角度总是有一两个点没法覆盖到
 *
 * 所以这里换个思路,把dst分成多个NxN的小方块,一次搜寻一个小方块
 *
 * 这个版本的算法在x2600e上面对比较大的图片效果很好
 * 为啥不以v2版本为基础呢? v2版本边边角角太多了,展开过于麻烦...
 */
static void rotate_func_v4(
    rotate_base_type *s, int s_w, int s_h, int s_lw, int s_x, int s_y,
    rotate_base_type *d, int d_w, int d_h, int d_lw, int d_x, int d_y,
    int cos_a, int sin_a)
{
    int x, y;

    int tx0 = to_rotate_x(0-s_x, 0-s_y, cos_a, sin_a);
    int ty0 = to_rotate_y(0-s_x, 0-s_y, cos_a, sin_a);

    int tx1 = to_rotate_x(0-s_x, s_h-s_y, cos_a, sin_a);
    int ty1 = to_rotate_y(0-s_x, s_h-s_y, cos_a, sin_a);

    int tx2 = to_rotate_x(s_w-s_x, 0-s_y, cos_a, sin_a);
    int ty2 = to_rotate_y(s_w-s_x, 0-s_y, cos_a, sin_a);

    int tx3 = to_rotate_x(s_w-s_x, s_h-s_y, cos_a, sin_a);
    int ty3 = to_rotate_y(s_w-s_x, s_h-s_y, cos_a, sin_a);

    int dx0 = FAST_floor(min(min(tx0, tx1), min(tx2, tx3)))/FAST_DEN_NUM;
    int dy0 = FAST_floor(min(min(ty0, ty1), min(ty2, ty3)))/FAST_DEN_NUM;
    int dx1 = FAST_ceil(max(max(tx0, tx1), max(tx2, tx3)))/FAST_DEN_NUM;
    int dy1 = FAST_ceil(max(max(ty0, ty1), max(ty2, ty3)))/FAST_DEN_NUM;

    dx0 = max(dx0-1, 0-d_x);
    dy0 = max(dy0-1, 0-d_y);

    dx1 = min(dx1+1, d_w-d_x);
    dy1 = min(dy1+1, d_h-d_y);

    d += (dy0+d_y)*d_lw + d_x;

    s_x = s_x * FAST_DEN_NUM;
    s_y = s_y * FAST_DEN_NUM;

    // printf("len: %dx%d\n", dx1-dx0, dy1-dy0);

    int i, j;
    rotate_base_type *save_d = d;

    for (j = dy0; j < dy1-(dy1-dy0)%4; j+=N) {
        int yend = j+N;
        if (yend > dy1-(dy1-dy0)%4)
            yend = dy1-(dy1-dy0)%4;
        for (i = dx0; i < dx1; i+=N) {
            int xend = i+N;
            if (xend > dx1)
                xend = dx1;
            d = save_d;
            for (y = j; y < yend; y+=4) {
                int cos_y0 = cos_a*(y+0);
                int cos_y1 = cos_a*(y+1);
                int cos_y2 = cos_a*(y+2);
                int cos_y3 = cos_a*(y+3);

                int sin_y0 = sin_a*(y+0);
                int sin_y1 = sin_a*(y+1);
                int sin_y2 = sin_a*(y+2);
                int sin_y3 = sin_a*(y+3);

                for (x = i; x < xend; x++) {
                    int x0 = (cos_a*x - sin_y0 + s_x);
                    int y0 = (sin_a*x + cos_y0 + s_y);

                    int x1 = (cos_a*x - sin_y1 + s_x);
                    int y1 = (sin_a*x + cos_y1 + s_y);

                    int x2 = (cos_a*x - sin_y2 + s_x);
                    int y2 = (sin_a*x + cos_y2 + s_y);

                    int x3 = (cos_a*x - sin_y3 + s_x);
                    int y3 = (sin_a*x + cos_y3 + s_y);

                    if (x0 < 0 || x0 > (s_w-1)*FAST_DEN_NUM ||
                        y0 < 0 || y0 > (s_h-1)*FAST_DEN_NUM)
                        set_bg_pix(d+x);
                    else
                        rotate_pix(s, s_w, s_h, s_lw, d+x, x0, y0);
                    
                    if (x1 < 0 || x1 > (s_w-1)*FAST_DEN_NUM ||
                        y1 < 0 || y1 > (s_h-1)*FAST_DEN_NUM)
                        set_bg_pix(d+1*d_lw+x);
                    else
                        rotate_pix(s, s_w, s_h, s_lw, d+1*d_lw+x, x1, y1);
                    
                    if (x2 < 0 || x2 > (s_w-1)*FAST_DEN_NUM ||
                        y2 < 0 || y2 > (s_h-1)*FAST_DEN_NUM)
                        set_bg_pix(d+2*d_lw+x);
                    else
                        rotate_pix(s, s_w, s_h, s_lw, d+2*d_lw+x, x2, y2);

                    if (x3 < 0 || x3 > (s_w-1)*FAST_DEN_NUM ||
                        y3 < 0 || y3 > (s_h-1)*FAST_DEN_NUM)
                        set_bg_pix(d+3*d_lw+x);
                    else
                        rotate_pix(s, s_w, s_h, s_lw, d+3*d_lw+x, x3, y3);
                }
                d += 4*d_lw;
            }
        }
        save_d += N*d_lw;
    }

    int yend = dy1;
    j = dy1-(dy1-dy0)%4;
    save_d = d;
    for (i = dx0; i < dx1; i+=N) {
        int xend = i+N;
        if (xend > dx1)
            xend = dx1;
        d = save_d;
        for (y = j; y < yend; y++) {
            for (x = i; x < xend; x++) {
                int x0 = (cos_a*x - sin_a*y + s_x);
                int y0 = (sin_a*x + cos_a*y + s_y);

                if (x0 < 0 || x0 > (s_w-1)*FAST_DEN_NUM ||
                    y0 < 0 || y0 > (s_h-1)*FAST_DEN_NUM) {
                    set_bg_pix(d+x);
                    continue;
                }

                rotate_pix(s, s_w, s_h, s_lw, d+x, x0, y0);
            }
            d += d_lw;
        }
    }

}

#undef N

/**
 * 针对几乎是单色的指针,旋转,可以采用查找边缘的方式优化,或者直接旋转后做个抗锯齿
 */

typedef void (*rotate_func_func_t)(
    rotate_base_type *s, int s_w, int s_h, int s_lw, int s_x, int s_y,
    rotate_base_type *d, int d_w, int d_h, int d_lw, int d_x, int d_y,
    int cos_a, int sin_a);

static rotate_func_func_t rotate_func_funcs[] = {
    [0] = rotate_func_v0,
    [1] = rotate_func_v1,
 // [2] = rotate_func_v2, v2 参数不一样,单独处理
    [3] = rotate_func_v3,
    [4] = rotate_func_v4,
};

static void rotate_func(
    void *s_, int s_w, int s_h, int s_x, int s_y, int s_linesz,
    void *d_, int d_w, int d_h, int d_x, int d_y, int d_linesz,
    float angle, int border_bilinear,
    int rotate_version, rotate_get_fast_version_t get_fast_version)
{
    rotate_base_type *s = s_;
    rotate_base_type *d = d_;
    int cos_a = cos((360-angle)*M_PI/180)*FAST_DEN_NUM;
    int sin_a = sin((360-angle)*M_PI/180)*FAST_DEN_NUM;

    if (s_linesz%sizeof(rotate_base_type) || d_linesz%sizeof(rotate_base_type)) {
        fprintf(stderr, "rotate: line size not aligned to base type\n");
        assert(0);
    }

    int s_lw = s_linesz/sizeof(rotate_base_type);
    int d_lw = d_linesz/sizeof(rotate_base_type);

    if (get_fast_version) {
        int a = floor(angle+0.5);
        a = a%360;
        if (a < 0)
            a = 360 - a;
        rotate_version = get_fast_version(a);
    }

    if (rotate_version < 0 || rotate_version > 4) {
        fprintf(stderr, "rotate: no this version:%d, use version 2\n", rotate_version);
        rotate_version = 2;
    }

    if (rotate_version == 2) {
        if ((int)angle%90 == 0 || (int)angle%180 == 0)
            rotate_func_v1(s, s_w, s_h, s_lw, s_x, s_y,
                d, d_w, d_h, d_lw, d_x, d_y,
                cos_a, sin_a);
        else {
            int angle2 = (int)angle-(int)angle%90;
            double tana = tan((angle-angle2)*M_PI/180);
            int tan_a = tana*FAST_DEN_NUM;
            if (tan_a < 0)
                tan_a = -tan_a;
            int cot_a = 1/tana*FAST_DEN_NUM;
            if (cot_a < 0)
                cot_a = -cot_a;
            rotate_func_v2(s, s_w, s_h, s_lw, s_x, s_y,
                        d, d_w, d_h, d_lw, d_x, d_y,
                        cos_a, sin_a, tan_a, cot_a, border_bilinear);
        }
        return;
    }

    if  (border_bilinear) {
    int angle2 = (int)angle-(int)angle%90;
    double tana = tan((angle-angle2)*M_PI/180);
    int tan_a = tana*FAST_DEN_NUM;
    if (tan_a < 0)
        tan_a = -tan_a;
    int cot_a = 1/tana*FAST_DEN_NUM;
    if (cot_a < 0)
        cot_a = -cot_a;

    rotate_func_border(s, s_w, s_h, s_lw, s_x, s_y,
                        d, d_w, d_h, d_lw, d_x, d_y,
                        cos_a, sin_a, tan_a, cot_a);
    }

    rotate_func_funcs[rotate_version](s, s_w, s_h, s_lw, s_x, s_y,
            d, d_w, d_h, d_lw, d_x, d_y,
            cos_a, sin_a);
}

#undef rotate_func
#undef rotate_func_func_t
#undef rotate_func_funcs
#undef rotate_func_pix
#undef rotate_func_v0
#undef rotate_func_v1
#undef rotate_func_v2
#undef rotate_func_v3
#undef rotate_func_v4
#undef rotate_linear
#undef rotate_bilinear
#undef rotate_linear_
#undef rotate_bilinear_
#undef rotate_func_border_pix
#undef rotate_func_border
#undef set_bg_pix
#undef rotate_pix
#undef rotate_func_clear_line
#undef rotate_func_clear_rect
#undef rotate_func_clear
#undef rotate_def
#undef ROTATE_COMMON_FUNC
#undef blend_pix
