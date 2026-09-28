/*
*  Copyright (C) 2018, kshen <kai.shen@ingenic.com>
*
*  Ingenic IMPF project
*
*  This program is free software; you can redistribute it and/or modify it
*  under  the terms of the GNU General  Public License as published by the
*  Free Software Foundation;  either version 2 of the License, or (at your
*  option) any later version.
*
*  You should have received a copy of the GNU General Public License along
*  with this program; if not, write to the Free Software Foundation, Inc.,
*  675 Mass Ave, Cambridge, MA 02139, USA.
*
*/

#ifndef __V4L2_HANDLER_H__
#define __V4L2_HANDLER_H__


#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */



/**
 * 白平衡参数
 */
typedef struct {
    int mode;                           /* 白平衡模式,1:自动; 0:手动 */
    int temperature;                    /* 色温 */
}IMPF_V4L2VideoWB;

/**
 * 曝光参数
 */
typedef struct {
    int mode;                           /* 曝光模式 */
    int time;                           /* 曝光时间 */
}IMPF_V4L2VideoExpr;

/*
 * V4L2 帧信息
 */
typedef struct {
    uint32_t index;                     /* 缓存编号 */

    uint32_t width;                     /* 帧宽 */
    uint32_t height;                    /* 帧高 */
    uint32_t pixfmt;                    /* 帧的图像格式 */
    uint32_t size;                      /* 帧所占用空间大小 */

    uint32_t virAddr;                   /* 帧的虚拟地址 */
    uint32_t phyAddr;                   /* 帧的物理地址 */

    int64_t timeStamp;                  /* 帧的时间戳 */
} IMPF_V4L2VideoFrameInfo;

/*
 * V4L2 图像格式
 */
typedef struct {
    uint32_t width;                     /* 图像宽度 */
    uint32_t height;                    /* 图像高度 */
    uint32_t pixfmt;                    /* 图像像素格式 */
    uint32_t size;                      /* 图像大小 */
} IMPF_V4L2VideoFormat;

/*
 * V4L2 video通道
 */
typedef struct {
    char *dev;                          /* 设备名 */
    uint8_t product[32];                /* 产品名 */
    int fd;                             /* 设备句柄 */

    IMPF_V4L2VideoFormat fmt;           /* 图像格式 */
    uint32_t fps;                       /* 帧率 */
    uint32_t nBuf;                      /* buf数量 */

    void *pri;                          /* 私有数据，系统内部使用，禁止改动 */
} IMPF_V4L2VideoChn;




typedef struct {
    int (*init)(IMPF_V4L2VideoChn *chn);                                               /* 初始化 */
    int (*deinit)(IMPF_V4L2VideoChn *chn);                                             /* 去初始化 */
    int (*start)(IMPF_V4L2VideoChn *chn);                                              /* 开流 */
    int (*stop)(IMPF_V4L2VideoChn *chn);                                               /* 关流 */

    int (*setFrameDepth)(IMPF_V4L2VideoChn *chn, uint8_t depth);                             /* 设置可获取的图像深度值，depth：>0申请，=0释放 */
    uint8_t (*getFrameDepth)(IMPF_V4L2VideoChn *chn);                                        /* 获取可获取的图像深度值 */
    int (*getFrame)(IMPF_V4L2VideoChn *chn, IMPF_V4L2VideoFrameInfo **frame, int timeoutMS); /* 获取图像，timeoutMS：>0等待超时，=0不等待，<0一直等待；返回0获取成功，返回-1获取失败 */
    int (*releaseFrame)(IMPF_V4L2VideoChn *chn, IMPF_V4L2VideoFrameInfo *frame);             /* 释放图像 */

    int (*getProduct)(IMPF_V4L2VideoChn *chn, char *product);                          /* 获取产品名称 */
    int (*menuFmt)(IMPF_V4L2VideoChn *chn);                                            /* 枚举格式，打印帧格式，分辨率，帧率 */
    void (*queryCtrl)(IMPF_V4L2VideoChn *chn);                                         /* 查询控制，打印可以控制的命令的id,名称，类型，最小最大值，步进，默认值 */
    int (*getFPS)(IMPF_V4L2VideoChn *chn, uint32_t *fps);                              /* 获取帧率 */
    int (*setFPS)(IMPF_V4L2VideoChn *chn, uint32_t fps);                               /* 设置帧率 */
    int (*getBrightness)(IMPF_V4L2VideoChn *chn, int *val);                            /* 获取亮度 */
    int (*setBrightness)(IMPF_V4L2VideoChn *chn, int val);                             /* 设置亮度 */
    int (*getContrast)(IMPF_V4L2VideoChn *chn, int *val);                              /* 获取对比度 */
    int (*setContrast)(IMPF_V4L2VideoChn *chn, int val);                               /* 设置对比度 */
    int (*getSaturation)(IMPF_V4L2VideoChn *chn, int *val);                            /* 获取饱和度 */
    int (*setSaturation)(IMPF_V4L2VideoChn *chn, int val);                             /* 设置饱和度 */
    int (*getSharpness)(IMPF_V4L2VideoChn *chn, int *val);                             /* 获取锐度 */
    int (*setSharpness)(IMPF_V4L2VideoChn *chn, int val);                              /* 设置锐度 */
    int (*getHue)(IMPF_V4L2VideoChn *chn, int *val);                                   /* 获取色调 */
    int (*setHue)(IMPF_V4L2VideoChn *chn, int val);                                    /* 设置色调 */
    int (*getGamma)(IMPF_V4L2VideoChn *chn, int *val);                                 /* 获取伽玛 */
    int (*setGamma)(IMPF_V4L2VideoChn *chn, int val);                                  /* 设置伽玛 */
    int (*getWB)(IMPF_V4L2VideoChn *chn, IMPF_V4L2VideoWB *WB);                        /* 获取白平衡 */
    int (*setWB)(IMPF_V4L2VideoChn *chn, IMPF_V4L2VideoWB *WB);                        /* 设置白平衡 */
    int (*getGain)(IMPF_V4L2VideoChn *chn, int *val);                                  /* 获取增益 */
    int (*setGain)(IMPF_V4L2VideoChn *chn, int val);                                   /* 设置增益 */
    int (*setHFlip)(IMPF_V4L2VideoChn *chn, bool enable);                              /* 设置水平翻转 */
    int (*setVFlip)(IMPF_V4L2VideoChn *chn, bool enable);                              /* 设置垂直翻转 */
    int (*setAntiFlicker)(IMPF_V4L2VideoChn *chn, enum v4l2_power_line_frequency val); /* 设置抗闪烁 */
    int (*setBackLightComp)(IMPF_V4L2VideoChn *chn, int val);                          /* 设置逆光补偿 */
} IMPF_V4L2VideoHandler;


IMPF_V4L2VideoHandler* IMPF_GetV4L2VideoHandler(void);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif
