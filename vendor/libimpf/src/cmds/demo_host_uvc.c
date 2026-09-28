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

#include "common.h"
#include "image_convert.h"
#include "huffman.h"


#define LOG_TAG                         "IMPF_DEMO"


#define V4L2_DEVICE_NAME                "/dev/video0"

#define CAMERA_WIDTH                    432
#define CAMERA_HEIGHT                   720
#define CAMERA_PIXFMT                   V4L2_PIX_FMT_YUYV
//#define CAMERA_PIXFMT                   V4L2_PIX_FMT_MJPEG

#define PREVIEW_RGN_WIDTH               480
#define PREVIEW_RGN_HEIGHT              800
#define PREVIEW_PIXFMT                  AV_PIX_FMT_BGRA

#define TEST_GET_PRODUCT                0
#define TEST_QUERY_FMT_CTRL             0
#define TEST_FPS                        0
#define TEST_HV_FLIP                    0
#define TEST_BRIGHTNESS                 0
#define TEST_CONTRAST                   0
#define TEST_SATURATION                 0
#define TEST_SHARPNESS                  0
#define TEST_HUE                        0
#define TEST_GAMMA                      0
#define TEST_WB                         0
#define TEST_GAIN                       0
#define TEST_ANTI_FLICKER               0
#define TEST_BACKLIGHT_COMP             0




IMPF_MediaManager* mediaManager;
IMPF_V4L2VideoHandler* V4L2VideoHandler;
IMPF_FBHandler* FBHandler;
rectRgn previewRgn = {0, 0, PREVIEW_RGN_WIDTH, PREVIEW_RGN_HEIGHT}; //图像分辨率小于480*800
//rectRgn previewRgn = {400, 0, 720+PREVIEW_RGN_WIDTH, 0+PREVIEW_RGN_HEIGHT}; //图像分辨率1280*720
void *BGRABuf = NULL;

IMPF_V4L2VideoChn V4L2VideoChn = {
    .dev = V4L2_DEVICE_NAME,
    .fmt = {
        .width = CAMERA_WIDTH,
        .height = CAMERA_HEIGHT,
        .pixfmt = CAMERA_PIXFMT,
    },
    .fps = 25,
    .nBuf = 2,
};



int isHuffman(uint8_t *buf)
{
    uint8_t *ptbuf;
    int i = 0;
    ptbuf = buf;
    while (((ptbuf[0] << 8) | ptbuf[1]) != 0xffda){
        if(i++ > 2048)
            return 0;
        if(((ptbuf[0] << 8) | ptbuf[1]) == 0xffc4)
            return 1;
        ptbuf++;
    }
    return 0;
}

int saveJpeg(char *path, void *buf, size_t size)
{
    FILE *file;
    uint8_t *ptdeb, *ptcur = buf;
    int sizein;

    file = fopen(path, "wb");
    if (file != NULL) {
        if(!isHuffman(buf)){
            ptdeb = ptcur = buf;
            while (((ptcur[0] << 8) | ptcur[1]) != 0xffc0)
                ptcur++;
            sizein = ptcur-ptdeb;
            fwrite(buf, sizein, 1, file);
            fwrite(dht_data, DHT_SIZE, 1, file);
            fwrite(ptcur, size-sizein, 1, file);
        } else {
            fwrite(ptcur, size, 1 , file);
        }
        fclose(file);
    }

    return 0;
}


void frameProcess(IMPF_V4L2VideoFrameInfo *frame, void *usrData)
{
    char fileName[64] = {0};
    static long frameCnt = 0;

    frameCnt++;
    if (V4L2_PIX_FMT_YUYV == frame->pixfmt) {
        image_convert((uint8_t *)frame->virAddr, frame->width, frame->height, v4l2pixfmt_2_avpixfmt(frame->pixfmt),
              BGRABuf, CAMERA_WIDTH, CAMERA_HEIGHT, PREVIEW_PIXFMT);

        rectRgnCopyFromSrc(BGRABuf, CAMERA_WIDTH, CAMERA_HEIGHT,
                        FBHandler->getFBMem(), FBHandler->getScreenWidth(), FBHandler->getScreenHeight(),
                        FBHandler->getBitsPerPixel(), previewRgn);
        FBHandler->display();
    } else if (V4L2_PIX_FMT_MJPEG == frame->pixfmt) {
        if (frameCnt%30 == 0) {
            sprintf(fileName, "/tmp/frame.jpg");
            saveJpeg(fileName, (void *)frame->virAddr, frame->size);
        }
    }
}


