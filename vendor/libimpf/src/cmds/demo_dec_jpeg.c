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


#define SIMPLE_USAGE

#define JPEG_PATH                        "/tmp/stream.jpeg"
#define DEC_DATA_PATH                    "/tmp/frame.nv12"



IMPDecoderCHNAttr decChnAttr = {
    .decAttr = {
        .decType            = PT_JPEG,
        .maxWidth           = SENSOR_WIDTH,
        .maxHeight          = SENSOR_HEIGHT,
        .pixelFormat        = PIX_FMT_NV12,
        .nrKeepStream       = 2,
        .frmRateNum         = 25,
        .frmRateDen         = 1,
    },
};


#ifdef SIMPLE_USAGE
int main(void)
{
    int ret = -1;
    int jpegSize;
    void *jpegBuf = NULL;
    IMPDecoderStream stream;
    IMPFrameInfo *frame;

    if (access(JPEG_PATH, F_OK) != 0) {
        printf("jpeg file is not exist(%s), can not decode!\n", JPEG_PATH);
        return -1;
    }
    jpegSize = getFileSize(JPEG_PATH);
    if (-1 == jpegSize)
        return -1;
    jpegBuf = calloc(1, jpegSize);
    if (jpegBuf == NULL) {
        printf("calloc jpegBuf failed, jpegSize: %d\n", jpegSize);
        return -1;
    }
    if (readFileByName(JPEG_PATH, jpegBuf, jpegSize) != 0)
        goto free_buf;

    ret = IMPF_VPU_JpegDecInit(&decChnAttr);
    if (ret != 0) {
        printf("IMPF_VPU_JpegDecInit fail\n");
        goto free_buf;
    }

    stream.decoderNal.i_payload = jpegSize;
    stream.decoderNal.p_payload = jpegBuf;
    stream.decoderNal.timeStamp = 0;
    ret = IMPF_VPU_JpegDecDoing(&stream, &frame);
    if (ret != 0) {
        printf("IMPF_VPU_JpegDecDoing fail\n");
        goto jpeg_dec_deinit;
    }

    fwriteFileByName(DEC_DATA_PATH, (void *)frame->virAddr, frame->size);
    printf("jpeg decode success, wirte file(%s)\n", DEC_DATA_PATH);

    IMPF_VPU_JpegDecOutputReleaseFrame(frame);

jpeg_dec_deinit:
    IMPF_VPU_JpegDecDeinit();

free_buf:
    if (jpegBuf)
        free(jpegBuf);

    return ret;
}

#else
int demoThreadRunFlag = 0;

void *streamInputThread(void *arg)
{
    int jpegSize;
    void *jpegBuf;
    IMPDecoderStream stream;

    jpegBuf = calloc(1, calc_pic_size(SENSOR_WIDTH, SENSOR_HEIGHT, PIX_FMT_NV12));
    if (jpegBuf == NULL) {
        printf("malloc jpegBuf failed\n");
        return NULL;
    }

    while (demoThreadRunFlag) {
        while (access(JPEG_PATH, F_OK) != 0) {
            printf("jpeg file is not exist(%s), can not decode!\n", JPEG_PATH);
            sleep(1);
        }
        usleep(300*1000);

        jpegSize = getFileSize(JPEG_PATH);
        if (-1 == jpegSize)
            continue;
        if (readFileByName(JPEG_PATH, jpegBuf, jpegSize) != 0)
            continue;

        stream.decoderNal.i_payload = jpegSize;
        stream.decoderNal.p_payload = jpegBuf;
        stream.decoderNal.timeStamp = getTimeUS();
        IMPF_VPU_JpegDecInputStream(&stream);
    }

    if(jpegBuf)
        free(jpegBuf);

    return NULL;
}

void *frameOutputThread(void *arg)
{
    int ret;
    IMPFrameInfo *frame;

    ret = IMPF_VPU_JpegDecStart();
    if (ret != 0) {
        printf("IMPF_VPU_JpegDecStart fail\n");
        return NULL;
    }

    while (demoThreadRunFlag) {
        ret = IMPF_VPU_JpegDecOutputGetFrame(&frame);
        if (ret != 0)
            continue;

        fwriteFileByName(DEC_DATA_PATH, (void *)frame->virAddr, calc_pic_size(frame->width, frame->height, frame->pixfmt));
        printf("jpeg decode success, wirte file(%s)\n", DEC_DATA_PATH);

        IMPF_VPU_JpegDecOutputReleaseFrame(frame);
    }

    IMPF_VPU_JpegDecStop();

    return NULL;
}


int main(void)
{
    int ret;

    ret = IMPF_VPU_JpegDecInit(&decChnAttr);
    if (ret != 0) {
        printf("IMPF_VPU_JpegDecInit fail\n");
        return -1;
    }

    demoThreadRunFlag = 1;
    pthread_t ptFrameOutput;
    ret = pthread_create(&ptFrameOutput, NULL, frameOutputThread, NULL);
    if (ret < 0) {
        printf("create frameOutputThread failed\n");
        return -1;
    }
    pthread_t ptStreamInput;
    ret = pthread_create(&ptStreamInput, NULL, streamInputThread, NULL);
    if (ret < 0) {
        printf("create streamInputThread failed\n");
        return -1;
    }

    while (1)
        sleep(10);

    demoThreadRunFlag = 0;
    pthread_join(ptStreamInput, NULL);
    pthread_join(ptFrameOutput, NULL);

    IMPF_VPU_JpegDecDeinit();

    return 0;
}
#endif
