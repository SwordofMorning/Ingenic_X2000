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
#include "logodata_100x100_bgra.h"
#ifdef SUPPORT_RGB555LE
#include "bgramapinfo_rgb555le.h"
#else
#include "bgramapinfo.h"
#endif



#define LOG_TAG                         "IMPF_DEMO"


#define LOGO_WIDTH                      100
#define LOGO_HEIGHT                     100

#define FONT_WIDTH                      16
#define FONT_HEIGHT                     34
#define TIME_STAMP_CHAR_NUM             20



IMPF_MediaManager* mediaManager;
IMPF_VideoHandler* videoHandler;
int demoThreadRunFlag = 0;

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

        .funcPipeNum = 2,
        .funcPipeList[0] = {
            .funcPipeType = FuncPipe_OSD,
            .osd = {
                .OSDRgnNum = 2,
                .OSDRgnList[0] = {
                    .rgnAttr = {
                        .type = OSD_REG_PIC,
                        .rect = {
                            .p0.x = SENSOR_WIDTH - LOGO_WIDTH,
                            .p0.y = SENSOR_HEIGHT - LOGO_HEIGHT,
                            .p1.x = SENSOR_WIDTH - 1,     //p0 is start，and p1 well be epual p0+width(or heigth)-1
                            .p1.y = SENSOR_HEIGHT - 1,
                        },
                        .fmt = PIX_FMT_BGRA,
                        .data.picData.pData = logodata_100x100_bgra,
                    },
                    .grpRgnAttr = {
                        .show = 1,
                        .gAlphaEn = 1,
                        .fgAlhpa = 0xaf,
                        .layer = 1,
                    },
                },
                .OSDRgnList[1] = {
                    .rgnAttr = {
                        .type = OSD_REG_PIC,
                        .rect = {
                            .p0.x = 10,
                            .p0.y = 10,
                            .p1.x = 10 + TIME_STAMP_CHAR_NUM * FONT_WIDTH - 1,     //p0 is start，and p1 well be epual p0+width(or heigth)-1
                            .p1.y = 10 + FONT_HEIGHT - 1,
                        },
#ifdef SUPPORT_RGB555LE
                        .fmt = PIX_FMT_RGB555LE,
#else
                        .fmt = PIX_FMT_BGRA,
#endif
                        .data.picData.pData = NULL,
                    },
                    .grpRgnAttr = {
                        .show = 0,
                        .gAlphaEn = 1,
                        .fgAlhpa = 0xff,
                        .layer = 2,
                    },
                },
            },
        },
        .funcPipeList[1] = {
            .funcPipeType = FuncPipe_ENC,
            .enc = {
                .encChnNum = 1,
                .encChnList[0] = {0},
            },
        },
    },
};



void jpegEncConfig(IMPF_FuncPipe_EncChn *chn)
{
    encodeConfig(chn, PT_JPEG, 0);
}


