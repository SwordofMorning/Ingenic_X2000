/*
*  Copyright (C) 2019, <yicnhun.zhou@ingenic.com>
*
*  Ingenic uvc samples project
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
#include "uvc.h"

#define LOG_TAG                         "IMPF_DEMO"


#define TEST_RUNNING_MODE               1
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
#define TEST_UVC                        1

char *out=NULL;

struct uvc_device *udev;

typedef enum {
    V4L2_METHOD_READ,
    V4L2_METHOD_MMAP,
    V4L2_METHOD_USERPTR,
} camera_v4l2_io_m;


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

int nv12_copyto_yuyv422(uint8_t* src, uint8_t* dst, uint32_t width, uint32_t height)
{
    int i,j;
    uint32_t w = width;
    uint32_t h = height;
    uint8_t *src_uv_plane = src+w*((h+15)&~15);
    uint8_t *src_y = src;
    uint8_t *dst_ = dst;
    uint8_t *src_uv;

    for (i = 0; i < h; i++) {
        src_uv = src_uv_plane+(i/2)*w;
        for (j = 0; j < w/2; j++) {
            *dst_ = *src_y;
            dst_++;
            src_y++;
            *dst_ = *src_uv;
            dst_++;
            src_uv++;
            *dst_ = *src_y;
            dst_++;
            src_y++;
            *dst_ = *src_uv;
            dst_++;
            src_uv++;
        }
    }
    return 0;
}



void frameProcess(IMPFrameInfo *frame, void *usrData)
{
   //char fileName[64] = "/tmp/frame.nv12";
   //char fileName1[64] = "/tmp/frame.yuv";
    static uint32_t frameNum = 0;
    int ret;
    const int width = 640;
    const int height = 480;

    /*frameNum++;
    if (frameNum % 100 != 0)
        return;*/
    //fwriteFileByName(fileName, (void *)frame->virAddr, frame->size);
    nv12_copyto_yuyv422((void *)frame->virAddr, out, SENSOR_WIDTH, SENSOR_HEIGHT);
   // fwriteFileByName(fileName1, (void *)out, SENSOR_WIDTH*SENSOR_HEIGHT*2);
   // fwriteFileByName(fileName, (void *)frame->virAddr, frame->size);

    if(TEST_UVC == 1) {

    image_load(udev, out, udev->width, udev->height);
    fd_set  fdsu;
    FD_ZERO(&fdsu);

    /* We want both setup and data events on UVC interface.. */
    FD_SET(udev->uvc_fd, &fdsu);
    fd_set dfds = fdsu;
    fd_set efds = fdsu;
    ret = select(udev->uvc_fd + 1, NULL, &dfds, &efds, NULL);

    if (FD_ISSET(udev->uvc_fd, &efds))
    uvc_events_process(udev);
    if (FD_ISSET(udev->uvc_fd, &dfds))
    uvc_video_process(udev);
    }
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
    if(TEST_UVC == 1) {
        out=malloc(SENSOR_WIDTH*SENSOR_HEIGHT*2);
        char *uvc_devname = "/dev/video3";
        int bulk_mode = 0;
        int dummy_data_gen_mode = 1;
        /* Frame format/resolution related params. */
        int nbufs = 3;          /* Ping-Pong buffers */
        /* USB speed related params */
        int mult = 0;
        int burst = 0;
        enum usb_device_speed speed = 2;    /* High-Speed */
        //enum io_method uvc_io_method = IO_METHOD_USERPTR;
        enum io_method uvc_io_method = IO_METHOD_MMAP;
        ret = uvc_open(&udev, uvc_devname,SENSOR_WIDTH, SENSOR_HEIGHT);
        if (udev == NULL || ret < 0)
            return 1;
        udev->uvc_devname = uvc_devname;
        /* Set parameters as passed by user. */
        udev->width = SENSOR_WIDTH;
        udev->height = SENSOR_HEIGHT;
        udev->imgsize = (udev->width * udev->height * 2);
        udev->fcc = V4L2_PIX_FMT_YUYV;
        udev->io = uvc_io_method;
        udev->bulk = bulk_mode;
        udev->nbufs = nbufs;
        udev->mult = mult;
        udev->burst = burst;
        udev->speed = speed;
        switch (speed) {
            case USB_SPEED_FULL:
            udev->maxpkt = 1023;
            break;
            default:
            case USB_SPEED_HIGH:
            udev->maxpkt = 1024;
        }
        if (dummy_data_gen_mode)
            /* UVC standalone setup. */
            udev->run_standalone = 1;
        uvc_video_set_format(udev);
    }

#if TEST_RUNNING_MODE
    IMP_ISP_Tuning_SetISPRunningMode(IMPISP_RUNNING_MODE_NIGHT);
#endif

#if TEST_FPS
    uint32_t fpsNum, fpsDen;
    IMP_ISP_Tuning_SetSensorFPS (20, 1);
    IMP_ISP_Tuning_GetSensorFPS (&fpsNum, &fpsDen);
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
    expSet.s_attr.time = expGet.g_attr.integration_time_max;
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
    printf("---- reg(val): %d(%d)\n", reg, val);
#endif


    ret = videoHandler->start();
    if(ret < 0) {
        printf("videoHandler->start fail!\n");
        return -1;
    }

    IMPF_FSChn *FSChn = &videoSets.videoStreamList[0].FSChn;
    if(FSChn->interface != NULL) {
        FSChn->interface->startFrameProcess(FSChn, frameProcess, NULL);
    }
    while(1) {
        sleep(10);
    }

    ret = videoHandler->stop();
    if(ret < 0) {
        printf("videoHandler->stop fail!\n");
        return -1;
    }

    ret = videoHandler->deinit();
    if(ret < 0) {
        printf("videoHandler->deinit fail!\n");
        return -1;
    }
    if(TEST_UVC == 1) {
            if (udev->is_streaming) {
                /* ... and now UVC streaming.. */
                uvc_video_stream(udev, 0);
                uvc_uninit_device(udev);
                uvc_video_reqbufs(udev, 0);
                udev->is_streaming = 0;
            }
            uvc_close(udev);
        }

    return 0;
}
