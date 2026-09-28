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

/*
思想：提供一个个不同的功能单元,再将这些单元按照一定顺序排序后绑定,让视频流从一个单元流向下一个单元,得到想要的数据。此为一路视频流。


重要概念：
    stream: 视频流
        一路完整的视频流，主要描述了：视频源头具有什么属性，中间经过哪些处理，得到什么样的数据。是几个pipe组合的结果，每路视频流至少要有FS，可以有一个或多个功能管道。

    pipe: 管道
        一类功能的容器。chn会注册到pipe中，使pipe具有处理几个同种功能的能力，即可以在同一个管道内完成一个或多个同种功能。
        功能单元：
            FS : 帧源（FS chn实际上就是FS pipe）
            OSD: 视屏叠加
            Enc: 编码
            IVS: 智能分析

    chn: 通道（rgn：区域，与chn属同一级）
        实现某一类功能的对象，具有属性和方法。

    stream,pipe,chn之间的关系:
        ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
        ┃ stream                            ┃
        ┃  ┏━━━━━━━━━━━┓      ┏━━━━━━━━━━━┓ ┃
        ┃  ┃ pipe0     ┃      ┃ pipe1     ┃ ┃
        ┃  ┃           ┃      ┃  ┏━━━━━━┓ ┃ ┃
        ┃  ┃  ┏━━━━━━┓ ┃      ┃  ┃ chn0 ┃ ┃ ┃
        ┃  ┃  ┃ chn0 ┃ ┣━━━━━━┫  ┗━━━━━━┛ ┃ ┃
        ┃  ┃  ┗━━━━━━┛ ┣━━━━━━┫  ┏━━━━━━┓ ┃ ┃
        ┃  ┃           ┃      ┃  ┃ chn1 ┃ ┃ ┃
        ┃  ┃           ┃      ┃  ┗━━━━━━┛ ┃ ┃
        ┃  ┗━━━━━━━━━━━┛      ┗━━━━━━━━━━━┛ ┃
        ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
*/

#ifndef __VIDEO_CHANNEL_H__
#define __VIDEO_CHANNEL_H__


#include <imp/constraints.h>
#include <imp/imp_common.h>
#include <imp/imp_system.h>
#include <imp/imp_log.h>
#include <imp/imp_isp.h>
#include <imp/imp_framesource.h>
#include <imp/imp_osd.h>
#include <imp/imp_encoder.h>
#include <imp/imp_ivs.h>


#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */



#define MAX_FUNCPIPE_NUM                3                               /* 每条视频流的最大功能管道数量      */



/**
 * 状态
 */
typedef enum {
	STATE_CLOSE                         = 0,
	STATE_OPEN,
	STATE_RUN,
} IMPF_State;

/**
 * FS通道类型
 */
typedef enum {
    FS_PHY_MAIN_CHN                     = 0,                            /* 主物理通道，满分辨率，支持NV12,RAW        */
    FS_PHY_SECOND_CHN                   = 1,                            /* 次物理通道，缩减分辨率，支持NV12*/
    FS_EXT_CHN                          = 2,                            /* 扩展通道，图像拷贝于主物理通道，支持NV12,BGRA,HSV,YUYV422 */
} IMPF_FS_ChnType;

/**
 * 功能管道类型
 */
typedef enum {
    //FS = 0,
    FuncPipe_OSD                        = 1,                            /* 功能管道OSD */
    FuncPipe_ENC                        = 2,                            /* 功能管道ENC */
    FuncPipe_IVS                        = 3,                            /* 功能管道IVS */
} IMPF_FuncPipeType;


/*
 * FS frame 输出回调函数
 */
typedef void (*FSFrameCallBack)(IMPFrameInfo *frame, void *usrData);

/**
 * FS通道的方法(开放接口)
 */
typedef struct {
    pthread_t ptID;
    bool ptRunFlag;
    bool ptEndFlag;
    FSFrameCallBack callBack;                                                       /* FS frame输出回调 */
    void *usrData;                                                                  /* 存储FS frame输出回调的用户数据指针 */
    int (*startFrameProcess)(void *FSChn, FSFrameCallBack callBack, void *usrData); /* 开始FS帧处理，会创建线程：获取图像->回调->释放图像。每个interface有且只有一个线程在运行 */
    int (*stopFrameProcess)(void *FSChn);                                           /* 停止FS帧处理 */
    /* 注意上下两种接口不能混着用 */
    int (*setFrameDepth)(void *FSChn, int depth);                                   /* 设置可获取的图像深度值，depth：>0申请，=0释放 */
    int (*getFrameDepth)(void *FSChn, int *depth);                                  /* 获取可获取的图像深度值 */
    int (*getFrame)(void *FSChn, IMPFrameInfo **frame, int timeoutMS);              /* 获取图像，timeoutMS：>0等待超时，=0不等待，<0一直等待；返回0获取成功，返回-1获取失败 */
    int (*releaseFrame)(void *FSChn, IMPFrameInfo *frame);                          /* 释放图像 */

    void *pri;                                                                      /* 私有数据，系统内部使用 */
} IMPF_FSChnInterface;

/**
 * FS通道
 */
typedef struct FSChn {
    uint32_t chnIndex;                                                  /* 通道索引(ID),系统按顺序分配 */
    IMPCell cell;                                                       /* 用来bind FS和各个功能管道，使数据自动从源流向目的，系统自动初始化 */

    IMPF_FS_ChnType chnType;                                            /* FS通道类型*/
    IMPFSChnAttr FSChnAttr;                                             /* FS通道属性 */
    //struct FSChn *extChnParent;

    IMPF_FSChnInterface *interface;                                     /* FS通道的方法 */

    void *pri;                                                          /* 私有数据，系统内部使用 */
} IMPF_FSChn;


/**
 * 功能管道模板,方便bind,  其他funcPipe的也要参照该结构体写,已有的成员类型和偏移不能改
 */
typedef struct {
    uint32_t pipeID;                                                    /* 管道ID,系统自动分配 */
    IMPCell cell;                                                       /* 用来bind FS和各个功能管道，使数据自动从源流向目的，系统自动初始化 */

    int chnNum;
} IMPF_FuncPipe_Template;


/**
 * OSD区域的方法(开放接口)
 */
typedef struct {
    int (*getRgnAttr)(void *OSDRgn, IMPOSDRgnAttr *rgnAttr);            /* 获取区域属性 */
    int (*setRgnAttr)(void *OSDRgn, IMPOSDRgnAttr *rgnAttr);            /* 设置区域属性 */
    int (*getGrpRgnAttr)(void *OSDRgn, IMPOSDGrpRgnAttr *grpRgnAttr);   /* 获取OSD组区域属性  */
    int (*setGrpRgnAttr)(void *OSDRgn, IMPOSDGrpRgnAttr *grpRgnAttr);   /* 设置OSD组区域属性 */
    int (*isRgnShow)(void *OSDRgn, int isShow);                         /* 设置组区域是否显示 */
    int (*updateRgnDate)(void *OSDRgn, uint32_t *data);                 /* 更新区域数据,只针对OSD_REG_BITMAP和OSD_REG_PIC的区域类型 */

    void *pri;                                                          /* 私有数据，系统内部使用 */
} IMPF_OSDRgnInterface;

/**
 * OSD区域
 */
typedef struct {
    uint32_t rgnIndex;                                                  /* 区域索引(ID),系统自动分配 */

    IMPRgnHandle rgnHandle;                                             /* OSD区域句柄 */
    IMPOSDRgnAttr rgnAttr;                                              /* OSD区域属性 */
    IMPOSDGrpRgnAttr grpRgnAttr;                                        /* OSD组区域属性 */

    IMPF_OSDRgnInterface *interface;                                    /* OSD区域的方法 */

    void *pri;                                                          /* 私有数据，系统内部使用 */
} IMPF_FuncPipe_OSDRgn;

/**
 * OSD功能管道
 */
typedef struct {
    uint32_t pipeID;                                                    /* 管道ID,系统自动分配 */
    IMPCell cell;                                                       /* 用来bind FS和各个功能管道，使数据自动从源流向目的，系统自动初始化 */

    int OSDRgnNum;                                                      /* OSD管道注册的区域数量 */
    IMPF_FuncPipe_OSDRgn OSDRgnList[NR_MAX_OSD_OUTPUT_IN_GROUP];        /* OSD管道注册的区域列表 */

    void *pri;                                                          /* 私有数据，系统内部使用 */
} IMPF_FuncPipe_OSD;


/**
 * 编码通道的方法(开放接口)
 */
typedef struct {
    int (*startEncode)(void *EncChn);                                            /* 开始编码 */
    int (*stopEncode)(void *EncChn);                                             /* 停止编码 */
    int (*getStream)(void *EncChn, IMPEncoderStream *stream, uint32_t timeoutMS);/* 获取码流，timeoutMS：>0等待超时，=0一直等待；返回0获取成功，返回-1获取失败 */
    int (*releaseStream)(void *EncChn, IMPEncoderStream *stream);                /* 释放码流 */
    int (*requestIDR)(void *EncChn);                                             /* 请求IDR帧,会在最近的编码帧申请IDR帧编码,只适用于H264和h265编码channel */
    int (*resetRcAttr)(void *EncChn, IMPEncoderRcAttr *rcAttr);                  /* 重置码率控制器属性,只适用于H264和h265编码channel */
    int (*insertUserDate)(void *EncChn, void *userData, uint32_t userDataLen);   /* 插入用户数据,只适用于H264和h265编码channel */
    /* 其他编码API可以直接调用IMP_Encoder_*(EncChn->chnIndex) */

    void *pri;                                                                   /* 私有数据，系统内部使用 */
} IMPF_EncChnInterface;

