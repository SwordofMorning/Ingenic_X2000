/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Ingenic Media Development Kit(IMDK)
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>

#include <isp_tuning_x2580.h>


#define TISP_TUNING_CID_PRIVATE_BASE  0x08000000

/* isp core tuning */
enum isp_tuning_private_cmd_id {
    ISP_TUNING_CID_STATIS_CONFIG = TISP_TUNING_CID_PRIVATE_BASE,
    ISP_TUNING_CID_AWB_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10,
    ISP_TUNING_CID_AWB_STATIS,
    ISP_TUNING_CID_AWB_WEIGHT,
    ISP_TUNING_CID_AWB_GLOBAL_STATIS,
    ISP_TUNING_CID_FACE_AWB_CONTROL,
    ISP_TUNING_CID_AE_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 2,
    ISP_TUNING_CID_AE_WEIGHT,
    ISP_TUNING_CID_AE_STATIS,
    ISP_TUNING_CID_AE_EXPR_INFO,
    ISP_TUNING_CID_AE_SCENCE_ATTR,
    ISP_TUNING_CID_GAMMA_ATTR,
    ISP_TUNING_CID_AE_ANTIFLICKER_ATTR,
    ISP_TUNING_CID_AE_EXP_LIST,
    ISP_TUNING_CID_AF_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 3,
    ISP_TUNING_CID_AF_STATIS,
    ISP_TUNING_CID_AF_WEIGHT,
    ISP_TUNING_CID_SENSOR_ATTR_CONTROL,
    ISP_TUNING_CID_AF_METRIC_INFO,
    ISP_TUNING_CID_FACE_AE_CONTROL,
    ISP_TUNING_CID_DYNAMIC_DP_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 4,
    ISP_TUNING_CID_STATIC_DP_ATTR,
    ISP_TUNING_CID_WDR_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 5,
    ISP_TUNING_CID_ENABLE_DRC,
    ISP_TUNING_CID_ENABLE_DEFOG,
    ISP_TUNING_CID_CUSTOM_ANTI_FOG,
    ISP_TUNING_CID_WDR_OUTPUT_MODE,
    ISP_TUNING_CID_SHARP_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 6,
    ISP_TUNING_CID_DEMO_ATTR,
    ISP_TUNING_CID_CONTROL_FPS= TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 7,
    ISP_TUNING_CID_DAY_OR_NIGHT,
    ISP_TUNING_CID_MODULE_CONTROL,
    ISP_TUNING_CID_HV_FLIP,
    ISP_TUNING_CID_MASK_BLOCK_ATTR,
    ISP_TUNING_CID_EV_START,
    ISP_TUNING_CID_ISP_CUST_MODE,
    ISP_TUNING_CID_AUTOZOOM_CONTROL,
    ISP_TUNING_CID_CCM_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 8,
    ISP_TUNING_CID_BCSH_HUE,
    ISP_TUNING_CID_DIS_STAINFO = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 9,
    ISP_TUNING_CID_ISP_WAIT_FRAME_ATTR,
    ISP_TUNING_CID_BRIGHTNESS,
    ISP_TUNING_CID_SHARPNESS,
    ISP_TUNING_CID_SATURATION,
    ISP_TUNING_CID_CONTRAST,
    ISP_TUNING_CID_CSC_ATTR,
    ISP_TUNING_CID_CSCCR_ATTR,

    ISP_TUNING_CID_DRAW_BLOCK_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 10,
    ISP_TUNING_CID_OSD_ATTR,
    ISP_TUNING_CID_OSD_BLOCK_ATTR,
    ISP_TUNING_CID_CUSTOM_WDR,
    ISP_TUNING_CID_MODULE_RATIO,
    ISP_TUNING_CID_SWITCH_BIN,
    ISP_TUNING_CID_SCALER_LEVEL,
};


int isp_tuning_open(const char *dev)
{
    int fd;

    fd = open(dev, (O_RDWR | O_CLOEXEC), 0);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s, %s\n", dev, strerror(errno));
        return -ENODEV;
    }

    return fd;
}

int isp_tuning_close(int fd)
{
    return close(fd);
}


