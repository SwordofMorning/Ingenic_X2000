#include <stdlib.h>
#include <libmedia/rotate_video.h>
#include <libmedia/rotate_video.h>
#include <libmedia/video_frame.h>
#include <errno.h>

#include <libutils2/boot_time.h>

#define ALIGN_DOWN(X, n) ((X) - ((X)%(n)))

static int default_use_method0(int width, int height, float angle, int type_size)
{
    if (angle < 0)
        angle = 0 - angle;
    int a = angle - ((int)angle)/360*360;
    if (a == 90 || a == 270)
        return 1;

    return 0;
}

int (*rotate_check_method)(int width, int height, float angle, int type_size)
     = default_use_method0;

static void copy_uv_to_full_uv(
    uint16_t *s, int s_lw, uint16_t *d, int d_lw, int w, int h)
{
    uint16_t *d0 = d;
    uint16_t *d1 = d + d_lw;
    int delta = s_lw - w/2;

    int i, j;
    for (i = 0; i < h; i+=2) {
        for (j = 0; j < ALIGN_DOWN(w, 8); j+=8) {
            uint16_t s0 = s[0];
            uint16_t s1 = s[1];
            uint16_t s2 = s[2];
            uint16_t s3 = s[3];
            d0[j+0] = s0; d0[j+1] = s0;
            d0[j+2] = s1; d0[j+3] = s1;
            d0[j+4] = s2; d0[j+5] = s2;
            d0[j+6] = s3; d0[j+7] = s3;
            d1[j+0] = s0; d1[j+1] = s0;
            d1[j+2] = s1; d1[j+3] = s1;
            d1[j+4] = s2; d1[j+5] = s2;
            d1[j+6] = s3; d1[j+7] = s3;
            s += 4;
        }
        for (; j < w; j+=2) {
            uint16_t s0 = s[0];
            d0[j+0] = s0;
            d0[j+1] = s0;
            d1[j+0] = s0;
            d1[j+1] = s0;
            s++;
        }
        d0 += 2*d_lw;
        d1 += 2*d_lw;
        s += delta;
    }
}

static void copy_full_uv_to_uv(
    uint16_t *s, int s_lw, uint16_t *d, int d_lw, int w, int h)
{
    w *= 2;
    d_lw *= 2;
    s_lw *= 2;

    uint8_t *s0 = (void*)s;
    uint8_t *s1 = s0 + s_lw;
    uint8_t *d0 = (void *)d;
    int delta = d_lw - w/2;

    int i, j;
    for (i = 0; i < h; i+=2) {
        for (j = 0; j < ALIGN_DOWN(w, 8); j+=8) {
            uint8_t u00 = s0[j+0];
            uint8_t v00 = s0[j+1];
            uint8_t u01 = s0[j+2];
            uint8_t v01 = s0[j+3];
            uint8_t u02 = s0[j+4];
            uint8_t v02 = s0[j+5];
            uint8_t u03 = s0[j+6];
            uint8_t v03 = s0[j+7];
            uint8_t u10 = s1[j+0];
            uint8_t v10 = s1[j+1];
            uint8_t u11 = s1[j+2];
            uint8_t v11 = s1[j+3];
            uint8_t u12 = s1[j+4];
            uint8_t v12 = s1[j+5];
            uint8_t u13 = s1[j+6];
            uint8_t v13 = s1[j+7];

            d0[0] = ((uint16_t)u00+u01+u10+u11) / 4;
            d0[1] = ((uint16_t)v00+v01+v10+v11) / 4;
            d0[2] = ((uint16_t)u02+u03+u12+u13) / 4;
            d0[3] = ((uint16_t)v02+v03+v12+v13) / 4;
            d0 += 4;
        }
        if (j != w) {
            uint8_t u00 = s0[j+0];
            uint8_t v00 = s0[j+1];
            uint8_t u01 = s0[j+2];
            uint8_t v01 = s0[j+3];

            uint8_t u10 = s1[j+0];
            uint8_t v10 = s1[j+1];
            uint8_t u11 = s1[j+2];
            uint8_t v11 = s1[j+3];

            d0[0] = ((uint16_t)u00+u01+u10+u11) / 4;
            d0[1] = ((uint16_t)v00+v01+v10+v11) / 4;
            d0 += 2;
        }
        s0 += 2*s_lw;
        s1 += 2*s_lw;
        d0 += delta;
    }
}

int rotate_nv12(
    uint8_t **src, uint32_t *src_linesz, int src_width, int src_height,
    uint8_t **dst, uint32_t *dst_linesz, int dst_width, int dst_height,
    float angle, uint32_t color)
{
    unsigned char y_color = (color >> 16) & 0xff;
    unsigned char u_color = (color >> 8) & 0xff;
    unsigned char v_color = (color >> 0) & 0xff;

    rotate_8(src[0], src_width, src_height, src_linesz[0], dst[0], dst_width, dst_height, dst_linesz[0], angle, y_color);

    void *tmp_src = malloc(2*src_width*src_height + 2*dst_width*dst_height);
    if (!tmp_src) {
        fprintf(stderr, "rotate: failed to alloc tmp buffer\n");
        return -1;
    }

    void *tmp_dst = tmp_src + 2*src_width*src_height;

    copy_uv_to_full_uv((uint16_t*)src[1], src_linesz[1]/2, tmp_src, src_width, src_width, src_height);

    rotate_16(tmp_src, src_width, src_height, src_width*2,
            tmp_dst, dst_width, dst_height, dst_width*2, angle, ((uint16_t)v_color << 8)|u_color);

    copy_full_uv_to_uv(tmp_dst, dst_width, (uint16_t *)dst[1], dst_linesz[1]/2, dst_width, dst_height);

    free(tmp_src);

    return 0;
}

int rotate_bgra(
    uint8_t **s_data, uint32_t *s_linesize, int s_width, int s_height,
    uint8_t **d_data, uint32_t *d_linesize, int d_width, int d_height,
    float angle, uint32_t color)
{
    rotate_32((void *)s_data[0], s_width, s_height, s_linesize[0],
     (void*)d_data[0], d_width, d_height, d_linesize[0], angle, color);
     return 0;
}

int rotate_y8(
    uint8_t **s_data, uint32_t *s_linesize, int s_width, int s_height,
    uint8_t **d_data, uint32_t *d_linesize, int d_width, int d_height,
    float angle, uint32_t color)
{
    rotate_8((void *)s_data[0], s_width, s_height, s_linesize[0],
     (void*)d_data[0], d_width, d_height, d_linesize[0], angle, color);
    return 0;
}

int rotate_y16(
    uint8_t **s_data, uint32_t *s_linesize, int s_width, int s_height,
    uint8_t **d_data, uint32_t *d_linesize, int d_width, int d_height,
    float angle, uint32_t color)
{
    rotate_16((void *)s_data[0], s_width, s_height, s_linesize[0],
     (void*)d_data[0], d_width, d_height, d_linesize[0], angle, color);
    return 0;
}
