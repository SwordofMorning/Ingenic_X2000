/*
 * media_manager.h
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __MEDIA_MANAGER_H__
#define __MEDIA_MANAGER_H__


#include <impf/video_channel.h>
#include <impf/video_base.h>
#include <impf/v4l2_video_handler.h>
#include <impf/fb_handler.h>


#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */


#define IMPF_VERSION                    "20200618"




typedef enum {
    SOC_UNKNOWN = -1,
    SOC_X1520 = 0,
    SOC_X1830,
    SOC_X1630,
    SOC_END,
} IMPF_CPUType;


typedef struct {
    IMPF_VideoHandler *(*getVideoHandler)(void);
    IMPF_V4L2VideoHandler  *(*getV4L2VideoHandler)(void);
    IMPF_FBHandler *(*getFBHandler)(void);
    //IMPF_AudioHandler *(*getAudioHandler)(void);
} IMPF_MediaManager;



int IMPF_CheckVersion(const char *version);

IMPF_CPUType IMPF_GetCPUType(void);

IMPF_MediaManager* IMPF_GetMediaManager(void);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif
