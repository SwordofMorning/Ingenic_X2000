/*
 * media_manager.h
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __VIDEO_BASE_H__
#define __VIDEO_BASE_H__


#include <imp/constraints.h>
#include <imp/imp_common.h>
#include <imp/imp_system.h>
#include <imp/imp_log.h>
#include <imp/imp_framesource.h>
#include <imp/imp_encoder.h>
#include <imp/imp_decoder.h>

#include <impf/video_channel.h>




#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */


typedef enum {
    MEM_ATTR_NONE,
    MEM_ATTR_DIRTY,
    MEM_ATTR_CACHED,
} IMPMemAttr;

typedef struct {
    char famliy[MAX_NAME_LEN];
    char mode[MAX_NAME_LEN];
    char owner[MAX_NAME_LEN];
    uint32_t vaddr;
    uint32_t paddr;
    int length;
    int ref_cnt;
    IMPMemAttr mem_attr;
} IMPAllocInfo;

typedef struct {
    char dev_name[MAX_NAME_LEN];
    IMPAllocInfo info;
} IMPAlloc;




/**
 * @fn uint16_t *IMPF_Soft_NV12ScaleAlloc(void);
 *
 * 软件NV12缩放处理内存申请，申请约50KB内存
 *
 * @param 无
 *
 * @retval 申请的内存地址
 *
 * @remark 无
 *
 * @attention 无
 */
uint16_t *IMPF_Soft_NV12ScaleAlloc(void);


/**
 * @fn void IMPF_Soft_NV12ScaleFree(uint16_t *scaleBuf);
 *
 * 软件NV12缩放处理内存释放
 *
 * @param[out] scaleBuf  缩放buf
 *
 * @retval 无
 *
 * @remark 无
 *
 * @attention 无
 */
void IMPF_Soft_NV12ScaleFree(uint16_t *scaleBuf);


/**
 * @fn int IMPF_Soft_NV12ScaleDoing(uint8_t* srcBuf, uint32_t srcWidth, uint32_t srcHeight,
                                    uint8_t* dstBuf, uint32_t dstWidth, uint32_t dstHeight,
                                    uint16_t *scaleBuf);
 *
 * 软件NV12缩放处理
 *
 * @param[in] srcBuf    源数据buf
 * @param[in] srcWidth  源图片宽度
 * @param[in] srcHeight 源图片高度
 * @param[out] dstBuf   输出数据buf
 * @param[in] dstWidth  输出图片宽度
 * @param[in] dstHeight 输出图片高度
 * @param[in] scaleBuf  缩放处理buf
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 如果有异步操作，需要申请多个scaleBuf
 */
int IMPF_Soft_NV12ScaleDoing(uint8_t* srcBuf, uint32_t srcWidth, uint32_t srcHeight,
                                    uint8_t* dstBuf, uint32_t dstWidth, uint32_t dstHeight,
                                    uint16_t *scaleBuf);


/**
 * @fn int IMPF_Soft_YUYVToY(uint8_t *srcBuf, uint8_t *dstBuf, uint32_t width, uint32_t height);
 *
 * 软件yuv422提取y
 *
 * @param[in] srcBuf  源数据buf
 * @param[out] dstBuf 输出数据buf
 * @param[in] width   图片宽度
 * @param[in] height  图片高度
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_Soft_YUYVToY(uint8_t *srcBuf, uint8_t *dstBuf, uint32_t width, uint32_t height);


/**
 * @fn int IMPF_Soft_RAW16ToRAW8(uint8_t *srcBuf, uint8_t *dstBuf, uint32_t width, uint32_t height);
 *
 * 软件raw16转raw8
 *
 * @param[in] srcBuf  源数据buf
 * @param[out] dstBuf 输出数据buf
 * @param[in] width   图片宽度
 * @param[in] height  图片高度
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_Soft_RAW16ToRAW8(uint8_t *srcBuf, uint8_t *dstBuf, uint32_t width, uint32_t height);


/**
 * @fn int IMPF_ALLOC(IMPAlloc *alloc, int size, char *owner);
 *
 * 内存分配，从rmem分配一段连续物理内存
 *
 * @param[in] alloc 内存描述
 * @param[in] size  申请大小
 * @param[in] owner 所有者
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_ALLOC(IMPAlloc *alloc, int size, char *owner);

/**
 * @fn void IMPF_FREE(IMPAlloc *alloc);
 *
 * 内存释放
 *
 * @param[in] alloc
 *
 * @retval 无
 *
 * @remark 无
 *
 * @attention 无
 */
void IMPF_FREE(IMPAlloc *alloc);

/**
 * @fn int IMPF_IPU_NV12ToBGRA(uint8_t* srcPhyAddr, uint8_t* dstPhyAddr,
                                uint32_t width, uint32_t height);
 *
 * IPU颜色空间转换 NV12转BGRA
 *
 * @param[in] srcPhyAddr  源数据物理地址
 * @param[out] dstPhyAddr 目的数据物理地址
 * @param[in] width       图片宽度
 * @param[in] height      图片高度
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_IPU_NV12ToBGRA(uint8_t* srcPhyAddr, uint8_t* dstPhyAddr,
                                uint32_t width, uint32_t height);

/**
 * @fn int IMPF_IPU_NV12ToHSV(uint8_t* srcPhyAddr, uint8_t* dstPhyAddr,
                                uint32_t width, uint32_t height);
 *
 * IPU颜色空间转换 NV12转HSV
 *
 * @param[in] srcPhyAddr  源数据物理地址
 * @param[out] dstPhyAddr 目的数据物理地址
 * @param[in] width       图片宽度
 * @param[in] height      图片高度
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_IPU_NV12ToHSV(uint8_t* srcPhyAddr, uint8_t* dstPhyAddr,
                                uint32_t width, uint32_t height);

/**
 * @fn void IMPF_Soft_NV12AlignMove(IMPFrameInfo *frame);
 *
 * NV12数据对齐，将数据按分辨率高进行16对齐拷贝
 *
 * @param[in] frame  帧数据
 *
 * @retval  无
 *
 * @remark 无
 *
 * @attention 无
 */
void IMPF_Soft_NV12AlignMove(IMPFrameInfo *frame);

/**
 * @fn int IMPF_VPU_JpegEncInit(IMPF_FuncPipe_EncChn *chn);
 *
 * VPU jpeg编码初始化
 *
 * @param[in] chn 编码通道
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 仅支持NV12编码
 */
int IMPF_VPU_JpegEncInit(IMPF_FuncPipe_EncChn *chn);

/**
 * @fn int IMPF_VPU_JpegEncDeinit(IMPF_FuncPipe_EncChn *chn);
 *
 * VPU jpeg编码去初始化
 *
 * @param[in] chn 编码通道
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_VPU_JpegEncDeinit(IMPF_FuncPipe_EncChn *chn);

/**
 * @fn int IMPF_VPU_JpegEncDoing(IMPF_FuncPipe_EncChn *chn, IMPFrameInfo *frame, IMPEncoderStream *stream);
 *
 * VPU jpeg编码处理
 *
 * @param[in] chn     编码通道
 * @param[in] frame   需编码的帧数据
 * @param[out] stream 编码后的流数据
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 要编码的NV12数据宽高需要16对齐
 */
int IMPF_VPU_JpegEncDoing(IMPF_FuncPipe_EncChn *chn, IMPFrameInfo *frame, IMPEncoderStream *stream);

/**
 * @fn int IMPF_VPU_JpegEncRelease(IMPF_FuncPipe_EncChn *chn, IMPEncoderStream *stream);
 *
 * VPU jpeg编码数据释放
 *
 * @param[in] chn     编码通道
 * @param[in] stream  需释放的流数据
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_VPU_JpegEncRelease(IMPF_FuncPipe_EncChn *chn, IMPEncoderStream *stream);

/**
 * @fn int IMPF_VPU_JpegDecInit(IMPDecoderCHNAttr *attr);
 *
 * VPU jpeg解码初始化
 *
 * @param[in] attr 解码通道属性
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 支持jpeg解码为NV12,NV21,I420
 */
int IMPF_VPU_JpegDecInit(IMPDecoderCHNAttr *attr);

/**
 * @fn int IMPF_VPU_JpegDecDeinit(void);
 *
 * VPU jpeg解码去初始化
 *
 * @param 无
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_VPU_JpegDecDeinit(void);

/**
 * @fn int IMPF_VPU_JpegDecStart(void);
 *
 * VPU jpeg解码开始
 *
 * @param 无
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_VPU_JpegDecStart(void);

/**
 * @fn int IMPF_VPU_JpegDecStop(void);
 *
 * VPU jpeg解码停止
 *
 * @param 无
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_VPU_JpegDecStop(void);

/**
 * @fn int IMPF_VPU_JpegDecInputStream(IMPDecoderStream *stream);
 *
 * VPU jpeg解码输入流数据
 *
 * @param[in] stream 需解码的数据流结构体
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_VPU_JpegDecInputStream(IMPDecoderStream *stream);

/**
 * @fn int IMPF_VPU_JpegDecOutputGetFrame(IMPFrameInfo **frame);
 *
 * VPU jpeg解码输出获取解码帧数据
 *
 * @param[out] frame 解码帧数据结构体指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 解码码流buffer由解码器内部申请，该函数只需要传入结构体指针即可
 *            该接口为阻塞接口
 */
int IMPF_VPU_JpegDecOutputGetFrame(IMPFrameInfo **frame);

/**
 * @fn int IMPF_VPU_JpegDecOutputReleaseFrame(IMPFrameInfo *frame);
 *
 * VPU jpeg解码输出释放解码帧数据
 *
 * @param[in] frame 解码帧数据结构体指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int IMPF_VPU_JpegDecOutputReleaseFrame(IMPFrameInfo *frame);

/**
 * @fn int IMPF_VPU_JpegDecDoing(IMPDecoderStream *stream, IMPFrameInfo **frame);
 *
 * VPU jpeg解码处理
 *
 * @param[in] stream  需解码的流数据
 * @param[out] frame  编码后的帧数据
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 该接口是对IMPF_VPU_JpegDecStart,IMPF_VPU_JpegDecStop,IMPF_VPU_JpegDecInputStream,
 *            IMPF_VPU_JpegDecOutputGetFrame的封装.
 */
int IMPF_VPU_JpegDecDoing(IMPDecoderStream *stream, IMPFrameInfo **frame);




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif
