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
            .chnType = FS_PHY_SECOND_CHN,
            .FSChnAttr = {
                .picWidth = SENSOR_WIDTH_SECOND,
                .picHeight = SENSOR_HEIGHT_SECOND,
                .pixFmt = PIX_FMT_NV12,
                .outFrmRateNum = 20,
                .outFrmRateDen = 1,
                .nrVBs = 2,

                .crop.enable = 0,
                .crop.top = 0,
                .crop.left = 0,
                .crop.width = SENSOR_WIDTH_SECOND,
                .crop.height = SENSOR_HEIGHT_SECOND,

                .scaler.enable = 1,
                .scaler.outwidth = SENSOR_WIDTH_SECOND,
                .scaler.outheight = SENSOR_HEIGHT_SECOND,
            },
        },

        .funcPipeNum = 0,
    },
};


void frameProcess0(IMPFrameInfo *frame, void *usrData)
{
    char fileName[64] = "/tmp/frame0.nv12";
    static uint32_t frameNum = 0;

    frameNum++;
    if (frameNum % 100 != 0)
        return;

    fwriteFileByName(fileName, (void *)frame->virAddr, frame->size);
}

void frameProcess1(IMPFrameInfo *frame, void *usrData)
{
    char fileName[64] = "/tmp/frame1.nv12";

    writeFileByName(fileName, (void *)frame->virAddr, frame->size);
}

int main(void)
{
    int ret;
    IMPF_FSChn *FSChn0, *FSChn1;
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

    FSChn0 = &videoSets.videoStreamList[0].FSChn;
    if(FSChn0->interface != NULL) {
        FSChn0->interface->startFrameProcess(FSChn0, frameProcess0, NULL);
    }

    FSChn1 = &videoSets.videoStreamList[1].FSChn;
    if(FSChn1->interface != NULL) {
        ret = FSChn1->interface->setFrameDepth(FSChn1, 1);
        if (ret < 0) {
            printf("setFrameDepth failed!\n");
            videoHandler->stop();
            videoHandler->deinit();
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
        sleep(5);
    }
    if(FSChn1->interface != NULL) {
        FSChn1->interface->setFrameDepth(FSChn1, 0);
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