void *timeStamUpdateThread(void *arg)
{
    IMPF_FuncPipe_OSDRgn *rgn = (IMPF_FuncPipe_OSDRgn *)arg;
    IMPF_OSDRgnInterface *interface = rgn->interface;
    time_t currTime;
    struct tm *currDate;
    char DateStr[TIME_STAMP_CHAR_NUM];
    void *dateData = NULL;
    uint32_t *timeStampData;
    uint32_t i, j;

#ifdef SUPPORT_RGB555LE
    timeStampData = malloc(TIME_STAMP_CHAR_NUM * FONT_HEIGHT * FONT_WIDTH * sizeof(uint16_t));
#else
    timeStampData = malloc(TIME_STAMP_CHAR_NUM * FONT_HEIGHT * FONT_WIDTH * sizeof(uint32_t));
#endif

    interface->isRgnShow(rgn, 1);

    while(demoThreadRunFlag) {
        int penPos = 0;
        int fontWidth = 0;

        time(&currTime);
        currDate = localtime(&currTime);
        strftime(DateStr, 40, "%Y-%m-%d %I:%M:%S", currDate);

        for (i = 0; i < TIME_STAMP_CHAR_NUM; i++) {
            switch(DateStr[i]) {
                case '0' ... '9':
                    dateData = (void *)gBgramap[DateStr[i] - '0'].pdata;
                    fontWidth = gBgramap[DateStr[i] - '0'].width;
                    penPos += gBgramap[DateStr[i] - '0'].width;
                    break;
                case '-':
                    dateData = (void *)gBgramap[10].pdata;
                    fontWidth = gBgramap[10].width;
                    penPos += gBgramap[10].width;
                    break;
                case ' ':
                    dateData = (void *)gBgramap[11].pdata;
                    fontWidth = gBgramap[11].width;
                    penPos += gBgramap[11].width;
                    break;
                case ':':
                    dateData = (void *)gBgramap[12].pdata;
                    fontWidth = gBgramap[12].width;
                    penPos += gBgramap[12].width;
                    break;
                default:
                    break;
            }
#ifdef SUPPORT_RGB555LE
            for (j = 0; j < FONT_HEIGHT; j++) {
                memcpy((void *)((uint16_t *)timeStampData + j*TIME_STAMP_CHAR_NUM*FONT_WIDTH + penPos),
                        (void *)((uint16_t *)dateData + j*fontWidth), fontWidth*sizeof(uint16_t));
            }
#else
            for (j = 0; j < FONT_HEIGHT; j++) {
                memcpy((void *)((uint32_t *)timeStampData + j*TIME_STAMP_CHAR_NUM*FONT_WIDTH + penPos),
                        (void *)((uint32_t *)dateData + j*fontWidth), fontWidth*sizeof(uint32_t));
            }
#endif
        }

        interface->updateRgnDate(rgn, timeStampData);

        sleep(1);
    }

    free(timeStampData);

    return NULL;
}


void *streamProcessThread(void *arg)
{
    IMPF_FuncPipe_EncChn *chn = (IMPF_FuncPipe_EncChn *)arg;
    IMPF_EncChnInterface *interface = chn->interface;
    IMPEncoderStream stream;
    char fileName[64] = "/tmp/stream.jpeg";
    static uint32_t frameNum = 0;

    if (interface->startEncode(chn) != 0) {
        printf("start encode fail!\n");
        return NULL;
    }
    while (demoThreadRunFlag) {
        frameNum++;
        if (frameNum % 100 != 0)
            continue;

        if (interface->getStream(chn, &stream, 1000) != 0)
            continue;

        writeStreamByName(fileName, &stream);

        interface->releaseStream(chn, &stream);
    }
    interface->stopEncode(chn);

    return NULL;
}


int main(void)
{
    int ret;
    IMPF_FuncPipe_EncChn *EncChn;

    memcpy(videoSets.sensorInfo.name, SENSOR_NAME, sizeof(SENSOR_NAME));
    memcpy(videoSets.sensorInfo.i2c.type, SENSOR_NAME, sizeof(SENSOR_NAME));
    EncChn = &videoSets.videoStreamList[0].funcPipeList[1].enc.encChnList[0];
    jpegEncConfig(EncChn);

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

    demoThreadRunFlag = 1;

    IMPF_FuncPipe_OSDRgn *OSDRgn = &videoSets.videoStreamList[0].funcPipeList[0].osd.OSDRgnList[1];
    pthread_t ptOSDID;
    if (OSDRgn->interface != NULL) {
        ret = pthread_create(&ptOSDID, NULL, timeStamUpdateThread, (void *)OSDRgn);
        if (ret < 0) {
            printf("create timeStamUpdateThread failed\n");
            goto End;
        }
    }

    pthread_t ptEncID;
    if (EncChn->interface != NULL) {
        ret = pthread_create(&ptEncID, NULL, streamProcessThread, (void *)EncChn);
        if (ret < 0) {
            printf("create streamProcessThread failed\n");
            goto End;
        }
    }

    while(1) {
        sleep(1);
    }

    demoThreadRunFlag = 0;
    if (EncChn->interface != NULL)
        pthread_join(ptEncID, NULL);
    if (OSDRgn->interface != NULL)
        pthread_join(ptOSDID, NULL);

End:
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
