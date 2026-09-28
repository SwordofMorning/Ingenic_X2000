#ifndef __SIMPLE_BAYER16_TO_NV12__
#define __SIMPLE_BAYER16_TO_NV12__

#include <string.h>

/**
 * @brief bayer16格式的图像数据转换成nv12格式
 * @param _dst  转换后图像数据的起始地址
 * @param _src  源图像数据的起始地址
 * @param xres  图像的宽度
 * @param yres  图像的高度
 * @param line_length   图像每行数据的字节数量
 * @return 无
 */
void simple_bayer16_to_nv12(void *_dst, void *_src, int xres, int yres, int line_length);

#endif /* __SIMPLE_BAYER16_TO_NV12__ */