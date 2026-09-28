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
#define EXT_CHN_ENC                     0
#define JPEGQL_SET                      1


#define RECORD_TIME_MS                  30*1000



IMPF_MediaManager* mediaManager;
IMPF_VideoHandler* videoHandler;

#if EXT_CHN_ENC
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
                .pixFmt = PIX_FMT_YUYV422,
                .outFrmRateNum = SENSOR_FPS_NUM,
                .outFrmRateDen = SENSOR_FPS_DEN,
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
                .picWidth = SENSOR_WIDTH,
                .picHeight = SENSOR_HEIGHT,
                .pixFmt = PIX_FMT_NV12,
                .outFrmRateNum = SENSOR_FPS_NUM,
                .outFrmRateDen = SENSOR_FPS_DEN,
                .nrVBs = 1,

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

        .funcPipeNum = 1,
        .funcPipeList[0] = {
            .funcPipeType = FuncPipe_ENC,
            .enc = {
                .encChnNum = 2,
                .encChnList[0] = {0},
                .encChnList[1] = {
                    .encChnAttr = {
                        .encAttr = {
                            .enType = PT_JPEG,
                            .bufSize = 0,
                            .profile = 0,
                            .picWidth = SENSOR_WIDTH,
                            .picHeight = SENSOR_HEIGHT,
                        },
                    },
                },
            },
        },
    },
};
#else
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
                .outFrmRateNum = SENSOR_FPS_NUM,
                .outFrmRateDen = SENSOR_FPS_DEN,
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

        .funcPipeNum = 1,
        .funcPipeList[0] = {
            .funcPipeType = FuncPipe_ENC,
            .enc = {
                .encChnNum = 2,
                .encChnList[0] = {0},
                .encChnList[1] = {
                    .encChnAttr = {
                        .encAttr = {
                            .enType = PT_JPEG,
                            .bufSize = 0,
                            .profile = 0,
                            .picWidth = SENSOR_WIDTH,
                            .picHeight = SENSOR_HEIGHT,
                        },
                    },
                },
            },
        },
    },
};
#endif



void h264EncConfig(IMPF_FuncPipe_EncChn *chn)
{
    encodeConfig(chn, PT_H264, ENC_RC_MODE_SMART);
}


int saveJpeg(IMPF_FuncPipe_EncChn *chn)
{
    IMPF_EncChnInterface *interface = chn->interface;
    IMPEncoderStream stream;
    char fileName[64] = "/tmp/stream.jpeg";

    if (interface->startEncode(chn) != 0) {
        printf("start jpeg encode fail!\n");
        return -1;
    }

    if (interface->getStream(chn, &stream, 1000) != 0) {
        interface->stopEncode(chn);
        return -1;
    }

    writeStreamByName(fileName, &stream);

    interface->releaseStream(chn, &stream);

    return 0;
}


void *saveH264Thread(void *arg)
{
    IMPF_FuncPipe_EncChn *chn = (IMPF_FuncPipe_EncChn *)arg;
    IMPF_EncChnInterface *interface = chn->interface;
    IMPEncoderStream stream;
    char fileName[64] = "/tmp/stream.h264";
    size_t writedSize;
    int fd, i;
    int64_t startRecordTimeUS;


    fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (fd < 0) {
        printf("Failed to open %s\n", fileName);
        return NULL;
    }

    if (interface->startEncode(chn) != 0) {
        printf("start h264 encode fail!\n");
        return NULL;
    }
    startRecordTimeUS = getTimeUS();
    while ((getTimeUS()-startRecordTimeUS) <= (RECORD_TIME_MS*1000)) {
        if (interface->getStream(chn, &stream, 1000) != 0)
            continue;

        for (i = 0; i < stream.packCount; i++) {
            writedSize = write(fd, (void *)stream.pack[i].virAddr, stream.pack[i].length);
            if (writedSize != stream.pack[i].length) {
                printf("writeStreamByName write fail, %s, writedSize(%d) != stream.pack[%d].length(%d)\n", strerror(errno), writedSize, i, stream.pack[i].length);
                interface->releaseStream(chn, &stream);
                interface->stopEncode(chn);
                close(fd);
                return NULL;
            }
        }

        interface->releaseStream(chn, &stream);
    }
    interface->stopEncode(chn);

    close(fd);

    return NULL;
}


int main(void)
{
    int ret;
    IMPF_FuncPipe_EncChn *h264EncChn;
    IMPF_FuncPipe_EncChn *jpegEncChn;

    memcpy(videoSets.sensorInfo.name, SENSOR_NAME, sizeof(SENSOR_NAME));
    memcpy(videoSets.sensorInfo.i2c.type, SENSOR_NAME, sizeof(SENSOR_NAME));
#if EXT_CHN_ENC
    jpegEncChn = &videoSets.videoStreamList[1].funcPipeList[0].enc.encChnList[1];
    h264EncChn = &videoSets.videoStreamList[1].funcPipeList[0].enc.encChnList[0];
#else
    jpegEncChn = &videoSets.videoStreamList[0].funcPipeList[0].enc.encChnList[1];
    h264EncChn = &videoSets.videoStreamList[0].funcPipeList[0].enc.encChnList[0];
#endif
    h264EncConfig(h264EncChn);

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

#if 0
    //if frmRateNum * frmRateDen > 64, encode fail
    IMP_ISP_Tuning_SetSensorFPS(30, 1);
#endif

    //sensor刚出流时，图像可能比较暗
    sleep(1);

#if JPEGQL_SET
    IMPEncoderJpegeQl jpegeQl;
    IMP_Encoder_GetJpegeQl(jpegEncChn->chnIndex, &jpegeQl);
    MakeTables(64, &(jpegeQl.qmem_table[0]), &(jpegeQl.qmem_table[64]));
    jpegeQl.user_ql_en = 1;
    IMP_Encoder_SetJpegeQl(jpegEncChn->chnIndex, &jpegeQl);
#endif
    if (jpegEncChn->interface != NULL) {
        saveJpeg(jpegEncChn);
    }

    pthread_t ptEncH264ID;
    if (h264EncChn->interface != NULL) {
        ret = pthread_create(&ptEncH264ID, NULL, saveH264Thread, (void *)h264EncChn);
        if (ret < 0) {
            printf("create saveH264Thread failed\n");
        } else {
            pthread_join(ptEncH264ID, NULL);
        }
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
