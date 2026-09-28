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

#include <isp_tuning.h>




/* isp core tuning */
enum isp_tuning_private_cmd_id {
    ISP_TUNING_CID_MODULE_CONTROL       = V4L2_CID_PRIVATE_BASE,
    ISP_TUNING_CID_DAY_OR_NIGHT,
    ISP_TUNING_CID_CONTROL_FPS,
    ISP_TUNING_CID_AE                   = V4L2_CID_PRIVATE_BASE + 0x40 * 1,
    ISP_TUNING_CID_AE_ZONE,
    ISP_TUNING_CID_AE_HIST,
    ISP_TUNING_CID_AE_ROI,
    ISP_TUNING_CID_AE_WEIGHT,
    ISP_TUNING_CID_EV_ATTR,
    ISP_TUNING_CID_EXPR_ATTR,
    ISP_TUNING_CID_AGAIN_ATTR,
    ISP_TUNING_CID_MAX_AGAIN,
    ISP_TUNING_CID_MAX_DGAIN,
    ISP_TUNING_CID_TOTAL_GAIN,
    ISP_TUNING_CID_AE_COMP,
    ISP_TUNING_CID_AE_MIN,
    ISP_TUNING_CID_EV_START,
    ISP_TUNING_CID_AE_LUMA,
    ISP_TUNING_CID_HILIGHT_DEPRESS_STRENGTH,
    ISP_TUNING_CID_AWB                  = V4L2_CID_PRIVATE_BASE + 0x40 * 2,
    ISP_TUNING_CID_WB_ATTR,
    ISP_TUNING_CID_WB_STATIS_GOL_ATTR,
    ISP_TUNING_CID_WB_STATIS_ATTR,
    ISP_TUNING_CID_AWB_WEIGHT,
    ISP_TUNING_CID_AWB_HIST,
    ISP_TUNING_CID_AF                   = V4L2_CID_PRIVATE_BASE + 0x40 * 3,
    ISP_TUNING_CID_AF_HIST,
    ISP_TUNING_CID_AF_WEIGHT,
    ISP_TUNING_CID_AF_METRIC,
    ISP_TUNING_CID_GAMMA                = V4L2_CID_PRIVATE_BASE + 0x40 * 4,
    ISP_TUNING_CID_GAMMA_ATTR,
    ISP_TUNING_CID_DMSC                 = V4L2_CID_PRIVATE_BASE + 0x40 * 5,
    ISP_TUNING_CID_DMSC_ATTR,
    ISP_TUNING_CID_FC_ATTR,
    ISP_TUNING_CID_MDNS                 = V4L2_CID_PRIVATE_BASE + 0x40 * 6,
    ISP_TUNING_CID_GET_NCU_INFO,
    ISP_TUNING_CID_3DNS_RATIO,
    ISP_TUNING_CID_ADR                  = V4L2_CID_PRIVATE_BASE + 0x40 * 7,
    ISP_TUNING_CID_ADR_RATIO,
    ISP_TUNING_CID_SDNS                 = V4L2_CID_PRIVATE_BASE + 0x40 * 8,
    ISP_TUNING_CID_DPC                  = V4L2_CID_PRIVATE_BASE + 0x40 * 9,
    ISP_TUNING_CID_BLC                  = V4L2_CID_PRIVATE_BASE + 0x40 * 10,
    ISP_TUNING_CID_LSC                  = V4L2_CID_PRIVATE_BASE + 0x40 * 11,
    ISP_TUNING_CID_CCM                  = V4L2_CID_PRIVATE_BASE + 0x40 * 12,
    ISP_TUNING_CID_WDR                  = V4L2_CID_PRIVATE_BASE + 0x40 * 13,
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


int isp_tuning_get_module_control(int fd, isp_module_ctrl *isp_module)
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

int isp_tuning_set_module_control(int fd, isp_module_ctrl *isp_module)
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

int isp_tuning_set_isp_running_mode(int fd, isp_running_mode mode)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = ISP_TUNING_CID_DAY_OR_NIGHT;
    ctrl.value = mode;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_isp_hflip(int fd, isp_tuning_ops_mode *mode)
{
    struct v4l2_control ctrl;
    int ret;

    if (mode == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = V4L2_CID_HFLIP;
    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if (!ret)
        *mode = ctrl.value;
    else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_isp_hflip(int fd, isp_tuning_ops_mode mode)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = V4L2_CID_HFLIP;
    ctrl.value = mode;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        fprintf(stderr, "%s(%d), set VIDIOC_S_CTRL failed\n", __func__, __LINE__);

    return ret;
}

int isp_tuning_get_isp_vflip(int fd, isp_tuning_ops_mode *mode)
{
    struct v4l2_control ctrl;
    int ret;

    if (mode == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = V4L2_CID_VFLIP;
    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if (!ret)
        *mode = ctrl.value;
    else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_isp_vflip(int fd, isp_tuning_ops_mode mode)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = V4L2_CID_VFLIP;
    ctrl.value = mode;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_sensor_fps(int fd, uint32_t *fps_num, uint32_t *fps_den)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = ISP_TUNING_CID_CONTROL_FPS;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (!ret) {
        *fps_num = (ctrl.value >> 16) & 0xffff;
        *fps_den = ctrl.value & 0xffff;
    } else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_sensor_fps(int fd, uint32_t fps_num, uint32_t fps_den)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = ISP_TUNING_CID_CONTROL_FPS;
    ctrl.value = (fps_num << 16) | fps_den;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_brightness(int fd, unsigned char *brightness)
{
    struct v4l2_control ctrl;
    int ret;

    if (brightness == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = V4L2_CID_BRIGHTNESS;
    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if (!ret) {
        *brightness = ctrl.value;
    } else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_brightness(int fd, unsigned char brightness)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = V4L2_CID_BRIGHTNESS;
    ctrl.value = brightness;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_contrast(int fd, unsigned char *contrast)
{
    int ret;

    if (contrast == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_CONTRAST;
    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if (!ret) {
        *contrast = ctrl.value;
    } else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_contrast(int fd, unsigned char contrast)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = V4L2_CID_CONTRAST;
    ctrl.value = contrast;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_saturation(int fd, unsigned char *saturation)
{
    int ret;

    if (saturation == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_SATURATION;
    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if (!ret) {
        *saturation = ctrl.value;
    } else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_saturation(int fd, unsigned char saturation)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = V4L2_CID_SATURATION;
    ctrl.value = saturation;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_sharpness(int fd, unsigned char *sharpness)
{
    int ret;

    if (sharpness == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_SHARPNESS;
    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if (!ret) {
        *sharpness = ctrl.value;
    } else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_sharpness(int fd, unsigned char sharpness)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = V4L2_CID_SHARPNESS;
    ctrl.value = sharpness;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_anti_flicker_attr(int fd, enum v4l2_power_line_frequency *attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if (!ret) {
        *attr = ctrl.value;
    } else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_anti_flicker_attr(int fd, enum v4l2_power_line_frequency attr)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
    ctrl.value = attr;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_ev_attr(int fd, isp_ev_attr *attr)
{
    struct v4l2_control ctrl;
    int ret;

    if (attr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_EV_ATTR;
    ctrl.value = (uint32_t)attr;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_expr(int fd, isp_expr *expr)
{
    struct v4l2_control ctrl;
    int ret;

    if (expr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_EXPR_ATTR;
    ctrl.value = (uint32_t)expr;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_expr(int fd, isp_expr *expr)
{
    struct v4l2_control ctrl;
    int ret;

    if (expr == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_EXPR_ATTR;
    ctrl.value = (uint32_t)expr;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_max_again(int fd, uint32_t *gain)
{
    struct v4l2_control ctrl;
    int ret;

    if (gain == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_MAX_AGAIN;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (!ret)
        *gain = ctrl.value;
    else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_max_again(int fd, uint32_t gain)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = ISP_TUNING_CID_MAX_AGAIN;
    ctrl.value = gain;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_max_dgain(int fd, uint32_t *gain)
{
    struct v4l2_control ctrl;
    int ret;

    if (gain == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_MAX_DGAIN;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (!ret)
        *gain = ctrl.value;
    else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_max_dgain(int fd, uint32_t gain)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = ISP_TUNING_CID_MAX_DGAIN;
    ctrl.value = gain;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_total_gain(int fd, uint32_t *gain)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = ISP_TUNING_CID_TOTAL_GAIN;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (!ret)
        *gain = ctrl.value;
    else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_ae_min(int fd, isp_ae_min *ae_min)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_min == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_MIN;
    ctrl.value = (uint32_t)ae_min;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_ae_min(int fd, isp_ae_min *ae_min)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_min == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_MIN;
    ctrl.value = (uint32_t)ae_min;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_ae_luma(int fd, int *luma)
{
    struct v4l2_control ctrl;
    int ret;

    if (luma == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_LUMA;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (!ret)
        *luma = ctrl.value;
    else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_hi_light_depress(int fd, uint32_t *strength)
{
    struct v4l2_control ctrl;
    int ret;

    if (strength == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_HILIGHT_DEPRESS_STRENGTH;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (!ret)
        *strength = ctrl.value;
    else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_hi_light_depress(int fd, uint32_t strength)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = ISP_TUNING_CID_HILIGHT_DEPRESS_STRENGTH;
    ctrl.value = strength;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_ae_zone(int fd, isp_zone *ae_zone)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_zone == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_ZONE;
    ctrl.value = (uint32_t)ae_zone;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_ae_hist(int fd, isp_ae_hist *ae_hist)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_hist == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_HIST;
    ctrl.value = (uint32_t)ae_hist;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_ae_hist(int fd, isp_ae_hist *ae_hist)
{
    struct v4l2_control ctrl;
    int ret;

    if (ae_hist == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_HIST;
    ctrl.value = (uint32_t)ae_hist;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_ae_roi(int fd, isp_weight *roi_weight)
{
    struct v4l2_control ctrl;
    int ret;

    if (roi_weight == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_ROI;
    ctrl.value = (unsigned int)roi_weight;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_ae_roi(int fd, isp_weight *roi_weight)
{
    struct v4l2_control ctrl;
    int ret;

    if (roi_weight == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_AE_ROI;
    ctrl.value = (unsigned int)roi_weight;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
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

int isp_tuning_get_wb(int fd, isp_wb *wb)
{
    struct v4l2_control ctrl;
    int ret;

    if (wb == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_WB_ATTR;
    ctrl.value = (uint32_t)wb;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_wb(int fd, isp_wb *wb)
{
    struct v4l2_control ctrl;
    int ret;

    if (wb == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_WB_ATTR;
    ctrl.value = (uint32_t)wb;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_wb_statis(int fd, isp_wb *wb)
{
    struct v4l2_control ctrl;
    int ret;

    if (wb == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_WB_STATIS_ATTR;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (!ret) {
        wb->rgain = ctrl.value >> 16;
        wb->bgain = ctrl.value & 0xffff;
    } else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_get_wb_gol_statis(int fd, isp_wb *wb)
{
    struct v4l2_control ctrl;
    int ret;

    if (wb == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    ctrl.id = ISP_TUNING_CID_WB_STATIS_GOL_ATTR;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (!ret) {
        wb->rgain = ctrl.value >> 16;
        wb->bgain = ctrl.value & 0xffff;
    } else
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

int isp_tuning_get_adr_strength(int fd, uint32_t *strength)
{
    struct v4l2_control ctrl;
    int ret;

    if (strength == NULL) {
        fprintf(stderr, "%s, param is NULL\n", __func__);
        return -1;
    }

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = ISP_TUNING_CID_ADR_RATIO;
    ret = ioctl(fd, VIDIOC_PRIVATE_G_CTRL, &ctrl);
    if (!ret)
        *strength = ctrl.value;
    else
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}

int isp_tuning_set_adr_strength(int fd, uint32_t strength)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = ISP_TUNING_CID_ADR_RATIO;
    ctrl.value = strength;
    ret = ioctl(fd, VIDIOC_PRIVATE_S_CTRL, &ctrl);
    if (ret)
        fprintf(stderr, "%s fail, %s\n", __func__, strerror(errno));

    return ret;
}