int main(void)
{
    IMPF_V4L2VideoFrameInfo *frame;
    int ret;

    mediaManager = IMPF_GetMediaManager();
    V4L2VideoHandler = mediaManager->getV4L2VideoHandler();
    FBHandler = mediaManager->getFBHandler();

	BGRABuf = calloc(1, image_size(PREVIEW_PIXFMT, CAMERA_WIDTH, CAMERA_HEIGHT));
    if (BGRABuf == NULL) {
        printf("calloc BGRABuf failed\n");
        return -1;
    }

	ret = FBHandler->init();
    if(ret < 0) {
        printf("FBHandler->init fail!\n");
        return -1;
    }

    ret = V4L2VideoHandler->init(&V4L2VideoChn);
    if(ret < 0) {
        printf("V4L2VideoHandler->init fail!\n");
        return -1;
    }

    ret = V4L2VideoHandler->start(&V4L2VideoChn);
    if(ret < 0) {
        printf("V4L2VideoHandler->start fail!\n");
        V4L2VideoHandler->deinit(&V4L2VideoChn);
        return -1;
    }

#if TEST_GET_PRODUCT
    char product[32];
    V4L2VideoHandler->getProduct(&V4L2VideoChn, product);
    printf("---- product: %s \n", product);
#endif

#if TEST_QUERY_FMT_CTRL
    V4L2VideoHandler->menuFmt(&V4L2VideoChn);
    V4L2VideoHandler->queryCtrl(&V4L2VideoChn);
#endif

#if TEST_FPS
    uint32_t fps;
    V4L2VideoHandler->setFPS(&V4L2VideoChn, 20);
    V4L2VideoHandler->getFPS(&V4L2VideoChn, &fps);
    printf("---- fps: %d \n", fps);
#endif

#if TEST_HV_FLIP
    V4L2VideoHandler->setHFlip(&V4L2VideoChn, 1);
    V4L2VideoHandler->setVFlip(&V4L2VideoChn, 1);
#endif

#if TEST_BRIGHTNESS
    int brightness = 30;
    V4L2VideoHandler->setBrightness(&V4L2VideoChn, brightness);
    V4L2VideoHandler->getBrightness(&V4L2VideoChn, &brightness);
    printf("---- brightness: %d\n", brightness);
#endif

#if TEST_CONTRAST
    int contrast = 64;
    V4L2VideoHandler->setContrast(&V4L2VideoChn, contrast);
    V4L2VideoHandler->getContrast(&V4L2VideoChn, &contrast);
    printf("---- contrast: %d\n", contrast);
#endif

#if TEST_SATURATION
    int saturation = 100;
    V4L2VideoHandler->setSaturation(&V4L2VideoChn, saturation);
    V4L2VideoHandler->getSaturation(&V4L2VideoChn, &saturation);
    printf("---- saturation: %d\n", saturation);
#endif

#if TEST_SHARPNESS
    int sharpness = 6;
    V4L2VideoHandler->setSharpness(&V4L2VideoChn, sharpness);
    V4L2VideoHandler->getSharpness(&V4L2VideoChn, &sharpness);
    printf("---- sharpness: %d\n", sharpness);
#endif

#if TEST_HUE
    int hue = 10;
    V4L2VideoHandler->setHue(&V4L2VideoChn, hue);
    V4L2VideoHandler->getHue(&V4L2VideoChn, &hue);
    printf("---- hue: %d\n", hue);
#endif

#if TEST_GAMMA
    int gamma = 200;
    V4L2VideoHandler->setGamma(&V4L2VideoChn, gamma);
    V4L2VideoHandler->getGamma(&V4L2VideoChn, &gamma);
    printf("---- gamma: %d\n", gamma);
#endif

#if TEST_WB
    IMPF_V4L2VideoWB WB;
    V4L2VideoHandler->getWB(&V4L2VideoChn, &WB);
    printf("---- WB mode: %d, temperature: %d\n", WB.mode, WB.temperature);
    WB.mode = 0;
    WB.temperature = 2800;
    V4L2VideoHandler->setWB(&V4L2VideoChn, &WB);
#endif

#if TEST_GAIN
    int gain = 4;
    V4L2VideoHandler->setGain(&V4L2VideoChn, gain);
    V4L2VideoHandler->getGain(&V4L2VideoChn, &gain);
    printf("---- gain: %d\n", gain);
#endif

#if TEST_ANTI_FLICKER
    V4L2VideoHandler->setAntiFlicker(&V4L2VideoChn, V4L2_CID_POWER_LINE_FREQUENCY_50HZ);
#endif

#if TEST_BACKLIGHT_COMP
    V4L2VideoHandler->setBackLightComp(&V4L2VideoChn, 1);
#endif

    ret = V4L2VideoHandler->setFrameDepth(&V4L2VideoChn, 1);
    if (ret < 0) {
        printf("setFrameDepth failed!\n");
        V4L2VideoHandler->stop(&V4L2VideoChn);
        V4L2VideoHandler->deinit(&V4L2VideoChn);
        return -1;
    }
    while(1) {
        if (V4L2VideoHandler->getFrame(&V4L2VideoChn, &frame, 2*1000) != 0)
            continue;

        frameProcess(frame, NULL);

        V4L2VideoHandler->releaseFrame(&V4L2VideoChn, frame);
    }
    V4L2VideoHandler->setFrameDepth(&V4L2VideoChn, 0);

    ret = V4L2VideoHandler->stop(&V4L2VideoChn);
    if(ret < 0) {
        printf("V4L2VideoHandler->stop fail!\n");
    }

    ret = V4L2VideoHandler->deinit(&V4L2VideoChn);
    if(ret < 0) {
        printf("V4L2VideoHandler->deinit fail!\n");
    }

    ret = FBHandler->deinit();
    if(ret < 0) {
        printf("FBHandler->deinit fail!\n");
    }

	if (BGRABuf)
        free(BGRABuf);

    return 0;
}
