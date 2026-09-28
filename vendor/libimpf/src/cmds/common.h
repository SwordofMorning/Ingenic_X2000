#ifndef __COMMON_H__
#define __COMMON_H__


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <impf/media_manager.h>



/**
 * 矩形区域定义
 */
typedef struct {
    uint32_t left;       /* 左上角X坐标 */
    uint32_t top;        /* 左上角Y坐标 */
    uint32_t right;      /* 右下角X坐标 */
    uint32_t bottom;     /* 右下角Y坐标 */
} rectRgn;






int64_t getTimeUS(void);

size_t getFileSize(char *path);

int readFileByName(char *path, void *buf, size_t size);

int writeFileByName(char *path, void *buf, size_t size);

int fwriteFileByName(char *path, void *buf, size_t size);

int writeStreamByName(char *path, IMPEncoderStream *stream);

int rectRgnCopyFromSrc(uint8_t *srcBuf, uint32_t srcWidth, uint32_t srcHeight,
                            uint8_t *dstBuf, uint32_t dstWidth, uint32_t dstHeight,
                            uint8_t bpp, rectRgn rgnInSrc);

int rectRgnCopyToDst(uint8_t *srcBuf, uint32_t srcWidth, uint32_t srcHeight,
                            uint8_t *dstBuf, uint32_t dstWidth, uint32_t dstHeight,
                            uint8_t bpp, rectRgn rgnInDst);



#endif
