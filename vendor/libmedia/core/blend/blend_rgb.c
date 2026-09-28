#include <stdint.h>
#include <libmedia/blend_video.h>
#include <stdio.h>

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
    if (_sa <= 0x8)                               \
         break;                                   \
                                                  \
    /* src_a > 0xf8 时,src几乎不透明 */             \
    /* 所以直接覆盖底色 */                           \
    if (_sa > 0xf8) {                             \
        store_dst(_sr, _sg, _sb, 0xff);           \
        break;                                    \
    }                                             \
                                                  \
    uint8_t _dr, _dg, _db, _da;                   \
    load_dst(_dr, _dg, _db, _da);                 \
                                                  \
    /* _da < 0x08 可认为是 0x00, 可以简化计算公式 */  \
    if (_da < 0x08) {                             \
        store_dst(_sr, _sg, _sb, _sa);            \
        break;                                    \
    }                                             \
                                                  \
    /* _da > 0xf4 可认为是 0xff, 可以简化计算公式*/   \
    if (_da > 0xf4) {                             \
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
    if (out_a < 0)                                \
        out_a = 0;                                \
                                                  \
    if (out_a) {                                  \
        _r = (_dr * _da + _sr * _sa) / out_a;     \
        _g = (_dg * _da + _sg * _sa) / out_a;     \
        _b = (_db * _da + _sb * _sa) / out_a;     \
        store_dst(_r, _g, _b, out_a);             \
        break;                                    \
    }                                             \
                                                  \
    store_dst(0, 0, 0, 0);                        \
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

void blend_alpha_color_bgra(uint8_t *src, int src_linesize,
                            uint8_t *dst, int dst_linesize,
                 int width, int height, unsigned int color)
{
    int i, j;

    unsigned char alpha = (color >> 24) & 0xff;
    unsigned char r = (color >> 16) & 0xff;
    unsigned char g = (color >> 8) & 0xff;
    unsigned char b = (color >> 0) & 0xff;

    uint8_t *s = src;
    uint8_t *d = dst;
    int delta_s = src_linesize - width;
    int delta_d = dst_linesize - width*4;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            uint8_t s_a = s[0];
            blend_pixel_no_premulti(r, g, b, alpha, s_a, load_d, store_d);
            s += 1;
            d += 4;
        }
        s += delta_s;
        d += delta_d;
    }
}

void blend_bgra_to_bgra(uint8_t **s_data, uint32_t *s_linesize,
                        uint8_t **d_data, uint32_t *d_linesize,
                        int width, int height, uint8_t alpha)
{
    int i, j;

    uint8_t *s = s_data[0];
    uint8_t *d = d_data[0];
    int src_linesize = s_linesize[0];
    int dst_linesize = d_linesize[0];
    int delta_s = src_linesize - width*4;
    int delta_d = dst_linesize - width*4;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            uint8_t s_b = s[0];
            uint8_t s_g = s[1];
            uint8_t s_r = s[2];
            uint8_t s_a = s[3];
            blend_pixel_no_premulti(
                s_r, s_g, s_b, s_a, alpha, load_d, store_d);
            s += 4;
            d += 4;
        }
        s += delta_s;
        d += delta_d;
    }
}
