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



IMPF_MediaManager* mediaManager;
IMPF_VideoHandler* videoHandler;

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
                .pixFmt = PIX_FMT_RAW,
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
};


void frameProcess(IMPFrameInfo *frame, void *usrData)
{
    char fileName[64] = "/tmp/frame.raw";
    static uint32_t frameNum = 0;

    frameNum++;
    if (frameNum % 100 != 0)
        return;

    fwriteFileByName(fileName, (void *)frame->virAddr, frame->size);
}

int main(void)
{
    int ret;
    memcpy(videoSets.sensorInfo.name, SENSOR_NAME, sizeof(SENSOR_NAME));
    memcpy(videoSets.sensorInfo.i2c.type, SENSOR_NAME, sizeof(SENSOR_NAME));

    mediaManager = IMPF_GetMediaManager();
    videoHandler = mediaManager->getVideoHandler();

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

    IMPF_FSChn *FSChn = &videoSets.videoStreamList[0].FSChn;
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

    return 0;
}