int isp_tuning_get_module_control(int fd, ISPModuleCtl *isp_module)
{
    struct v4l2_control ctrl;
    int ret;

    if (isp_module == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_MODULE_CONTROL;
    ctrl.value = (uint32_t)isp_module;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_module_control(int fd, ISPModuleCtl *isp_module)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = ISP_TUNING_CID_MODULE_CONTROL;
    ctrl.value = (uint32_t)isp_module;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_isp_running_mode(int fd, isp_running_mode *mode)
{
    struct v4l2_control ctrl;
    int ret;

    if (mode == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_DAY_OR_NIGHT;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (!ret)
        *mode = ctrl.value;
    else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_isp_running_mode(int fd, isp_running_mode *mode)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = ISP_TUNING_CID_DAY_OR_NIGHT;
    ctrl.value = (int)mode;
    fprintf(stderr, "%s:  cmd=0x%08x\n", __func__, ctrl.id);
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_isp_hvflip(int fd, tisp_hv_flip_t *mode)
{
    struct v4l2_control ctrl;
    int ret;

    if (mode == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_HV_FLIP;
    ctrl.value = (int)mode;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if(0 != ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_isp_hvflip(int fd, tisp_hv_flip_t *mode)
{
    struct v4l2_control ctrl;
    int32_t ret = 0;
    if (mode == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_HV_FLIP;
    ctrl.value =(int)mode;

    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret < 0)
        fprintf(stderr, "%s(%d), set VIDIOC_S_CTRL failed ret %d\n", __func__, __LINE__, ret);

    return ret;
}

typedef struct {
    int32_t CscCoef[9];
    unsigned char CscOffset[2];
    unsigned char CscClip[4];
} isp_csc_matrix;
typedef struct {
    ISPCSCColorGamut ColorGamut;
    isp_csc_matrix Matrix;
    isp_csc_matrix MatrixIn;
} isp_csc_attrInter;

int32_t isp_tuning_set_isp_csc_attr(int fd, isp_csc_attr *csc)
{
    int32_t ret = 0;
    struct v4l2_control ctrl;
    isp_csc_attrInter attr_inter;

    if(csc->ColorGamut < ISP_CG_BT601_FULL || csc->ColorGamut >= ISP_CG_BUTT) {
         fprintf(stderr, "isp_tuning_set_ispCSCAttr (%d) is overflow!\n", csc->ColorGamut);
    }

    attr_inter.ColorGamut = csc->ColorGamut;
    if (csc->ColorGamut == ISP_CG_USER){
        int32_t i = 0;
        for(i = 0; i < 9; i++)
            attr_inter.Matrix.CscCoef[i] = (int)(csc->Matrix.CscCoef[i] * 1024 + 0.5) & 0x3ff;
        for(i = 0; i < 2; i++)
            attr_inter.Matrix.CscOffset[i] = csc->Matrix.CscOffset[i];
        for(i = 0; i < 4; i++)
            attr_inter.Matrix.CscClip[i] = csc->Matrix.CscClip[i];
    }

    ctrl.id = ISP_TUNING_CID_CSC_ATTR;
    ctrl.value = (uint32_t)(&attr_inter);
    ret = ioctl (fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if(ret < 0)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));
    return ret;
}

int32_t isp_tuning_get_isp_csc_attr(int fd, isp_csc_attr *csc)
{
    struct v4l2_control ctrl;
    isp_csc_attrInter csc_attr;
    int32_t ret = 0;


    ctrl.id = ISP_TUNING_CID_CSC_ATTR;
    ctrl.value = (uint32_t)(&csc_attr);
    ret = ioctl (fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if(!ret){
        if(csc_attr.ColorGamut == ISP_CG_USER){
            int32_t i = 0;
            for(i = 0; i < 9; i++)
                csc->Matrix.CscCoef[i] = (float)csc_attr.Matrix.CscCoef[i] / 1024;

            csc->Matrix.CscCoef[3] = -csc->Matrix.CscCoef[3];
            csc->Matrix.CscCoef[4] = -csc->Matrix.CscCoef[4];
            csc->Matrix.CscCoef[7] = -csc->Matrix.CscCoef[7];
            csc->Matrix.CscCoef[8] = -csc->Matrix.CscCoef[8];

            for(i = 0; i < 2; i++)
                csc->Matrix.CscOffset[i] = csc_attr.Matrix.CscOffset[i];
            for(i = 0; i < 4; i++)
                csc->Matrix.CscClip[i] = csc_attr.Matrix.CscClip[i];

        } else {
            memset(csc, 0, sizeof(isp_csc_attr));
        }
        csc->ColorGamut = csc_attr.ColorGamut;
    } else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}


typedef struct {
    char ManualEn;              /* CCM Manual enable ctrl */
    char SatEn;                 /* CCM Saturation enable ctrl */
    int32_t ColorMatrix[9];     /* color matrix on manual mode */
} isp_ccm_ctl_attr;

int32_t isp_tuning_set_isp_ccm_attr(int fd, isp_ccm_attr *ccm)
{
    int32_t ret = 0;
    int32_t i = 0;
    isp_ccm_ctl_attr ccm_attr;
    struct v4l2_control ctrl;

    for(i = 0; i < 9; i++) {
        if (ccm->ColorMatrix[i] < -0.00001){
            ccm->ColorMatrix[i] = -ccm->ColorMatrix[i];
            ccm_attr.ColorMatrix[i] = (((~((int)(ccm->ColorMatrix[i] * 1024))) + 1) | 0x2000) & 0x3fff;
        } else
            ccm_attr.ColorMatrix[i] = (int)(ccm->ColorMatrix[i] * 1024) & 0x1fff;
    }

    ccm_attr.ManualEn = ccm->ManualEn;
    ccm_attr.SatEn = ccm->SatEn;

    ctrl.id = ISP_TUNING_CID_CCM_ATTR;
    ctrl.value = (uint32_t)(&ccm_attr);
    ret = ioctl (fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if(0 != ret) {
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));
    }

    return ret;
}

int32_t isp_tuning_get_isp_ccm_attr(int fd, isp_ccm_attr *ccm)
{
    int32_t ret = 0;
    int32_t i = 0;
    struct v4l2_control ctrl;
    isp_ccm_ctl_attr ccm_attr;

    ctrl.id = ISP_TUNING_CID_CCM_ATTR;
    ctrl.value = (uint32_t)(&ccm_attr);
    ret = ioctl (fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);

    for(i = 0; i < 9; i++) {
        if ((ccm_attr.ColorMatrix[i]) & 0x2000)
            ccm->ColorMatrix[i] = -(float)((((~(ccm_attr.ColorMatrix[i])) + 1) & 0x1fff)) / 1024;
        else
            ccm->ColorMatrix[i] = (float)(ccm_attr.ColorMatrix[i]) / 1024;
    }
    ccm->ManualEn = ccm_attr.ManualEn;
    ccm->SatEn = ccm_attr.SatEn;

    if(0 != ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}


int32_t isp_tuning_set_isp_auto_zoom(int fd, isp_auto_zoom *ispautozoom)
{
    int32_t ret = 0;
    struct v4l2_control ctrl;

    ctrl.id = ISP_TUNING_CID_AUTOZOOM_CONTROL;
    ctrl.value = (uint32_t)ispautozoom;
    ret = ioctl (fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if(0 != ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int32_t isp_tuning_get_isp_auto_zoom(int fd, isp_auto_zoom *ispautozoom)
{
    int32_t ret  = 0;
    struct v4l2_control ctrl;

    ctrl.id = ISP_TUNING_CID_AUTOZOOM_CONTROL;
    ctrl.value = (uint32_t)ispautozoom;
    ret = ioctl (fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if(0 != ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}


int32_t isp_tuning_set_sensor_fps(int fd, isp_sensor_fps *fps)
{
    int32_t ret  = 0;
    struct v4l2_control ctrl;
    int32_t tmp = 0;

    tmp = (fps->num << 16) | fps->den;

    ctrl.id = ISP_TUNING_CID_CONTROL_FPS;
    ctrl.value = tmp;
    ret = ioctl (fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if(0 != ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int32_t isp_tuning_get_sensor_fps(int fd, isp_sensor_fps *fps)
{
    int32_t ret = 0;
    struct v4l2_control ctrl;

    ctrl.id = ISP_TUNING_CID_CONTROL_FPS;
    ret = ioctl (fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if(0 != ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    fps->num =  (ctrl.value >> 16) & 0xffff;
    fps->den = ctrl.value & 0xffff;

    return ret;
}

int isp_tuning_get_anti_flicker_attr(int fd, ISPAntiflickerAttr *flickerattr)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.value = (int)flickerattr;
    ctrl.id = ISP_TUNING_CID_AE_ANTIFLICKER_ATTR;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if(0 != ret) {
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));
    }

    return ret;
}

int isp_tuning_set_anti_flicker_attr(int fd, ISPAntiflickerAttr *flickerattr)
{
    struct v4l2_control ctrl;
    int ret;

    if(flickerattr->mode < ISP_ANTIFLICKER_DISABLE_MODE || flickerattr->mode >= ISP_ANTIFLICKER_BUTT) {
        fprintf(stderr, "Antiflicker mode(%d) overflow!!!!\n", flickerattr->mode);
    }

    if(flickerattr->mode != ISP_ANTIFLICKER_DISABLE_MODE) {
        if(flickerattr->freq < 0 || flickerattr->freq > 255){
            fprintf(stderr, "Antiflicker freq(%d) overflow!!!!\n", flickerattr->freq);
        }
    }

    ctrl.id = ISP_TUNING_CID_AE_ANTIFLICKER_ATTR;
    ctrl.value = (int)flickerattr;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (0 != ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_ae_weight(int fd, isp_weight *ae_weight)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_weight == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_WEIGHT;
    ctrl.value = (uint32_t)ae_weight;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_ae_weight(int fd, isp_weight *ae_weight)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_weight == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_WEIGHT;
    ctrl.value = (uint32_t)ae_weight;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_gamma(int fd, isp_gamma *gamma)
{
    struct v4l2_control ctrl;
    int ret;

    if (gamma == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_GAMMA_ATTR;
    ctrl.value = (int)gamma;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_gamma(int fd, isp_gamma *gamma)
{
    struct v4l2_control ctrl;
    int ret;

    if (gamma == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_GAMMA_ATTR;
    ctrl.value = (int)gamma;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_sensor_attr(int fd, isp_sensor_attr *sensor_attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (sensor_attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_SENSOR_ATTR_CONTROL;
    ctrl.value = (int)sensor_attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}
    // AWB
int isp_tuning_get_awb_weight(int fd, isp_weight *weight)
{
    struct v4l2_control ctrl;
    int ret;

    if (weight == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AWB_WEIGHT;
    ctrl.value = (int)weight;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_awb_weight(int fd, isp_weight *weight)
{
    struct v4l2_control ctrl;
    int ret;

    if (weight == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AWB_WEIGHT;
    ctrl.value = (int)weight;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_awb_attr(int fd, isp_awb_attr *awb_attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (awb_attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AWB_ATTR;
    ctrl.value = (int)awb_attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_awb_attr(int fd, isp_awb_attr *awb_attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (awb_attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AWB_ATTR;
    ctrl.value = (int)awb_attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_awb_statis_zone(int fd, isp_awb_statis_info *awb_statis_info)
{
    struct v4l2_control ctrl;
    int ret;

    if (awb_statis_info == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AWB_STATIS;
    ctrl.value = (int)awb_statis_info;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_awb_global_statis(int fd, isp_awb_statics_global *awb_statics_global)
{
    struct v4l2_control ctrl;
    int ret;

    if (awb_statics_global == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AWB_GLOBAL_STATIS;
    ctrl.value = (int)awb_statics_global;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_isp_ae_scence_attr(int fd, isp_ae_scence_attr *ae_scence_attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_scence_attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_SCENCE_ATTR;
    ctrl.value = (int)ae_scence_attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_isp_ae_scence_attr(int fd, isp_ae_scence_attr *ae_scence_attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_scence_attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_SCENCE_ATTR;
    ctrl.value = (int)ae_scence_attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_isp_ae_expr_info(int fd, isp_ae_expr_info *ae_expr_info)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_expr_info == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_EXPR_INFO;
    ctrl.value = (int)ae_expr_info;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_isp_ae_expr_info(int fd, isp_ae_expr_info *ae_expr_info)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_expr_info == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_EXPR_INFO;
    ctrl.value = (int)ae_expr_info;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_isp_ae_statis_attr(int fd, isp_ae_statis_attr *ae_statis_attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_statis_attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_STATIS;
    ctrl.value = (int)ae_statis_attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_isp_ae_weight(int fd, isp_weight_attr *weight_attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (weight_attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_WEIGHT;
    ctrl.value = (int)weight_attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_isp_ae_weight(int fd, isp_weight_attr *weight_attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (weight_attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_WEIGHT;
    ctrl.value = (int)weight_attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_isp_statis_config(int fd, isp_statis_config *statis_config)
{
    struct v4l2_control ctrl;
    int ret;

    if (statis_config == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_STATIS_CONFIG;
    ctrl.value = (int)statis_config;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_isp_statis_config(int fd, isp_statis_config *statis_config)
{
    struct v4l2_control ctrl;
    int ret;

    if (statis_config == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_STATIS_CONFIG;
    ctrl.value = (int)statis_config;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_isp_ae_list_attr(int fd, isp_ae_list_attr *ae_list_attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_list_attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_STATIS_CONFIG;
    ctrl.value = (int)ae_list_attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_isp_ae_list_attr(int fd, isp_ae_list_attr *ae_list_attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_list_attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_STATIS_CONFIG;
    ctrl.value = (int)ae_list_attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}