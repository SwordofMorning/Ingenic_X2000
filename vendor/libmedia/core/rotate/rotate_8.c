#define rotate_base_type uint8_t

#include "rotate_common.h"

void rotate_8(
    uint8_t *src, int src_width, int src_height, int src_linesz,
    uint8_t *dst, int dst_width, int dst_height, int dst_linesz,
    float angle, uint8_t color)
{
    rotate_base(
        src, src_width, src_height, src_linesz,
        dst, dst_width, dst_height, dst_linesz, angle, color);
}
