#ifndef __BAYER16_TO_RGB__
#define __BAYER16_TO_RGB__

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libhardware2/camera.h>
#include <libhardware2/fb.h>

/**
 * @brief bayer16格式的图像数据转换成rgb格式，转换后的图像显示在fb设备上
 * @param cam_bayer_buf  bayer图像数据的起始地址
 * @param cam_fmt   bayer数据的格式
 * @param cam_bytes_per_line  cam设备每行数据的字节数量
 * @param fb_rgb_buf  转换成功后，rgb图像数据的起始存储地址
 * @param fb_fmt    fb设备需要的数据格式
 * @param fb_bytes_per_line   fb设备每行数据的字节数量
 * @param width   显示图像数据的宽度
 * @param height  显示图像数据的高度
 * @return 0 表示成功, -1 表示失败
 */
int bayer16_to_rgb(void *cam_bayer_buf, camera_pixel_fmt cam_fmt, int cam_bytes_per_line,
                   void *fb_rgb_buf, enum fb_fmt fb_fmt, int fb_bytes_per_line,
                   int width, int height);

#ifdef  __cplusplus
}
#endif

#endif /* __BAYER16_TO_RGB__ */