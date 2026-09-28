#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <linux/videodev2.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>

#include "image_convert.h"




enum AVPixelFormat v4l2pixfmt_2_avpixfmt(int v4l2pixfmt)
{
    enum AVPixelFormat avpixfmt;

    switch (v4l2pixfmt) {
    case V4L2_PIX_FMT_GREY:
        avpixfmt = AV_PIX_FMT_GRAY8;
        break;
    case V4L2_PIX_FMT_YUYV:
        avpixfmt = AV_PIX_FMT_YUYV422;
        break;
    case V4L2_PIX_FMT_UYVY:
        avpixfmt = AV_PIX_FMT_UYVY422;
        break;
    case V4L2_PIX_FMT_NV12:
        avpixfmt = AV_PIX_FMT_NV12;
        break;
    case V4L2_PIX_FMT_NV21:
        avpixfmt = AV_PIX_FMT_NV21;
        break;
    case V4L2_PIX_FMT_RGB555:
        avpixfmt = AV_PIX_FMT_RGB555LE;
        break;
    case V4L2_PIX_FMT_RGB565:
        avpixfmt = AV_PIX_FMT_RGB565LE;
        break;
    case V4L2_PIX_FMT_RGB555X:
        avpixfmt = AV_PIX_FMT_RGB555BE;
        break;
    case V4L2_PIX_FMT_RGB565X:
        avpixfmt = AV_PIX_FMT_RGB565BE;
        break;
    case V4L2_PIX_FMT_RGB24:
        avpixfmt = AV_PIX_FMT_RGB24;
        break;
    case V4L2_PIX_FMT_BGR24:
        avpixfmt = AV_PIX_FMT_BGR24;
        break;
    case V4L2_PIX_FMT_RGB32:
        avpixfmt = AV_PIX_FMT_RGBA;
        break;
    case V4L2_PIX_FMT_BGR32:
        avpixfmt = AV_PIX_FMT_BGRA;
        break;
    default:
        avpixfmt = AV_PIX_FMT_NONE;
        break;
    }

    return avpixfmt;
}


int image_size(enum AVPixelFormat format, int width, int height)
{
    return avpicture_get_size(format, width, height);
}


int image_convert(uint8_t *src_buf, int src_w, int src_h, enum AVPixelFormat src_format,
                        uint8_t *dst_buf, int dst_w, int dst_h, enum AVPixelFormat dst_format)
{
    struct SwsContext *img_convert_ctx;
    AVPicture src_picture, dst_picture;

    if (NULL == src_buf || NULL == dst_buf) {
        printf("NULL == src_buf || NULL == dst_buf!\n");
        return -1;
    }

    avpicture_fill(&src_picture, src_buf, src_format, src_w, src_h);
    avpicture_fill(&dst_picture, dst_buf, dst_format, dst_w, dst_h);

    img_convert_ctx = sws_getContext(src_w, src_h, src_format,
                                     dst_w, dst_h, dst_format,
                                     SWS_POINT,
                                     NULL, NULL, NULL);
    if(img_convert_ctx == NULL) {
        printf("Cannot initialize SwsContext!\n");
        return -1;
    }

    sws_scale(img_convert_ctx, src_picture.data, src_picture.linesize,
                               0, src_h,
                               dst_picture.data, dst_picture.linesize);

    sws_freeContext(img_convert_ctx);

    return 0;
}


int image_zoom(enum AVPixelFormat format,
                    uint8_t *src_buf, int src_w, int src_h,
                    uint8_t *dst_buf, int dst_w, int dst_h)
{
    return image_convert(src_buf, src_w, src_h, format,
                         dst_buf, dst_w, dst_h, format);
}


int raw16_2_raw8(uint8_t* src_buf, uint32_t src_size, uint8_t* dst_buf)
{
    int i;

    if (src_buf == NULL || dst_buf == NULL) {
        printf("src_buf == NULL || dst_buf == NULL\n");
        return -1;
    }

    for (i=1; i<src_size; i+=2) {
        *dst_buf++ = src_buf[i];
    }
    return 0;
}

