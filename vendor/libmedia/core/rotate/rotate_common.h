#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#ifndef FAST_DEN_NUM
#define FAST_DEN_NUM 4096
#endif

#ifndef rotate_base_type
#define rotate_base_type uint32_t
#endif

static void rotate_base_part0(
    rotate_base_type *s, int s_w, int s_h, int s_lw,
    rotate_base_type *d, int d_w, int d_h, int d_lw,
    int from, int to,
    int cos_angle, int sin_angle, rotate_base_type color)
{
    int x, y;

    int sx_center = s_w / 2;
    int sy_center = s_h / 2;

    int dx_center = d_w / 2;
    int dy_center = d_h / 2;

    d += from * d_lw;

    for (y = from; y < to; y++) {
        for (x = 0; x < d_w; x++) {
            int y0 = y-dy_center;
            int x0 = x-dx_center;
            int x1 = (cos_angle*x0 - sin_angle*y0)/FAST_DEN_NUM;
            int y1 = (sin_angle*x0 + cos_angle*y0)/FAST_DEN_NUM;

            x1 += sx_center;
            y1 += sy_center;

            if (x1 < 0 || x1 >= s_w || y1 < 0 || y1 >= s_h)
                d[x] = color;
            else
                d[x] = s[y1*s_lw + x1];
        }
        d += d_lw;
    }
}

static void rotate_base_part1(
    rotate_base_type *s, int s_w, int s_h, int s_lw,
    rotate_base_type *d, int d_w, int d_h, int d_lw,
    int from, int to,
    int cos_angle, int sin_angle, rotate_base_type color)
{
    int x, y;

    int sx_center = s_w / 2;
    int sy_center = s_h / 2;

    int dx_center = d_w / 2;
    int dy_center = d_h / 2;

    d += from * d_lw;

    for (y = from; y < to; y+=4) {
        for (x = 0; x < d_w; x++) {
            int x0 = x-dx_center;
            int y0 = y+0-dy_center;
            int y1 = y+1-dy_center;
            int y2 = y+2-dy_center;
            int y3 = y+3-dy_center;

            int a0 = (cos_angle*x0 - sin_angle*y0)/FAST_DEN_NUM;
            int b0 = (sin_angle*x0 + cos_angle*y0)/FAST_DEN_NUM;

            int a1 = (cos_angle*x0 - sin_angle*y1)/FAST_DEN_NUM;
            int b1 = (sin_angle*x0 + cos_angle*y1)/FAST_DEN_NUM;

            int a2 = (cos_angle*x0 - sin_angle*y2)/FAST_DEN_NUM;
            int b2 = (sin_angle*x0 + cos_angle*y2)/FAST_DEN_NUM;

            int a3 = (cos_angle*x0 - sin_angle*y3)/FAST_DEN_NUM;
            int b3 = (sin_angle*x0 + cos_angle*y3)/FAST_DEN_NUM;

            a0 += sx_center;
            b0 += sy_center;
            a1 += sx_center;
            b1 += sy_center;
            a2 += sx_center;
            b2 += sy_center;
            a3 += sx_center;
            b3 += sy_center;

            if (a0 < 0 || a0 >= s_w || b0 < 0 || b0 >= s_h)
                d[x+0*d_lw] = color;
            else
                d[x+0*d_lw] = s[b0*s_lw + a0];
            if (a1 < 0 || a1 >= s_w || b1 < 0 || b1 >= s_h)
                d[x+1*d_lw] = color;
            else
                d[x+1*d_lw] = s[b1*s_lw + a1];
            if (a2 < 0 || a2 >= s_w || b2 < 0 || b2 >= s_h)
                d[x+2*d_lw] = color;
            else
                d[x+2*d_lw] = s[b2*s_lw + a2];
            if (a3 < 0 || a3 >= s_w || b3 < 0 || b3 >= s_h)
                d[x+3*d_lw] = color;
            else
                d[x+3*d_lw] = s[b3*s_lw + a3];
        }
        d += 4*d_lw;
    }
}

/*
 * 不同的cpu平台在处理不同的大小和角度的时候,采用旋转的算法的时间会有区别
 * 实际测试x2000e 1280x720@bgra 旋转 大于 20° 的时候,方法1会明显好于方法0
 * 然而 8,16 位宽度时,仍然是方法0速度快
 * 用户如果在乎性能可以进行测试,然后设置此函数指针,用于特定需要优化的场景
 */
extern int (*rotate_check_method)(int width, int height, float angle, int type_size);

static void rotate_base(
    rotate_base_type *s, int s_w, int s_h, int s_linesz,
    rotate_base_type *d, int d_w, int d_h, int d_linesz,
    float angle, rotate_base_type color)
{
    int cos_angle = cos((360-angle)*M_PI/180)*FAST_DEN_NUM;
    int sin_angle = sin((360-angle)*M_PI/180)*FAST_DEN_NUM;

    if (s_linesz%sizeof(rotate_base_type) || d_linesz%sizeof(rotate_base_type)) {
        fprintf(stderr, "rotate: line size not aligned to base type\n");
        assert(0);
    }

    int s_lw = s_linesz/sizeof(rotate_base_type);
    int d_lw = d_linesz/sizeof(rotate_base_type);

    int method = rotate_check_method(s_w, s_h, angle, sizeof(rotate_base_type));

    if (method == 0) {
        rotate_base_part0(s, s_w, s_h, s_lw, d, d_w, d_h, d_lw, 0, d_h,
                    cos_angle, sin_angle, color);
    } else {
        rotate_base_part1(s, s_w, s_h, s_lw, d, d_w, d_h, d_lw, 0, d_h-d_h%4,
                        cos_angle, sin_angle, color);
        if (d_h%4)
        rotate_base_part0(s, s_w, s_h, s_lw, d, d_w, d_h, d_lw, d_h-d_h%4, d_h,
                    cos_angle, sin_angle, color);
    }
}