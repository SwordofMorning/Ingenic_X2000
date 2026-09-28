#ifndef _IMAGE_CONVERT_H
#define _IMAGE_CONVERT_H

#include <libswscale/swscale.h>







/**
 * Extern functions
 */

/**
* @fn enum AVPixelFormat imppixfmt_2_avpixfmt(isp_pix_fmt imppixfmt);
*
* @brief  v4l2像素格式转ffmpeg像素格式
*
* @param v4l2pixfmt  v4l2像素格式
*
* @retval ffmpeg像素格式
*/
enum AVPixelFormat v4l2pixfmt_2_avpixfmt(int v4l2pixfmt);


/**
* @fn int image_size(enum AVPixelFormat format, int width, int height);
*
* @brief  通过图像格式宽高计算图像大小
*
* @param format      图片格式
*        width       图片宽度
*        height      图片高度
*
* @retval image size
*/
int image_size(enum AVPixelFormat format, int width, int height);

/**
* @fn int image_convert(uint8_t *src_buf, int src_w, int src_h, enum AVPixelFormat src_format,
                        uint8_t *dst_buf, int dst_w, int dst_h, enum AVPixelFormat dst_format);
*
* @brief  图像转换(格式转换，分辨率转换)
*
* @param src_buf    源数据buf
*        src_w      源图片宽度
*        src_h      源图片高度
*        src_format 源图片格式
*        dst_buf    输出数据buf
*        dst_w      输出图片宽度
*        dst_h      输出图片高度
*        dst_format 输出图片格式
*
* @retval 0 成功
* @retval -1 失败
*/
int image_convert(uint8_t *src_buf, int src_w, int src_h, enum AVPixelFormat src_format,
                        uint8_t *dst_buf, int dst_w, int dst_h, enum AVPixelFormat dst_format);

/**
* @fn int image_zoom(enum AVPixelFormat format,
                    uint8_t *src_buf, int src_w, int src_h,
                    uint8_t *dst_buf, int dst_w, int dst_h);
*
* @brief  图像缩放
*
* @param format     图片格式
*        src_buf    源数据buf
*        src_w      源图片宽度
*        src_h      源图片高度
*        dst_buf    输出数据buf
*        dst_w      输出图片宽度
*        dst_h      输出图片高度
*
* @retval 0 成功
* @retval -1 失败
*/
int image_zoom(enum AVPixelFormat format,
                    uint8_t *src_buf, int src_w, int src_h,
                    uint8_t *dst_buf, int dst_w, int dst_h);

/**
* @fn int raw16_2_raw8(uint8_t* src_buf, uint32_t src_size, uint8_t* dst_buf);
*
* @brief  raw16转raw8
*
* @param src_buf  源数据buf
*        src_size 源图片size
*        dst_buf  输出数据buf
*
* @retval 0 成功
* @retval <0 失败
*
*/
int raw16_2_raw8(uint8_t* src_buf, uint32_t src_size, uint8_t* dst_buf);


#endif