/**
 * 编码通道
 */
typedef struct {
    uint32_t chnIndex;                                                  /* 通道索引(ID),系统自动分配 */

    IMPEncoderCHNAttr encChnAttr;                                       /* 编码器通道属性 */

    IMPF_EncChnInterface *interface;                                    /* 编码通道的方法 */

    void *pri;                                                          /* 私有数据，系统内部使用 */
} IMPF_FuncPipe_EncChn;

/**
 * 编码功能管道
 */
typedef struct {
    uint32_t pipeID;                                                    /* 管道ID,系统自动分配 */
    IMPCell cell;                                                       /* 用来bind FS和各个功能管道，使数据自动从源流向目的，系统自动初始化 */

    int encChnNum;                                                      /* 编码管道注册的通道数量 */
    IMPF_FuncPipe_EncChn encChnList[NR_MAX_ENC_CHN_IN_GROUP];           /* 编码管道注册的通道列表 */

    void *pri;                                                          /* 私有数据，系统内部使用 */
} IMPF_FuncPipe_Enc;


/**
 * IVS通道的方法(开放接口)
 */
typedef struct {
    int (*start)(void *IVSChn);                                         /* 开始智能分析 */
    int (*stop)(void *IVSChn);                                          /* 停止智能分析 */
    int (*getResult)(void *IVSChn, void **result, int timeoutMS);       /* 获取分析结果 */
    int (*releaseResult)(void *IVSChn, void *result);                   /* 释放分析结果 */
    int (*getParam)(void *IVSChn, void *param);                         /* 获取算法参数 */
    int (*setParam)(void *IVSChn, void *param);                         /* 设置算法参数 */

    void *pri;                                                          /* 私有数据，系统内部使用 */
} IMPF_IVSChnInterface;

/**
 * IVS通道
 */
typedef struct {
    uint32_t chnIndex;                                                  /* 通道索引(ID),系统自动分配 */

    IMPIVSInterface IVSInterface;                                       /* IVS通用接口    */

    IMPF_IVSChnInterface *interface;                                    /* IVS通道的方法 */

    void *pri;
} IMPF_FuncPipe_IVSChn;

/**
 * IVS功能管道
 */
typedef struct {
    uint32_t pipeID;                                                    /* 管道ID,系统自动分配 */
    IMPCell cell;                                                       /* 用来bind FS和各个功能管道，使数据自动从源流向目的，系统自动初始化 */

    int IVSChnNum;                                                      /* IVS管道注册的通道数量 */
    IMPF_FuncPipe_IVSChn IVSChnList[NR_MAX_IVS_CHN_IN_GROUP];           /* IVS管道注册的通道列表 */

    void *pri;                                                          /* 私有数据，系统内部使用 */
} IMPF_FuncPipe_IVS;


/**
 * 功能管道union
 */
typedef struct {
    IMPF_FuncPipeType funcPipeType;
    union {
        IMPF_FuncPipe_Template funcPipeTemplate;                        /* funcPipe模板,方便bind,  其他funcPipe的也要参照该结构体写,已有的成员类型和偏移不能改 */
        IMPF_FuncPipe_OSD osd;
        IMPF_FuncPipe_Enc enc;
        IMPF_FuncPipe_IVS ivs;
    };
} IMPF_FuncPipe;



/**
 * 视频流,每路视频流至少有FS，可以有一个或多个功能管道
 *      系统会按照FSChn - funcPipeList[0] - funcPipeList[1] ... 按顺序bind，即数据会按此顺序流走
 */
typedef struct {
    bool enable;                                                        /* 视频流创建并使能的标志，=0将不做任何处理 */

    IMPF_FSChn FSChn;

    int funcPipeNum;                                                    /* 添加使用的功能管道数量 */
    IMPF_FuncPipe funcPipeList[MAX_FUNCPIPE_NUM];                       /* 添加使用的功能管道列表 */
} IMPF_VideoStream;

/**
 * 视频流集,视频流的总和
 */
typedef struct {
    IMPSensorInfo sensorInfo;                                           /* 摄像头注册信息 */
    IMPF_VideoStream videoStreamList[NR_MAX_FS_CHNS];                   /* 视频流列表 */
} IMPF_VideoSets;



typedef struct {
    int (*init)(IMPF_VideoSets *videoSets);                             /* 视频流集初始化 */
    int (*deinit)(void);                                                /* 视频流集去初始化 */
    int (*start)(void);                                                 /* 视频流集开流 */
    int (*stop)(void);                                                  /* 视频流集关流，注isp和sensor会关掉 */
    int (*streamOn)(IMPF_VideoStream *videoStream);                     /* 视频流开流 */
    int (*streamOff)(IMPF_VideoStream *videoStream);                    /* 视频流关流 */
    void (*dumpStremTree)(void);                                        /* 打印视频流树 */
} IMPF_VideoHandler;


IMPF_VideoHandler* IMPF_GetVideoHandler(void);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif
