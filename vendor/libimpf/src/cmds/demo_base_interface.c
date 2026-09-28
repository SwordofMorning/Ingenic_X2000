/*
*  Copyright (C) 2018, <kai.shen@ingenic.com>
*
*  Ingenic IMP samples project
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
#include <impf/media_manager.h>

#include "sensor_info.h"
#include "video_common.h"
#include "common.h"


#define LOG_TAG                         "IMPF_DEMO"
#define DISPLAY_EN                      0
#define JPEGQL_SET                      1

#define LCD_PREVIEW_WIDTH               SENSOR_WIDTH_SECOND
#define LCD_PREVIEW_HEIGHT              SENSOR_HEIGHT_SECOND


IMPF_MediaManager* mediaManager;
IMPF_VideoHandler* videoHandler;
IMPF_FBHandler* FBHandler;
IMPAlloc NV12Buf;
IMPAlloc CSCBuf;
rectRgn LCDPreviewRgn = {0, 0, LCD_PREVIEW_WIDTH, LCD_PREVIEW_HEIGHT};


IMPF_VideoSets videoSets = {
    .sensorInfo = {
        .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
        .i2c = {
            .addr = SENSOR_I2C_ADDR,
            .i2c_adapter_id = SENSOR_I2C_ID,
        },
    },

    .videoStreamList[0] = {
        .enable = 1,
        .FSChn = {
            .chnType = FS_PHY_MAIN_CHN,
            .FSChnAttr = {
                .picWidth = SENSOR_WIDTH,
                .picHeight = SENSOR_HEIGHT,
                .pixFmt = PIX_FMT_NV12,
                .outFrmRateNum = 30,
                .outFrmRateDen = 1,
                .nrVBs = 2,

                .crop.enable = 0,
                .crop.top = 0,
                .crop.left = 0,
                .crop.width = SENSOR_WIDTH,
                .crop.height = SENSOR_HEIGHT,

                .scaler.enable = 1,
                .scaler.outwidth = SENSOR_WIDTH,
                .scaler.outheight = SENSOR_HEIGHT,
            },
        },

        .funcPipeNum = 0,
    },

    .videoStreamList[1] = {
        .enable = 1,
        .FSChn = {
            .chnType = FS_PHY_SECOND_CHN,
            .FSChnAttr = {
                .picWidth = SENSOR_WIDTH,
                .picHeight = SENSOR_HEIGHT,
                .pixFmt = PIX_FMT_NV12,
                .outFrmRateNum = 30,
                .outFrmRateDen = 1,
                .nrVBs = 2,

                .crop.enable = 1,
                .crop.top = 0,
                .crop.left = 0,
                .crop.width = SENSOR_WIDTH,
                .crop.height = SENSOR_HEIGHT,

                .scaler.enable = 0,
                .scaler.outwidth = SENSOR_WIDTH,
                .scaler.outheight = SENSOR_HEIGHT,
            },
        },

        .funcPipeNum = 0,
    },
};

IMPF_FuncPipe_EncChn encChn = {
    .encChnAttr = {
        .encAttr = {
            .enType = PT_JPEG,
            .bufSize = 0,
            .profile = 1,
            .picWidth = SENSOR_WIDTH,
            .picHeight = SENSOR_HEIGHT,
        },
    },
};


void frameProcess0(IMPFrameInfo *frame, void *usrData)
{
    IMPF_Soft_NV12ScaleDoing((uint8_t *)frame->virAddr, frame->width, frame->height,
                            (uint8_t *)NV12Buf.info.vaddr, LCD_PREVIEW_WIDTH, LCD_PREVIEW_HEIGHT,
                            (uint16_t *)usrData);

    IMPF_IPU_NV12ToBGRA((uint8_t *)NV12Buf.info.paddr, (uint8_t *)CSCBuf.info.paddr, LCD_PREVIEW_WIDTH, LCD_PREVIEW_HEIGHT);
#if DISPLAY_EN
    rectRgnCopyToDst((uint8_t *)CSCBuf.info.vaddr, LCD_PREVIEW_WIDTH, LCD_PREVIEW_HEIGHT,
                  FBHandler->getFBMem(), FBHandler->getScreenWidth(), FBHandler->getScreenHeight(),
                  FBHandler->getBitsPerPixel(), LCDPreviewRgn);
    FBHandler->display();
#else
    char fileName[64] = "/tmp/frame.bgra";
    static uint32_t frameNum = 0;

    frameNum++;
    if (frameNum % 100 != 0)
        return;

    fwriteFileByName(fileName, (void *)CSCBuf.info.vaddr, calc_pic_size(LCD_PREVIEW_WIDTH, SENSOR_HEIGHT_SECOND, PIX_FMT_BGRA));
#endif
}


void frameProcess1(IMPFrameInfo *frame, void *usrData)
{
    char fileName[64] = {0};
    static uint32_t frameNum = 0;
    IMPEncoderStream stream;

    frameNum++;
    if (frameNum % 3 != 0)
        return;

    sprintf(fileName, "/tmp/frame.nv12");
    fwriteFileByName(fileName, (void *)frame->virAddr, frame->size);

    IMPF_Soft_NV12AlignMove(frame);
    if (IMPF_VPU_JpegEncDoing(&encChn, frame, &stream) != 0)
        return;

    sprintf(fileName, "/tmp/stream.jpeg");
    writeStreamByName(fileName, &stream);

    IMPF_VPU_JpegEncRelease(&encChn, &stream);
}


int main(void)
{
    int ret, retval = 0;
    uint16_t *scaleBuf;
    memcpy(videoSets.sensorInfo.name, SENSOR_NAME, sizeof(SENSOR_NAME));
    memcpy(videoSets.sensorInfo.i2c.type, SENSOR_NAME, sizeof(SENSOR_NAME));

    mediaManager = IMPF_GetMediaManager();
    videoHandler = mediaManager->getVideoHandler();
#if DISPLAY_EN
    FBHandler = mediaManager->getFBHandler();
#endif

#if DISPLAY_EN
    ret = FBHandler->init();
    if(ret < 0) {
        printf("FBHandler->init fail!\n");
        return -1;
    }
#endif

    ret = videoHandler->init(&videoSets);
    if(ret < 0) {
        printf("videoHandler->init fail!\n");
        retval = -1;
        goto End0;
    }

    ret = videoHandler->start();
    if(ret < 0) {
        printf("videoHandler->start fail!\n");
        retval = -1;
        goto End1;
    }

    ret = IMPF_VPU_JpegEncInit(&encChn);
    if(ret < 0) {
        printf("IMPF_VPU_JpegEncInit failed!\n");
        retval = -1;
        goto End2;
    }
#if JPEGQL_SET
    IMPEncoderJpegeQl jpegeQl;
    IMP_Encoder_GetJpegeQl(encChn.chnIndex, &jpegeQl);
    MakeTables(64, &(jpegeQl.qmem_table[0]), &(jpegeQl.qmem_table[64]));
    jpegeQl.user_ql_en = 1;
    IMP_Encoder_SetJpegeQl(encChn.chnIndex, &jpegeQl);
#endif

    scaleBuf = IMPF_Soft_NV12ScaleAlloc();
    if (NULL == scaleBuf) {
        printf("IMPF_Soft_NV12ScaleAlloc failed!\n");
        retval = -1;
        goto End3;
    }
    ret = IMPF_ALLOC(&NV12Buf, calc_pic_size(LCD_PREVIEW_WIDTH, LCD_PREVIEW_HEIGHT, PIX_FMT_NV12), "NV12 Buf");
    if(ret < 0) {
        printf("IMPF_ALLOC NV12Buf failed!\n");
        retval = -1;
        goto End4;
    }
    ret = IMPF_ALLOC(&CSCBuf, calc_pic_size(LCD_PREVIEW_WIDTH, SENSOR_HEIGHT_SECOND, PIX_FMT_BGRA), "CSC Buf");
    if(ret < 0) {
        printf("IMPF_ALLOC CSCBuf failed!\n");
        retval = -1;
        goto End5;
    }

    IMPF_FSChn *FSChn0 = &videoSets.videoStreamList[0].FSChn;
    if(FSChn0->interface != NULL) {
        FSChn0->interface->startFrameProcess(FSChn0, frameProcess0, (void *)scaleBuf);
    }

    IMPF_FSChn *FSChn1 = &videoSets.videoStreamList[1].FSChn;
    if(FSChn1->interface != NULL) {
        ret = FSChn1->interface->setFrameDepth(FSChn1, 1);
        if (ret < 0) {
            printf("setFrameDepth failed!\n");
            retval = -1;
            goto End6;
        }
    }
    while(1) {
        IMPFrameInfo *frame;
        if(FSChn1->interface != NULL) {
            if (FSChn1->interface->getFrame(FSChn1, &frame, 2*1000) != 0)
                continue;
            frameProcess1(frame, NULL);
            FSChn1->interface->releaseFrame(FSChn1, frame);
        }
        sleep(1);
    }

End6:
    IMPF_FREE(&CSCBuf);
End5:
    IMPF_FREE(&NV12Buf);
End4:
    IMPF_Soft_NV12ScaleFree(scaleBuf);
End3:
    IMPF_VPU_JpegEncDeinit(&encChn);
End2:
    ret = videoHandler->stop();
    if(ret < 0) {
        printf("videoHandler->stop fail!\n");
    }
End1:
    ret = videoHandler->deinit();
    if(ret < 0) {
        printf("videoHandler->deinit fail!\n");
    }
End0:
#if DISPLAY_EN
    ret = FBHandler->deinit();
    if(ret < 0) {
        printf("FBHandler->deinit fail!\n");
    }
#endif

    return retval;
}
