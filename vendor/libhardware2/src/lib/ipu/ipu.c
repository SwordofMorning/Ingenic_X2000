#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <stdint.h>

#include <libhardware2/ipu.h>
#include <libhardware2/ipu_utils.h>

#define JZIPU_IOC_MAGIC                 'I'

#define IOCTL_IPU_START                 _IO(JZIPU_IOC_MAGIC, 106)
#define IOCTL_IPU_RES_PBUFF             _IO(JZIPU_IOC_MAGIC, 114)
#define IOCTL_IPU_GET_PBUFF             _IO(JZIPU_IOC_MAGIC, 115)
#define IOCTL_IPU_BUF_LOCK              _IO(JZIPU_IOC_MAGIC, 116)
#define IOCTL_IPU_BUF_UNLOCK            _IO(JZIPU_IOC_MAGIC, 117)
#define IOCTL_IPU_BUF_FLUSH_CACHE       _IO(JZIPU_IOC_MAGIC, 118)

#define IPU_DEV_NAME                    "/dev/ipu"

static inline void ipu_err(const char *err_msg)
{
    fprintf(stderr, "ipu: failed to %s, %s\n", err_msg, strerror(errno));
}

static unsigned int fmt_is_yuv(int bg_fmt)
{
    unsigned int is_yuv = 0;
    switch (bg_fmt) {
    case HAL_PIXEL_FORMAT_RGBA_5551:
    case HAL_PIXEL_FORMAT_BGRA_5551:
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_ARGB_8888:
    case HAL_PIXEL_FORMAT_ABGR_8888:
    case HAL_PIXEL_FORMAT_RGBA_4444:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RGB_565:
        is_yuv = 0;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_422_P:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
    case HAL_PIXEL_FORMAT_JZ_YUV_420_P:
    case HAL_PIXEL_FORMAT_YCbCr_420_B:
    case HAL_PIXEL_FORMAT_JZ_YUV_420_B:
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_YCbCr_420_I:
    case HAL_PIXEL_FORMAT_NV21:
    case HAL_PIXEL_FORMAT_NV12:
        is_yuv = 1;
        break;
    default:
        is_yuv = 0;
        break;
    }

    return is_yuv;
}


/* The interface _ipu_set_osdx_para was used for the background format must be YUV,
if was RGB should be change CSCCTL_SET */
static unsigned int ipu_set_osdx_para(unsigned int *chx_para, unsigned int chx_fmt, unsigned int chx_alpha, unsigned int bg_fmt)
{
    int ret = 0;
    unsigned int is_yuv = 0;
    unsigned int para = 0x020347f9;

    if((chx_alpha & (0x1<<8)) > 0) {
        /* disable gAlphaEn use pixel chx_alpha */
        para = 0x020347f9;
        chx_alpha = chx_alpha & 0xff;
    } else {
        if ((chx_fmt == HAL_PIXEL_FORMAT_NV12) || (chx_fmt == HAL_PIXEL_FORMAT_NV21))
            para = 0x020347fb;
        else
            para = 0x020347fd;
    }

    is_yuv = fmt_is_yuv(bg_fmt);
    switch(chx_fmt) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
            if(is_yuv)
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 2);
            else
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 0);
            IPU_OSD_CHX_PARA_PICTYPE_SET(para, 0);
            IPU_OSD_CHX_PARA_ARGBTYPE_SET(para, 8);
            break;
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_BGRX_8888:
            if(is_yuv){
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 2);
            }else {
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 0);
            }
            IPU_OSD_CHX_PARA_PICTYPE_SET(para, 0);
            IPU_OSD_CHX_PARA_ARGBTYPE_SET(para, 0xd);
            break;
        case HAL_PIXEL_FORMAT_ARGB_8888:
            if(is_yuv)
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 2);
            else
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 0);
            IPU_OSD_CHX_PARA_PICTYPE_SET(para, 0);
            IPU_OSD_CHX_PARA_ARGBTYPE_SET(para, 0);
            break;
        case HAL_PIXEL_FORMAT_ABGR_8888:
            if(is_yuv)
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 2);
            else
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 0);
            IPU_OSD_CHX_PARA_PICTYPE_SET(para, 0);
            IPU_OSD_CHX_PARA_ARGBTYPE_SET(para, 5);
            break;
        case HAL_PIXEL_FORMAT_NV12:
            if(is_yuv){
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 0);
            }
            else {
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 1);
            }
            IPU_OSD_CHX_PARA_PICTYPE_SET(para, 2);
            break;
        case HAL_PIXEL_FORMAT_NV21:
            if(is_yuv)
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 0);
            else
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 1);
            IPU_OSD_CHX_PARA_PICTYPE_SET(para, 3);
            break;
        case HAL_PIXEL_FORMAT_RGBA_5551:
            if(is_yuv)
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 2);
            else
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 0);
            IPU_OSD_CHX_PARA_PICTYPE_SET(para, 4);
            IPU_OSD_CHX_PARA_ARGBTYPE_SET(para, 8);
            break;
        case HAL_PIXEL_FORMAT_BGRA_5551:
            if(is_yuv)
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 2);
            else
                IPU_OSD_CHX_PARA_CSCCTL_SET(para, 0);
            IPU_OSD_CHX_PARA_PICTYPE_SET(para, 4);
            IPU_OSD_CHX_PARA_ARGBTYPE_SET(para, 0xd);
            break;
        default:
            printf("ipu: err osd chx_fmt not support \n");
            ret = -1;
            break;
    }

    IPU_OSD_CHX_PARA_ALPHA_SET(para, chx_alpha);
    *chx_para = para;
    return ret;
}

