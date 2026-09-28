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

#include "common.h"





int64_t getTimeUS(void)
{
    int64_t currTimeUS;
    struct timeval currTime;

    if (gettimeofday(&currTime, NULL) == -1) {
        printf("gettimeofday failed\n");
        return 0;
    }

    currTimeUS = (int64_t)currTime.tv_sec *1000000 + currTime.tv_usec;

    return currTimeUS;
}


size_t getFileSize(char *path)
{
    int ret;
    int fd;
    struct stat statbuf;

    if(NULL == path) {
        printf("getFileSize fail, path is NULL\n");
        return -1;
    }

#if 0
    if (access(path, F_OK) != 0) {
        printf("getFileSize fail, file is not exist\n");
        return -1;
    }
#endif

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("getFileSize open %s fail!\n", path);
        return -1;
    }

    ret = fstat(fd, &statbuf);
    if (ret < 0) {
        printf("fstat fail, %s\n", path);
        close(fd);
        return -1;
    }
    close(fd);

    return statbuf.st_size;
}


int readFileByName(char *path, void *buf, size_t size)
{
    size_t readSize;
    int fd;

    if(NULL == path) {
        printf("readFileByName fail, path is NULL\n");
        return -1;
    }
    if(NULL == buf) {
        printf("readFileByName fail, buf is NULL\n");
        return -1;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("readFileByName open %s fail!\n", path);
        return -1;
    }

    readSize = read(fd, buf, size);
    if (readSize != size) {
        printf("readFileByName write fail, %s, readSize(%d) != size(%d)\n", strerror(errno), readSize, size);
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}


int writeFileByName(char *path, void *buf, size_t size)
{
    size_t writedSize;
    int fd;

    if(NULL == path) {
        printf("writeFileByName fail, path is NULL\n");
        return -1;
    }

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (fd < 0) {
        printf("writeFileByName open %s fail!\n", path);
        return -1;
    }

    writedSize = write(fd, buf, size);
    if (writedSize != size) {
        printf("writeFileByName write fail, %s, writedSize(%d) != size(%d)\n", strerror(errno), writedSize, size);
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}


int fwriteFileByName(char *path, void *buf, size_t size)
{
    size_t writedSize;
    FILE *fp;

    if(NULL == path) {
        printf("fwriteFileByName fail, path is NULL\n");
        return -1;
    }

    fp = fopen(path,"wb");
    if (fp == NULL) {
        printf("fwriteFileByName fopen %s fail!\n",path);
        return -1;
    }
    writedSize = fwrite(buf, sizeof(char), size, fp);
    if(writedSize != size) {
        printf("fwriteFileByName fwrite fail, %s, writedSize(%d) != size(%d)\n", strerror(errno), writedSize, size);
        fclose(fp);
        return -1;
    }
    fclose(fp);

    return 0;
}


int writeStreamByName(char *path, IMPEncoderStream *stream)
{
    int i;
    size_t writedSize;
    int fd;

    if(NULL == path) {
        printf("writeStreamByName fail, path is NULL\n");
        return -1;
    }

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (fd < 0) {
        printf("writeStreamByName open %s fail!\n", path);
        return -1;
    }

    for (i = 0; i < stream->packCount; i++) {
        writedSize = write(fd, (void *)stream->pack[i].virAddr, stream->pack[i].length);
        if (writedSize != stream->pack[i].length) {
            close(fd);
            printf("writeStreamByName write fail, %s, writedSize(%d) != stream->pack[%d].length(%d)\n", strerror(errno), writedSize, i, stream->pack[i].length);
            return -1;
        }
    }
    close(fd);

    return 0;
}

int rectRgnCopyFromSrc(uint8_t *srcBuf, uint32_t srcWidth, uint32_t srcHeight,
                            uint8_t *dstBuf, uint32_t dstWidth, uint32_t dstHeight,
                            uint8_t bpp, rectRgn rgnInSrc)
{
    int i;
    uint32_t copyRowNum;
    uint32_t copyColSize;
    uint8_t *psrc;
    uint8_t *pdst;

    if (srcBuf == NULL || dstBuf == NULL){
        printf("srcBuf == NULL || dstBuf == NULL\n");
        return -1;
    }
    if (rgnInSrc.left >= rgnInSrc.right || rgnInSrc.top >= rgnInSrc.bottom){
        printf("error RectRgn\n");
        return -1;
    }

    /*
     * 说明：从源图片的(rectRgn.left, rectRgn.top)坐标扣出rectRgn区域大小的图像，放到目的(0, 0)坐标处
     */

    if (rgnInSrc.right > srcWidth)
        rgnInSrc.right = srcWidth;
    if (rgnInSrc.bottom > srcHeight)
        rgnInSrc.bottom = srcHeight;

    copyColSize = ((rgnInSrc.right-rgnInSrc.left) > dstWidth ? dstWidth : (rgnInSrc.right-rgnInSrc.left)) * bpp>>3;
    copyRowNum  = (rgnInSrc.bottom-rgnInSrc.top) > dstHeight ? dstHeight : (rgnInSrc.bottom-rgnInSrc.top);
    psrc = srcBuf + ((srcWidth*rgnInSrc.top + rgnInSrc.left) * bpp>>3);
    pdst = dstBuf;

    for (i=0; i<copyRowNum; i++) {
        memcpy(pdst, psrc, copyColSize);
        psrc += srcWidth * bpp>>3;
        pdst += dstWidth * bpp>>3;
    }
    return 0;
}


int rectRgnCopyToDst(uint8_t *srcBuf, uint32_t srcWidth, uint32_t srcHeight,
                            uint8_t *dstBuf, uint32_t dstWidth, uint32_t dstHeight,
                            uint8_t bpp, rectRgn rgnInDst)
{
    int i;
    uint32_t copyRowNum;
    uint32_t copyColSize;
    uint8_t *psrc;
    uint8_t *pdst;

    if (srcBuf == NULL || dstBuf == NULL){
        printf("srcBuf == NULL || dstBuf == NULL\n");
        return -1;
    }
    if (rgnInDst.left >= rgnInDst.right || rgnInDst.top >= rgnInDst.bottom){
        printf("error RectRgn\n");
        return -1;
    }

    /*
     * 说明：从源图片的(0, 0)坐标扣出rectRgn区域大小的图像，放到目的(rectRgn.left, rectRgn.top)坐标处
     */

    if (rgnInDst.right - rgnInDst.left > srcWidth)
        rgnInDst.right = rgnInDst.left + srcWidth;
    if (rgnInDst.bottom - rgnInDst.top > srcHeight)
        rgnInDst.bottom = rgnInDst.top + srcWidth;

    copyColSize = (rgnInDst.right > dstWidth ? (dstWidth-rgnInDst.left) : (rgnInDst.right-rgnInDst.left)) * bpp>>3;
    copyRowNum  = rgnInDst.bottom > dstHeight ? (dstHeight-rgnInDst.top) : (rgnInDst.bottom-rgnInDst.top);
    psrc = srcBuf;
    pdst = dstBuf + ((dstWidth*rgnInDst.top + rgnInDst.left) * bpp>>3);

    for (i=0; i<copyRowNum; i++) {
        memcpy(pdst, psrc, copyColSize);
        psrc += srcWidth * bpp>>3;
        pdst += dstWidth * bpp>>3;
    }
    return 0;
}

