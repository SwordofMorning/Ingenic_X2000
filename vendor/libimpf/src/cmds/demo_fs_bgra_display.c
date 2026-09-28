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
#include "common.h"


#define LOG_TAG                         "IMPF_DEMO"

#define LCD_PREVIEW_WIDTH               SENSOR_WIDTH_SECOND
#define LCD_PREVIEW_HEIGHT              SENSOR_HEIGHT_SECOND



IMPF_MediaManager* mediaManager;
IMPF_VideoHandler* videoHandler;
IMPF_FBHandler* FBHandler;
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

                .scaler.enable = 0,
                .scaler.outwidth = SENSOR_WIDTH,
                .scaler.outheight = SENSOR_HEIGHT,
            },
        },

        .funcPipeNum = 0,
    },

    .videoStreamList[1] = {
        .enable = 1,
        .FSChn = {
            .chnType = FS_EXT_CHN,
            .FSChnAttr = {
                .picWidth = SENSOR_WIDTH_SECOND,
                .picHeight = SENSOR_HEIGHT_SECOND,
                .pixFmt = PIX_FMT_BGRA,
                .outFrmRateNum = 30,
                .outFrmRateDen = 1,
                .nrVBs = 1,

                .crop.enable = 0,
                .crop.top = 0,
                .crop.left = 0,
                .crop.width = SENSOR_WIDTH_SECOND,
                .crop.height = SENSOR_HEIGHT_SECOND,
            },
        },

        .funcPipeNum = 0,
    },
};



void frameProcess(IMPFrameInfo *frame, void *usrData)
{
    rectRgnCopyToDst((uint8_t *)frame->virAddr, frame->width, frame->height,
                  FBHandler->getFBMem(), FBHandler->getScreenWidth(), FBHandler->getScreenHeight(),
                  FBHandler->getBitsPerPixel(), LCDPreviewRgn);
    FBHandler->display();
}

int main(void)
{
    int ret;
    memcpy(videoSets.sensorInfo.name, SENSOR_NAME, sizeof(SENSOR_NAME));
    memcpy(videoSets.sensorInfo.i2c.type, SENSOR_NAME, sizeof(SENSOR_NAME));

    mediaManager = IMPF_GetMediaManager();
    videoHandler = mediaManager->getVideoHandler();
    FBHandler = mediaManager->getFBHandler();

    ret = FBHandler->init();
    if(ret < 0) {
        printf("FBHandler->init fail!\n");
        return -1;
    }

    ret = videoHandler->init(&videoSets);
    if(ret < 0) {
        printf("videoHandler->init fail!\n");
        return -1;
    }

    ret = videoHandler->start();
    if(ret < 0) {
        printf("videoHandler->start fail!\n");
        videoHandler->deinit();
        return -1;
    }

    IMPF_FSChn *FSChn = &videoSets.videoStreamList[1].FSChn;
    if(FSChn->interface != NULL) {
        FSChn->interface->startFrameProcess(FSChn, frameProcess, NULL);
    }
    while(1) {
        sleep(1);
    }

    ret = videoHandler->stop();
    if(ret < 0) {
        printf("videoHandler->stop fail!\n");
    }

    ret = videoHandler->deinit();
    if(ret < 0) {
        printf("videoHandler->deinit fail!\n");
    }

    ret = FBHandler->deinit();
    if(ret < 0) {
        printf("FBHandler->deinit fail!\n");
    }

    return 0;
}
