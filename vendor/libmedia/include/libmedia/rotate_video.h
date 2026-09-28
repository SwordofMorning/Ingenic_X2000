#ifndef _ROTATE_VIDEO_H_
#define _ROTATE_VIDEO_H_

#include <stdint.h>

/*
 * 不同的cpu平台在处理不同的大小和角度的时候,采用旋转的算法的时间会有区别
 * 实际测试x2000e 1280x720@bgra 旋转 大于 20° 的时候,方法1会明显好于方法0
 * 然而 8,16 位宽度时,仍然是方法0速度快
 * 用户如果在乎性能可以进行测试,然后设置此函数指针,用于特定需要优化的场景
 */
extern int (*rotate_check_method)(int width, int height, float angle, int type_size);

void rotate_32(
    uint32_t *src, int src_width, int src_height, int src_linesz,
    uint32_t *dst, int dst_width, int dst_height, int dst_linesz,
    float angle, uint32_t color);

void rotate_16(
    uint16_t *src, int src_width, int src_height, int src_linesz,
    uint16_t *dst, int dst_width, int dst_height, int dst_linesz,
    float angle, uint16_t color);

void rotate_8(
    uint8_t *src, int src_width, int src_height, int src_linesz,
    uint8_t *dst, int dst_width, int dst_height, int dst_linesz,
    float angle, uint8_t color);

int rotate_bgra(
    uint8_t **s_data, uint32_t *s_linesize, int s_width, int s_height,
    uint8_t **d_data, uint32_t *d_linesize, int d_width, int d_height,
    float angle, uint32_t color);

int rotate_y8(
    uint8_t **s_data, uint32_t *s_linesize, int s_width, int s_height,
    uint8_t **d_data, uint32_t *d_linesize, int d_width, int d_height,
    float angle, uint32_t color);

int rotate_y16(
    uint8_t **s_data, uint32_t *s_linesize, int s_width, int s_height,
    uint8_t **d_data, uint32_t *d_linesize, int d_width, int d_height,
    float angle, uint32_t color);

int rotate_nv12(
    uint8_t **s_data, uint32_t *s_linesize, int s_width, int s_height,
    uint8_t **d_data, uint32_t *d_linesize, int d_width, int d_height,
    float angle, uint32_t color);

#endif /* _ROTATE_VIDEO_H_ */