static void ipu_get_para(ipu_param_t* ipu_param)
{
    int ch = 0;
    for ( ch = IPU_CMD_OSD_CH0; ch <= IPU_CMD_OSD_CH3; ch++) {
        if (ipu_param->cmd & (1<<ch)) {
            ipu_set_osdx_para(&(ipu_param->ipu_osdx_param[ch].osd_chx_para), \
                                ipu_param->ipu_osdx_param[ch].osd_chx_fmt, \
                                ipu_param->ipu_osdx_param[ch].osd_chx_bak_argb, \
                                ipu_param->bg_fmt);
        }
    }
}

int ipu_open(void)
{
    int fd = 0;

    fd = open(IPU_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ipu_err("open device");
        return -1;
    }
    return fd;
}

int ipu_close(int fd)
{
    return close(fd);
}

static int ipu_lock_buf(int fd, const ipu_param_t* ipu_param)
{
    int ret = 0;
    ret = ioctl(fd, IOCTL_IPU_BUF_LOCK, (void *)ipu_param);
    if (ret) {
        ipu_err("IPU_BUF_LOCK\n");
    }
    return ret;
}

static int ipu_unlock_buf(int fd, const ipu_param_t* ipu_param)
{
    int ret = 0;
    ret = ioctl(fd, IOCTL_IPU_BUF_UNLOCK, (void *)ipu_param);
    if (ret < 0) {
        ipu_err("IPU_BUF_UNLOCK\n");
    }
    return ret;
}

static int ipu_start(int fd, const ipu_param_t* ipu_param)
{
    int ret = 0;
    ret = ioctl(fd, IOCTL_IPU_START, (void *)ipu_param);
    if (ret < 0) {
        ipu_err("IPU_START\n");
    }
    return ret;
}

int ipu_start_draw(const int fd, const ipu_param_t *ipu_param)
{
    int ret = 0;

    if (ipu_param->cmd & IPU_CMD_OSD_FLAG) {
        ipu_get_para((ipu_param_t *)ipu_param);
    }

    ret = ipu_lock_buf(fd, ipu_param);
    if (ret) {
        ipu_err("ipu_lock_buf fail!\n");
        goto err;
    }

    ret = ipu_start(fd, ipu_param);
    if (ret) {
        ipu_err("ipu_start fail!\n");
        goto err;
    }

    ret = ipu_unlock_buf(fd, ipu_param);
    if (ret) {
        ipu_err("ipu_unlock_buf fail!\n");
        goto err;
    }

err:
    return ret;
}

