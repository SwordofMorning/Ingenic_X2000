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


#define TEST_RUNNING_MODE               0
#define TEST_FPS                        0
#define TEST_HV_FLIP                    0
#define TEST_BRIGHTNESS                 0
#define TEST_CONTRAST                   0
#define TEST_SHARPNESS                  0
#define TEST_SATURATION                 0
#define TEST_SINTER                     0
#define TEST_TEMPER                     0
#define TEST_AE_COMP                    0
#define TEST_EXP                        0
#define TEST_SCENE_MODE                 0
#define TEST_ANTI_FLICKER               0
#define TEST_SENSOR_REG                 0




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
};


void frameProcess(IMPFrameInfo *frame, void *usrData)
{
    char fileName[64] = "/tmp/frame.nv12";
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

#if 0
/*
* sensor_s5k6a1 can work correctly after crop useless pixels
*/
    printf("================S5K6A1================\n");
    system("echo 1024 > /sys/module/tx_isp_mscaler/parameters/isph");
    system("echo 8 > /sys/module/tx_isp_mscaler/parameters/isptop");
    system("echo 8 > /sys/module/tx_isp_mscaler/parameters/ispleft");
#endif

    ret = videoHandler->start();
    if(ret < 0) {
        printf("videoHandler->start fail!\n");
        videoHandler->deinit();
        return -1;
    }

#if TEST_RUNNING_MODE
    IMP_ISP_Tuning_SetISPRunningMode(IMPISP_RUNNING_MODE_NIGHT);
#endif

#if TEST_FPS
    uint32_t fpsNum, fpsDen;
    IMP_ISP_Tuning_SetSensorFPS(20, 1);
    IMP_ISP_Tuning_GetSensorFPS(&fpsNum, &fpsDen);
    printf("---- fps: %.1f \n", (float)fpsNum/fpsDen);
#endif

#if TEST_HV_FLIP
    IMP_ISP_Tuning_SetISPHVflip(IMPISP_TUNING_OPS_MODE_DISABLE, IMPISP_TUNING_OPS_MODE_ENABLE);
#endif

#if TEST_BRIGHTNESS
    uint8_t brightness = 128;
    IMP_ISP_Tuning_SetBrightness(brightness);
    IMP_ISP_Tuning_GetBrightness(&brightness);
    printf("---- brightness: %d\n", brightness);
#endif

#if TEST_CONTRAST
    uint8_t contrast = 128;
    IMP_ISP_Tuning_SetContrast(contrast);
    IMP_ISP_Tuning_GetContrast(&contrast);
    printf("---- contrast: %d\n", contrast);
#endif

#if TEST_SHARPNESS
    uint8_t sharpness = 128;
    IMP_ISP_Tuning_SetSharpness(sharpness);
    IMP_ISP_Tuning_GetSharpness(&sharpness);
    printf("---- sharpness: %d\n", sharpness);
#endif

#if TEST_SATURATION
    uint8_t saturation = 128;
    IMP_ISP_Tuning_SetSaturation(saturation);
    IMP_ISP_Tuning_GetSaturation(&saturation);
    printf("---- saturation: %d\n", saturation);
#endif

#if TEST_SINTER
    IMPISPSinterDenoiseAttr sinterAttr = {
        .enable = IMPISP_TUNING_OPS_MODE_ENABLE,
        .type = IMPISP_TUNING_OPS_TYPE_MANUAL,
        .sinter_strength = 20,
    };
    IMP_ISP_Tuning_SetSinterDnsAttr(&sinterAttr);
#endif

#if TEST_TEMPER
    IMPISPTemperDenoiseAttr temperAttr = {
        .type = IMPISP_TEMPER_MANUAL,
        .temper_strength = 100,
    };
    IMP_ISP_Tuning_SetTemperDnsAttr(&temperAttr);
#endif

#if TEST_SCENE_MODE
    IMPISPSceneMode sceneMode = IMPISP_SCENE_MODE_AUTO;
    IMP_ISP_Tuning_SetSceneMode(sceneMode);
#endif

#if TEST_EXP
    IMPISPExpr expGet, expSet;
    IMP_ISP_Tuning_GetExpr(&expGet);
    printf("---- exp mode: %d, integration time: %d\n", expGet.g_attr.mode, expGet.g_attr.integration_time);

    expSet.s_attr.mode = ISP_CORE_EXPR_MODE_MANUAL;
    expSet.s_attr.unit = ISP_CORE_EXPR_UNIT_LINE;
    expSet.s_attr.time = expGet.g_attr.integration_time;
    IMP_ISP_Tuning_SetExpr(&expSet);
#endif

#if TEST_AE_COMP
    IMP_ISP_Tuning_SetAeComp(120);
#endif

#if TEST_ANTI_FLICKER
    IMP_ISP_Tuning_SetAntiFlickerAttr(IMPISP_ANTIFLICKER_50HZ);
#endif

#if TEST_SENSOR_REG
    uint32_t reg = 0x00;
    uint32_t val = 0xaa;
    IMP_ISP_SetSensorRegister(reg, val);
    IMP_ISP_GetSensorRegister(reg, &val);
    printf("---- reg(val): %x(%x)\n", reg, val);
#endif


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
