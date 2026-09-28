#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>

#include "isp_tuning.h"
#include "tiziano-core-ctrl.h"


static int isp_tunning_sensor_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = 0;
    struct jz_isp_data *isp = tuning->parent;
    struct sensor_attr *attr = isp->camera.sensor[vinum];
    struct isp_core_sensor_attr sensor_attr;

    sensor_attr.hts    = attr->sensor_info.total_width;
    sensor_attr.vts    = attr->sensor_info.total_height;
    sensor_attr.fps    = attr->sensor_info.fps;
    sensor_attr.width  = attr->sensor_info.width;
    sensor_attr.height = attr->sensor_info.height;

    ret = copy_to_user((void __user*)control->value, &sensor_attr, sizeof(sensor_attr));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

    return ret;
}

typedef enum {
    TISP_IRQ_FD = 0,
    TISP_IRQ_FS = 1,
} irq_type_t;

struct isp_frame_done_info {
    unsigned int timeout;
    uint64_t cnt;
    irq_type_t irqtype;
};

uint64_t frame_done_cnt[2] = {0};
DECLARE_WAIT_QUEUE_HEAD(frame_done_wq);
atomic_t frame_done_cond = ATOMIC_INIT(0);
DECLARE_WAIT_QUEUE_HEAD(frame_done_wq_sec);
atomic_t frame_done_cond_sec = ATOMIC_INIT(0);

int isp_frame_done_wait(int vinum, int timeout, uint64_t *cnt)
{
    int ret = -1;

    if(vinum == 0){
        atomic_set(&frame_done_cond, 0);
        ret = wait_event_interruptible_timeout(frame_done_wq, (1 == atomic_read(&frame_done_cond)), timeout);
    } else {
        atomic_set(&frame_done_cond_sec, 0);
        ret = wait_event_interruptible_timeout(frame_done_wq_sec, (1 == atomic_read(&frame_done_cond_sec)), timeout);
    }
    *cnt = frame_done_cnt[vinum];
    if (-ERESTARTSYS == ret)
        return ret;

    if (!ret)
        return -ETIMEDOUT;

    return 0;
}

void isp_frame_done_wakeup(void *data)
{
    struct isp_event_initarg init;

    memcpy(&init, data, sizeof(init));
    frame_done_cnt[init.vinum]++;

    if(init.vinum == 0){
        atomic_set(&frame_done_cond, 1);
        wake_up(&frame_done_wq);
    } else {
        atomic_set(&frame_done_cond_sec, 1);
        wake_up(&frame_done_wq_sec);
    }
}

static int isp_tunning_wait_frame_done(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = ISP_SUCCESS;
    int timeout = 0;
    uint64_t cnt = 0;
    struct isp_frame_done_info info;

    ret = copy_from_user(&info, (const void __user*)control->value, sizeof(info));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!! %d\n", __func__, __LINE__, ret);
        return ret;
    }
    timeout = info.timeout;

    ret = isp_frame_done_wait(vinum, timeout, &cnt);
    info.cnt = cnt;
    info.irqtype = TISP_IRQ_FD;

    ret = copy_to_user((void __user*)control->value, &info, sizeof(info));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

    return ret;
}

static inline int isp_tunning_fps_g_control(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    struct jz_isp_data *isp = tuning->parent;
    struct sensor_attr *attr = isp->camera.sensor[vinum];

    control->value = attr->sensor_info.fps;

    return ISP_SUCCESS;
}

static inline int isp_tunning_sharp_s_control(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    unsigned char value = 0;
    int ret = ISP_SUCCESS;

    ret = copy_from_user(&value, (const void __user*)control->value, sizeof(value));
    if(ret != 0) {
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_set_sharpness(vinum, value);
        tuning->ctrls[vinum].sharpness = value;
    }

    return ret;
}

static inline int isp_tunning_sat_s_control(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    unsigned char value = 0;
    int ret = ISP_SUCCESS;

    ret = copy_from_user(&value, (const void __user*)control->value, sizeof(value));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_set_saturation(vinum, value);
        tuning->ctrls[vinum].saturation = value;
    }

    return ret;
}

static inline int isp_tunning_bright_s_control(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    unsigned char value = 0;
    int ret = ISP_SUCCESS;

    ret = copy_from_user(&value, (const void __user*)control->value, sizeof(value));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_set_brightness(vinum, value);
        tuning->ctrls[vinum].brightness = value;
    }

    return ret;
}

static inline int isp_tunning_contrast_s_control(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    unsigned char value = 0;
    int ret = ISP_SUCCESS;

    ret = copy_from_user(&value, (const void __user*)control->value, sizeof(value));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_set_contrast(vinum, value);
        tuning->ctrls[vinum].contrast = value;
    }

    return ret;
}

static inline int isp_tunning_fps_s_control(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    struct jz_isp_data *isp = tuning->parent;
    struct sensor_attr *attr = isp->camera.sensor[vinum];
    unsigned int fps = control->value;
    int ret = 0;

    if(attr->ops.set_fps) {
        ret = attr->ops.set_fps(fps);
        if (ret != 0) {
            printk(KERN_ERR "Failed to set sensor fps=0x%x, ret=%d\n", fps, ret);
            return ret;
        }

        tisp_set_fps(vinum, fps);
        tuning->ctrls[vinum].fps = fps;

    } else {
        printk(KERN_ERR "attr->ops.set_fps is NULL\n");
        return -EINVAL;
    }

    return 0;
}

static int isp_tunning_hvflip_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = ISP_SUCCESS;
    struct jz_isp_data *isp = tuning->parent;

    tisp_mirr_flip_t state;
    tisp_hv_flip_t flip;;

    uint32_t width, height;
    width = isp->camera.sensor[vinum]->sensor_info.width;
    height = isp->camera.sensor[vinum]->sensor_info.height;
    ret = copy_from_user(&flip, (const void __user*)control->value, sizeof(flip));
    if(ret != 0){
        printk(KERN_ERR "copy error!!!\n");
    } else {
        int i;

        if (flip.sensor_mode & (flip.isp_mode[0] | flip.isp_mode[1] | flip.isp_mode[2])) {
            printk(KERN_ERR "ISP and Sensor are prohibited from using mirr or flip together!!!\n");
            return -1;
        }

        for (i = 0; i < 3; i++) {
            state.chx = i;
            tisp_g_hv_flip(vinum, &state);
            state.y_mirr_en = flip.isp_mode[i] & 0x1;
            state.c_mirr_en = flip.isp_mode[i] & 0x1;
            state.y_flip_en = (flip.isp_mode[i] & 0x2) >> 1;
            state.c_flip_en = (flip.isp_mode[i] & 0x2) >> 1;
            tisp_s_hv_flip(vinum, &state);
        }

        if (tuning->ctrls[vinum].shvflip != flip.sensor_mode) {
            if (flip.sensor_mode == 1) {
                tisp_lsc_hvflip(width, height, 0, 1);
            } else if (flip.sensor_mode == 2) {
                tisp_lsc_hvflip(width, height, 1, 0);
            } else if (flip.sensor_mode == 3) {
                tisp_lsc_hvflip(width, height, 1, 1);
            } else {
                tisp_lsc_hvflip(width, height, 0, 0);
            }

            isp->i2c_msgs[vinum][TISP_I2C_SET_HVFLIP].flag = 1;
            isp->i2c_msgs[vinum][TISP_I2C_SET_HVFLIP].value = flip.sensor_mode;
            tuning->ctrls[vinum].shvflip = flip.sensor_mode;
        }

        if(ret != 0){
            printk(KERN_ERR "set control failed!!!\n");
            return -1;
        }
    }

    return ret;
}

static int isp_tunning_hvflip_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_hv_flip_t flip;
    tisp_mirr_flip_t state;
    int ret = ISP_SUCCESS;
    int i;

    memset(&flip, 0x0, sizeof(flip));
    for (i = 0; i < 3; i++) {
        state.chx = i;
        tisp_g_hv_flip(vinum, &state);
        if (state.y_mirr_en == state.c_mirr_en) {
            if (state.y_mirr_en) {
                flip.isp_mode[i] |= 0x1;
            }
        } else {
            printk(KERN_DEBUG "The mirr status of ch%d is abnormal.", i);
        }
        if (state.y_flip_en == state.c_flip_en) {
            if (state.y_flip_en) {
                flip.isp_mode[i] |= 0x2;
            }
        } else {
            printk(KERN_DEBUG "The flip status of ch%d is abnormal.", i);
        }
    }
    flip.sensor_mode = tuning->ctrls[vinum].shvflip;

    ret = copy_to_user((void __user*)control->value, &flip, sizeof(flip));
    if(ret != 0)
        printk(KERN_ERR "copy error!!!\n");

    return ISP_SUCCESS;
}

static int isp_tunning_bcsh_hue_s_ctrl(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    unsigned char value = 0;
    int ret = ISP_SUCCESS;

    ret = copy_from_user(&value, (const void __user*)control->value, sizeof(value));
    if(ret != 0) {
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_set_hue(vinum, value);
        tuning->ctrls[vinum].hue = value;
    }

        return ret;
}

static int isp_tunning_module_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_module_control_t module;
    int ret = 0;

    ret = copy_from_user(&module, (const void __user*)control->value, sizeof(tisp_module_control_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    else
        tisp_s_module_control(vinum, module);

    return ret;
}

static int isp_tunning_module_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_module_control_t module;
    int ret = 0;

    tisp_g_module_control(vinum, &module);
    ret = copy_to_user((void __user*)control->value, &module, sizeof(tisp_module_control_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!! --> ret %d\n", __func__, __LINE__, ret);

    return ret;
}

static inline int isp_tunning_day_or_night_s_ctrl(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = ISP_SUCCESS;
    struct jz_isp_data *isp = tuning->parent;
    struct image_tuning_ctrls *ctrls = &(tuning->ctrls[vinum]);
    TISP_MODE_DN_E dn;

    ret = copy_from_user(&dn, (const void __user*)control->value, sizeof(TISP_MODE_DN_E));
    if(ret != 0) {
        printk(KERN_ERR "[ %s:%d ] copy error!!! --> ret %d dn %d size %d %d\n", __func__, __LINE__, ret, dn,           \
                        sizeof(control->value), sizeof(TISP_MODE_DN_E));
    } else {
        if(dn != ctrls->daynight){
            ctrls->daynight = dn;
            isp->isp_daynight_switch[vinum] = 1;
            printk(KERN_ERR "00 ctrls->daynight %d!\n", ctrls->daynight);
        }
    }

    return ret;
}

static inline int isp_tunning_day_or_night_g_ctrl(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    struct image_tuning_ctrls *ctrls = &(tuning->ctrls[vinum]);
    /* unsigned int dn = tisp_day_or_night_g_ctrl(vinum); */

    /* if(dn != ctrls->daynight){ */
    /*      printk(KERN_ERR "%s:%d:@@@@ vinum is %d, dn is %d, ctrl is %d\n", __func__, __LINE__, vinum, dn, ctrls->daynight); */
    /*      printk(KERN_ERR "[ %s:%d ] day night mode is not sync!!!\n", __func__, __LINE__); */
    /* } */

    control->value = ctrls->daynight;

    return ISP_SUCCESS;
}

static inline int isp_tunning_csc_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_csc_attr_t attr;
    int ret = 0;

    ret = copy_from_user(&attr, (const void __user*)control->value, sizeof(attr));
    if(ret != 0) {
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        ret = tisp_set_csc_attr(vinum, &attr);
    }

    return ret;
}

static inline int isp_tunning_csc_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_csc_attr_t attr;
    int ret = 0;

    tisp_get_csc_attr(vinum, &attr);
    ret = copy_to_user((void __user*)control->value, &attr, sizeof(attr));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

    return ISP_SUCCESS;
}

static int tiziano_isp_ccm_attr_g_ctrl(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_ccm_attr_t ccm_attr;
    int ret = 0;

    tisp_g_ccm_attr(vinum, &ccm_attr);
    ret = copy_to_user((void __user*)control->value, &ccm_attr, sizeof(tisp_ccm_attr_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

    return 0;
}

static inline int tiziano_isp_ccm_attr_s_ctrl(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = 0;
    tisp_ccm_attr_t ccm_attr;

    ret = copy_from_user(&ccm_attr, (const void __user*)control->value, sizeof(tisp_ccm_attr_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        ret = tisp_s_ccm_attr(vinum, &ccm_attr);
        if(ret != 0)
            printk(KERN_DEBUG "[ %s:%d ] set ccm attr failed!!! ret == %d\n", __func__, __LINE__, ret);
    }

    return 0;
}

static int isp_tunning_module_ratio_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_module_ratio_t module_ratio;
    int ret = 0;

    ret = copy_from_user(&module_ratio, (const void __user*)control->value, sizeof(tisp_module_ratio_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        ret = tisp_s_module_ratio_attr(vinum, &module_ratio);
        if(ret != 0)
            printk(KERN_DEBUG "[ %s:%d ] set failed!!!\n", __func__, __LINE__);
    }

    return ret;
}

static int isp_tunning_module_ratio_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_module_ratio_t module_ratio;
    int ret = 0;

    ret = tisp_g_module_ratio_attr(vinum, &module_ratio);
    ret = copy_to_user((void __user*)control->value, (const void *)&module_ratio, sizeof(tisp_module_ratio_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] get failed!!!\n", __func__, __LINE__);

    return ret;
}

static int isp_tunning_gamma_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_gamma_attr_t tgamma;
    int ret = 0;

    ret = tisp_g_Gamma(vinum, &tgamma);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get gamma failed!!!\n", __func__, __LINE__);
        goto err_get_gamma;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&tgamma, sizeof(tgamma));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_gamma:
    return ret;
}

static int isp_tunning_gamma_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_gamma_attr_t tgamma;
    int ret = 0;

    ret = copy_from_user(&tgamma, (const void __user*)control->value, sizeof(tgamma));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!! --> ret %d dn %d size %d %d\n", __func__, __LINE__, ret, tgamma.gamma[0],           \
                    sizeof(control->value), sizeof(tgamma));
    } else {
        ret = tisp_s_Gamma(vinum, &tgamma);
        if(ret != 0)
            printk(KERN_DEBUG "[ %s:%d ] set gamma failed!!!\n", __func__, __LINE__);

    }
    return ret;
}

static inline int isp_tunning_flicker_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_anfiflicker_attr_t attr;
    int ret = 0;

    ret = copy_from_user(&attr, (const void __user*)control->value, sizeof(tisp_anfiflicker_attr_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        ret = tisp_s_antiflick(vinum, &attr);
        if(ret != 0)
            printk(KERN_DEBUG "[ %s:%d ] set control failed!!!\n", __func__, __LINE__);
    }

    return ret;
}

static int isp_tunning_flicker_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_anfiflicker_attr_t attr;
    int ret = 0;

    ret = tisp_g_antiflick(vinum, &attr);
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_flicker;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&attr, sizeof(tisp_anfiflicker_attr_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_flicker:
    return ret;
}

static int isp_tunning_autozoom_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_autozoom_attr_t autozoom_attr;
    int ret = 0;

    ret = copy_from_user(&autozoom_attr, (const void __user*)control->value, sizeof(tisp_autozoom_attr_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_autozoom_control(vinum, &autozoom_attr);
    }

    return ret;
}

static int isp_tunning_autozoom_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_autozoom_attr_t autozoom_attr;
    int ret = 0;

    ret = tisp_g_autozoom_control(vinum, &autozoom_attr);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_autozoom;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&autozoom_attr, sizeof(tisp_autozoom_attr_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_autozoom:
    return ret;
}

static int isp_tunning_mask_block_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_mask_block_attr_t mask_attr;
    int ret = 0;

    ret = copy_from_user(&mask_attr, (const void __user*)control->value, sizeof(tisp_mask_block_attr_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_mscaler_mask_block_attr(vinum, &mask_attr);
    }

    return 0;
}

static int isp_tunning_mask_block_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_mask_block_attr_t mask_attr;
    int ret = 0;

    tisp_g_mscaler_mask_block_attr(vinum, &mask_attr);
    ret = copy_to_user((void __user*)control->value, &mask_attr, sizeof(tisp_mask_block_attr_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

    return ret;
}

static int isp_tunning_osd_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = 0;
    tisp_osd_attr_t osd_attr;

    ret = copy_from_user(&osd_attr, (const void __user*)control->value, sizeof(tisp_osd_attr_t));

    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_osd_attr(vinum, &osd_attr);
    }

    return ret;
}

static int isp_tunning_osd_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = 0;
    tisp_osd_attr_t osd_attr;

    ret = tisp_g_osd_attr(vinum, &osd_attr);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_osd;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&osd_attr, sizeof(tisp_osd_attr_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_osd:
    return ret;
}

static int isp_tunning_osd_block_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = 0;
    tisp_osd_block_attr_t block_attr;

    ret = copy_from_user(&block_attr, (const void __user*)control->value, sizeof(tisp_osd_block_attr_t));

    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_osd_block_attr(vinum, &block_attr);
    }

    return ret;
}

static int isp_tunning_osd_block_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = 0;
    tisp_osd_block_attr_t block_attr;

    ret = tisp_g_osd_block_attr(vinum, &block_attr);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_osd;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&block_attr, sizeof(tisp_osd_block_attr_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_osd:
    return ret;
}

static int isp_tunning_draw_block_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = 0;
    tisp_draw_block_attr_t draw_attr;

    ret = copy_from_user(&draw_attr, (const void __user*)control->value, sizeof(tisp_draw_block_attr_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_draw_block_attr(vinum, &draw_attr);
    }

    return ret;
}

static int isp_tunning_draw_block_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    int ret = 0;
    tisp_draw_block_attr_t draw_attr;

    ret = tisp_g_draw_block_attr(vinum, &draw_attr);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_draw;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&draw_attr, sizeof(tisp_draw_block_attr_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_draw:
    return ret;
}

static int isp_tunning_af_weight_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_3a_weight_t af_weight;
    int ret = 0;

    ret = copy_from_user(&af_weight, (const void __user*)control->value, sizeof(tisp_3a_weight_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_af_weight_attr(vinum, &af_weight);
    }

    return ret;
}

static int isp_tunning_af_weight_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_3a_weight_t af_weight;
    int ret = 0;

    ret = tisp_g_af_weight_attr(vinum, &af_weight);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_af_weight;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&af_weight, sizeof(tisp_3a_weight_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_af_weight:
    return ret;
}

static int isp_tunning_awb_weight_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_3a_weight_t awb_weight;
    int ret = 0;

    ret = copy_from_user(&awb_weight, (const void __user*)control->value, sizeof(tisp_3a_weight_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_awb_weight_attr(vinum, &awb_weight);
    }

    return ret;
}

static int isp_tunning_awb_weight_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_3a_weight_t awb_weight;
    int ret = 0;

    ret = tisp_g_awb_weight_attr(vinum, &awb_weight);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_awb_weight;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&awb_weight, sizeof(tisp_3a_weight_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_awb_weight:
    return ret;
}

static int isp_tunning_awb_attr_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_awb_attr_t awb_attr;
    int ret = 0;

    ret = copy_from_user(&awb_attr, (const void __user*)control->value, sizeof(tisp_awb_attr_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_awb_attr(vinum, &awb_attr);
    }

    return ret;
}

static int isp_tunning_awb_global_statis_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_awb_global_statics_t awb_statis;
    int ret = 0;

    memset(&awb_statis, 0, sizeof(tisp_awb_global_statics_t));

    ret = tisp_g_awb_global_statis_attr(vinum, &awb_statis);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_awb_global;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)(&awb_statis), sizeof(tisp_awb_global_statics_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_awb_global:
    return ret;
}

static int isp_tunning_awb_attr_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_awb_attr_t awb_attr;
    int ret = 0;

    ret = tisp_g_awb_attr(vinum, &awb_attr);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_awb_attr;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&awb_attr, sizeof(tisp_awb_attr_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_awb_attr:
    return ret;
}

static int isp_tunning_awb_statis_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_wb_statis_info_t *awb_statis = NULL;
    int ret = 0;

    awb_statis = (tisp_wb_statis_info_t *)kmalloc(sizeof(tisp_wb_statis_info_t), GFP_KERNEL);
    memset(awb_statis, 0, sizeof(tisp_wb_statis_info_t));

    ret = tisp_g_awb_statis_attr(vinum, awb_statis);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_awb_attr;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)awb_statis, sizeof(tisp_wb_statis_info_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

    kfree((void*)awb_statis);

err_get_awb_attr:
    return ret;
}

static int isp_tunning_ae_scence_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_ae_scence_attr_t ae_scence;
    int ret = 0;

    ret = copy_from_user(&ae_scence, (const void __user*)control->value, sizeof(tisp_ae_scence_attr_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_ae_scence_attr(vinum, &ae_scence);
    }

    return ret;
}

static int isp_tunning_ae_scence_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_ae_scence_attr_t ae_scence;
    int ret = 0;

    ret = tisp_g_ae_scence_attr(vinum, &ae_scence);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_ae_scence;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&ae_scence, sizeof(tisp_ae_scence_attr_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_ae_scence:
    return ret;
}

static int isp_tunning_ae_expoinfo_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_ae_exprinfo_t ae_exprinfo;
    int ret = 0;

    ret = copy_from_user(&ae_exprinfo, (const void __user*)control->value, sizeof(tisp_ae_exprinfo_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_ae_exprinfo_attr(vinum, &ae_exprinfo);
    }

    return ret;
}

static int isp_tunning_ae_expoinfo_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_ae_exprinfo_t ae_exprinfo;
    int ret = 0;

    ret = tisp_g_ae_exprinfo_attr(vinum, &ae_exprinfo);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_ae_exprinfo;
    }
    ret = copy_to_user((void __user*)control->value, (const void *)&ae_exprinfo, sizeof(tisp_ae_exprinfo_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_ae_exprinfo:
    return ret;
}

static int isp_tunning_af_statis_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_af_statis_info_t *af_statis = (tisp_af_statis_info_t *)kmalloc(sizeof(tisp_af_statis_info_t), GFP_KERNEL);
    int ret = 0;

    ret = tisp_g_af_statis_attr(vinum, af_statis);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_af_statis_attr;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)af_statis, sizeof(tisp_af_statis_info_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

    kfree((void *)af_statis);

err_get_af_statis_attr:
        return ret;
}

static int isp_tunning_ae_statis_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_ae_statis_info_t *ae_statis = NULL;
    int ret = 0;

    ae_statis = (tisp_ae_statis_info_t *)kmalloc(sizeof(tisp_ae_statis_info_t), GFP_KERNEL);
    memset(ae_statis, 0, sizeof(tisp_ae_statis_info_t));

    ret = tisp_g_ae_statis_attr(vinum, ae_statis);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_ae_statis_attr;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)ae_statis, sizeof(tisp_ae_statis_info_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

    kfree((void*)ae_statis);
err_get_ae_statis_attr:
        return ret;
}

static int isp_tunning_ae_weight_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_ae_weight_t ae_weight;
    int ret = 0;

    ret = copy_from_user(&ae_weight, (const void __user*)control->value, sizeof(tisp_ae_weight_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_ae_weight_attr(vinum, &ae_weight);
    }

    return ret;
}

static int isp_tunning_ae_weight_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_ae_weight_t ae_weight;
    int ret = 0;

    ret = tisp_g_ae_weight_attr(vinum, &ae_weight);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_ae_weight;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&ae_weight, sizeof(tisp_ae_weight_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_ae_weight:
    return ret;
}

static int isp_tunning_statis_config_s_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_statis_config_t statis_config;
    int ret = 0;

    ret = copy_from_user(&statis_config, (const void __user*)control->value, sizeof(tisp_statis_config_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        tisp_s_statis_config_attr(vinum, &statis_config);
    }

    return ret;
}

static int isp_tunning_csccr_s_mode(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_csccr_mode_t attr;
    int ret = 0;

    ret = copy_from_user(&attr, (const void __user*)control->value, sizeof(tisp_csccr_mode_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    }else {
        tisp_s_csccr_mode(vinum, &attr);
    }

    return ret;
}

static int isp_tunning_csccr_g_mode(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_csccr_mode_t attr;
    int ret = 0;

    ret = tisp_g_csccr_mode(vinum, &attr);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_csccr;
    }

    ret = copy_to_user((void __user*)control->value, &attr, sizeof(tisp_csccr_mode_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    return ret;

err_csccr:
        return ret;
}

static int isp_tunning_statis_config_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_statis_config_t statis_config;
    int ret = 0;

    ret = tisp_g_statis_config_attr(vinum, &statis_config);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_statis_config;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&statis_config, sizeof(tisp_statis_config_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_statis_config:
    return ret;
}

static int isp_tunning_af_metrics_g_attr(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_af_metric_info_t metrics;
    int ret = 0;

    ret = tisp_g_af_metric_attr(vinum, &metrics);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
        goto err_get_metrics;
    }

    ret = copy_to_user((void __user*)control->value, (const void *)&metrics, sizeof(tisp_af_metric_info_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_metrics:
    return ret;
}

static int isp_tunning_switch_bin(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_bin_t attr;
    int ret = 0;
    struct jz_isp_data *isp = tuning->parent;

    ret = copy_from_user(&attr, (const void __user*)control->value, sizeof(attr));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    } else {
        /* printk(KERN_DEBUG "\n------------------>> Test:%d-%s <<------------------\n", attr.enable, attr.bname); */
        if(!attr.enable)
            sprintf(attr.bname, isp->bpath[vinum].path);

        if(isp->state[vinum] == ISP_MODULE_RUNNING) {
        ret = tisp_switch_bin(vinum, &attr);
        isp->isp_daynight_switch[vinum] = 1;
        } else
            printk(KERN_ERR"[ %s:%d ] The camera is not activated!!!\n", __func__, __LINE__);

        if(ret != 0)
            printk(KERN_ERR "[ %s:%d ] switch bin failed!!!\n", __func__, __LINE__);
        }

    return ret;
}

static int isp_tunning_ae_s_explist(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_ae_explist_t elist;
    int ret = 0;

    ret = copy_from_user(&elist, (const void __user*)control->value, sizeof(tisp_ae_explist_t));
    if(ret != 0){
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    }else {
        tisp_s_ae_explist(vinum, &elist);
    }

    return ret;
}

static int isp_tunning_ae_g_explist(int vinum, image_tuning_vdrv_t *tuning, struct v4l2_control *control)
{
    tisp_ae_explist_t elist;
    int ret = 0;

    ret = tisp_g_ae_explist(vinum, &elist);
    if(ret != 0){
        printk(KERN_DEBUG "[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
    }

    ret = copy_to_user((void __user*)control->value, &elist, sizeof(tisp_ae_explist_t));
    if(ret != 0)
        printk(KERN_ERR "[ %s:%d ] copy error!!!\n", __func__, __LINE__);
    return ret;
}

inline static int isp_core_ops_v4l2_g_ctrl(int vinum, struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    // image_tuning_ctrls *core_tuning = tuning->ctrls[vinum];
    int ret = 0;

    // switch(ctrl->id){
    //     case V4L2_CID_HFLIP:
    //         ctrl->value = core_tuning->hflip;
    //         break;
    //     case V4L2_CID_VFLIP:
    //         ctrl->value = core_tuning->vflip;
    //         break;
    //     case V4L2_CID_BRIGHTNESS:
    //         ret = isp_brightness_g_ctrl(tuning, ctrl);
    //         break;
    //     case V4L2_CID_CONTRAST:
    //         ret = isp_contrast_g_ctrl(tuning, ctrl);
    //         break;
    //     case V4L2_CID_SATURATION:
    //         ret = isp_saturation_g_ctrl(tuning, ctrl);
    //         break;
    //     case V4L2_CID_SHARPNESS:
    //         ret = isp_sharpness_g_ctrl(tuning, ctrl);
    //         break;
    //     case V4L2_CID_POWER_LINE_FREQUENCY:
    //         ret = isp_flicker_g_ctrl(tuning, ctrl);
    //         break;
    //     default:
    //         printk(KERN_ERR "%s, unknown ctrl cmd: %d !\n", __func__, ctrl->id);
    //         ret = -ENOIOCTLCMD;
    // }
    return ret;
}

inline static int isp_core_ops_v4l2_s_ctrl(int vinum, struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    int ret = 0;

    // switch (ctrl->id) {
    //     case V4L2_CID_HFLIP:
    //         ret = isp_hflip_s_control(tuning, ctrl);
    //         break;
    //     case V4L2_CID_VFLIP:
    //         ret = isp_vflip_s_control(tuning, ctrl);
    //         break;
    //     case V4L2_CID_BRIGHTNESS:
    //         ret = isp_brightness_s_ctrl(tuning, ctrl);
    //         break;
    //     case V4L2_CID_CONTRAST:
    //         ret = isp_contrast_s_ctrl(tuning, ctrl);
    //         break;
    //     case V4L2_CID_SATURATION:
    //         ret = isp_saturation_s_ctrl(tuning, ctrl);
    //         break;
    //     case V4L2_CID_SHARPNESS:
    //         ret = isp_sharpness_s_ctrl(tuning, ctrl);
    //         break;
    //     case V4L2_CID_POWER_LINE_FREQUENCY:
    //         ret = isp_flicker_s_ctrl(tuning, ctrl);
    //         break;
    //     default:
    //         printk(KERN_ERR "%s, unknown ctrl cmd: %d !\n", __func__, ctrl->id);
    //         ret = -ENOIOCTLCMD;
    // }
    return ret;
}

inline static int isp_core_ops_private_g_ctrl(int vinum, struct isp_core_tuning_driver *tuning, struct v4l2_control *control)
{
    int ret = 0;

    switch(control->id){
        case ISP_TUNING_CID_MODULE_CONTROL:
            ret = isp_tunning_module_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_HV_FLIP:
            ret = isp_tunning_hvflip_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_DAY_OR_NIGHT:
            ret = isp_tunning_day_or_night_g_ctrl(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_CSC_ATTR:
            ret = isp_tunning_csc_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_CCM_ATTR:
            ret = tiziano_isp_ccm_attr_g_ctrl(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_GAMMA_ATTR:
            ret = isp_tunning_gamma_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AE_ANTIFLICKER_ATTR:
            ret = isp_tunning_flicker_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AUTOZOOM_CONTROL:
            ret = isp_tunning_autozoom_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_CONTROL_FPS:
            ret = isp_tunning_fps_g_control(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_SENSOR_ATTR_CONTROL:
            ret = isp_tunning_sensor_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_ISP_WAIT_FRAME_ATTR:
            ret = isp_tunning_wait_frame_done(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_MASK_BLOCK_ATTR:
            ret = isp_tunning_mask_block_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_OSD_ATTR:
            ret = isp_tunning_osd_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_OSD_BLOCK_ATTR:
            ret = isp_tunning_osd_block_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_DRAW_BLOCK_ATTR:
            ret = isp_tunning_draw_block_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AF_WEIGHT:
            ret = isp_tunning_af_weight_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AWB_WEIGHT:
            ret = isp_tunning_awb_weight_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AWB_ATTR:
            ret = isp_tunning_awb_attr_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AWB_STATIS:
            ret = isp_tunning_awb_statis_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AWB_GLOBAL_STATIS:
            ret = isp_tunning_awb_global_statis_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AE_SCENCE_ATTR:
            ret = isp_tunning_ae_scence_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AE_EXPR_INFO:
            ret = isp_tunning_ae_expoinfo_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AE_STATIS:
            ret = isp_tunning_ae_statis_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AF_STATIS:
            ret = isp_tunning_af_statis_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AE_WEIGHT:
            ret = isp_tunning_ae_weight_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_STATIS_CONFIG:
            ret = isp_tunning_statis_config_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_MODULE_RATIO:
            ret = isp_tunning_module_ratio_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AF_METRIC_INFO:
            ret = isp_tunning_af_metrics_g_attr(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_CSCCR_ATTR:
            ret = isp_tunning_csccr_g_mode(vinum, tuning, control);
            break;
        case ISP_TUNING_CID_AE_EXP_LIST:
            ret = isp_tunning_ae_g_explist(vinum, tuning, control);
            break;
        default:
            printk(KERN_ERR "%s, unknown ctrl cmd: %d !\n", __func__, control->id);
            ret = -ENOIOCTLCMD;
    }

    return ret;
}

inline static int isp_core_ops_private_s_ctrl(int vinum, struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    int ret = 0;
//     printk("##### %s %d DEF_CTRL cmd=0x%08x #####\n", __func__,__LINE__, ctrl->id);

    switch (ctrl->id) {
        case ISP_TUNING_CID_BCSH_HUE:
            ret = isp_tunning_bcsh_hue_s_ctrl(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_HV_FLIP:
            ret = isp_tunning_hvflip_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_SATURATION:
            ret = isp_tunning_sat_s_control(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_BRIGHTNESS:
            ret = isp_tunning_bright_s_control(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_CONTRAST:
            ret = isp_tunning_contrast_s_control(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_SHARPNESS:
            ret = isp_tunning_sharp_s_control(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_CONTROL_FPS:
            ret = isp_tunning_fps_s_control(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_DAY_OR_NIGHT:
            ret = isp_tunning_day_or_night_s_ctrl(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_MODULE_CONTROL:
            ret = isp_tunning_module_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_CSC_ATTR:
            ret = isp_tunning_csc_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_CCM_ATTR:
            ret = tiziano_isp_ccm_attr_s_ctrl(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_GAMMA_ATTR:
            ret = isp_tunning_gamma_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_MODULE_RATIO:
            ret = isp_tunning_module_ratio_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_ANTIFLICKER_ATTR:
            ret = isp_tunning_flicker_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_AUTOZOOM_CONTROL:
            ret = isp_tunning_autozoom_s_attr(vinum, tuning, ctrl);
                break;
        case ISP_TUNING_CID_MASK_BLOCK_ATTR:
            ret = isp_tunning_mask_block_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_OSD_ATTR:
            ret = isp_tunning_osd_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_OSD_BLOCK_ATTR:
            ret = isp_tunning_osd_block_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_DRAW_BLOCK_ATTR:
            ret = isp_tunning_draw_block_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_AF_WEIGHT:
            ret = isp_tunning_af_weight_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_AWB_WEIGHT:
            ret = isp_tunning_awb_weight_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_AWB_ATTR:
            ret = isp_tunning_awb_attr_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_SCENCE_ATTR:
            ret = isp_tunning_ae_scence_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_EXPR_INFO:
            ret = isp_tunning_ae_expoinfo_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_WEIGHT:
            ret = isp_tunning_ae_weight_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_STATIS_CONFIG:
            ret = isp_tunning_statis_config_s_attr(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_SWITCH_BIN:
            ret = isp_tunning_switch_bin(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_CSCCR_ATTR:
            ret = isp_tunning_csccr_s_mode(vinum, tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_EXP_LIST:
            ret = isp_tunning_ae_s_explist(vinum, tuning, ctrl);
            break;
        default:
            printk(KERN_ERR "%s, unknown ctrl cmd: %d !\n", __func__, ctrl->id);
            ret = -ENOIOCTLCMD;
    }

    return ret;
}

inline static long isp_core_tunning_default_ioctl(image_tuning_vdrv_t *tuning, unsigned int cmd, unsigned long arg)
{
    int vinum =0;
    long ret = 0;
    struct v4l2_control control;

    switch(cmd) {
    case VIDIOC_PRIVATE_S_CTRL:
        ret = isp_core_ops_private_s_ctrl(vinum, tuning, (void __user *)arg);
    break;
    case VIDIOC_PRIVATE_G_CTRL:
        ret = isp_core_ops_private_g_ctrl(vinum, tuning, (void __user *)arg);
        if(ret && ret != -ENOIOCTLCMD)
            goto done;
        if (copy_to_user((void __user *)arg, &control, sizeof(control)))
            ret = -EFAULT;
    break;

    }

done:
    return ret;
}

static long isp_core_tunning_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct miscdevice *dev = file->private_data;
    struct isp_core_tuning_driver *tuning = mdev_to_tuningdriver(dev);
    struct v4l2_control control;
    int vinum = 0;
    long ret = 0;

    mutex_lock(&tuning->mlock);
    /* the file must be opened firstly. */
    if(tuning->state != ISP_MODULE_INIT){
        ret = -EPERM;
    }

    if(copy_from_user(&control, (void __user *)arg, sizeof(control))) {
        ret = -EFAULT;
        goto done;
    }

    switch(cmd){
        case VIDIOC_PRIVATE_S_CTRL:
            ret = isp_core_ops_private_s_ctrl(vinum, tuning, &control);
        break;
        case VIDIOC_PRIVATE_G_CTRL:
            ret = isp_core_ops_private_g_ctrl(vinum ,tuning, &control);
            if(ret && ret != -ENOIOCTLCMD)
                goto done;
            if (copy_to_user((void __user *)arg, &control, sizeof(control)))
                ret = -EFAULT;
        break;
        // default:
        // ret = isp_core_tunning_default_ioctl(tuning, cmd, arg);
        // break;

    }
    ret = (ret == -ENOIOCTLCMD) ? 0 : ret;

done:
    mutex_unlock(&tuning->mlock);

    return ret;
}

static int isp_core_tunning_open(struct inode *inode, struct file *file)
{
#if 0
    struct miscdevice *dev = file->private_data;
    struct isp_core_tuning_driver *tuning = mdev_to_tuningdriver(dev);

    /* the module must be actived firstly. */
    if(tuning->state <= STATE_CLOSE){
        printk(KERN_ERR "isp tuning open fail, please stream on first\n");
        return -EPERM;
    }
#endif

    struct miscdevice *dev = file->private_data;
    image_tuning_vdrv_t *tuning = mdev_to_tuningdriver(dev);
    int i = 0;

    /* the module must be actived firstly. */
    if(tuning->state != ISP_MODULE_ACTIVATE){
        return -EPERM;
    }

    for(i = 0; i < TIZIANONUM; i++){
        tuning->ctrls[i].contrast = 128;
        tuning->ctrls[i].saturation = 128;
        tuning->ctrls[i].brightness = 128;
        tuning->ctrls[i].sharpness = 128;
        tuning->ctrls[i].fps = 25 << 16 | 1;
        tuning->ctrls[i].shvflip = IMPISP_FLIP_NORMAL_MODE;
    }

    tisp_enable_tuning();
    tuning->state = ISP_MODULE_INIT;

    return 0;
}

static int isp_core_tunning_release(struct inode *inode, struct file *file)
{
    struct miscdevice *dev = file->private_data;
    image_tuning_vdrv_t *tuning = mdev_to_tuningdriver(dev);

    if(tuning->state == ISP_MODULE_DEINIT)
        return 0;

    tisp_disable_tuning();
    tuning->state = ISP_MODULE_DEINIT;

    return 0;
}

static struct file_operations isp_core_tunning_fops = {
    .open = isp_core_tunning_open,
    .release = isp_core_tunning_release,
    .unlocked_ioctl = isp_core_tunning_unlocked_ioctl,
};

static int isp_core_tuning_activate(struct isp_core_tuning_driver *tuning)
{
    mutex_lock(&tuning->mlock);
    tuning->state = ISP_MODULE_ACTIVATE;
    mutex_unlock(&tuning->mlock);
    return 0;
}

static int isp_core_tuning_slake(struct isp_core_tuning_driver *tuning)
{
    mutex_lock(&tuning->mlock);
    tuning->state = ISP_MODULE_SLAKE;
    mutex_unlock(&tuning->mlock);
    return 0;
}

static int isp_tunning_day_or_night_s_ctrl_internal(image_tuning_vdrv_t *tuning, void *data)
{
    struct isp_event_initarg *arg = (struct isp_event_initarg *)data;
    struct image_tuning_ctrls *ctrls = &(tuning->ctrls[arg->vinum]);
    int ret = ISP_SUCCESS;
    TISP_MODE_DN_E dn = ctrls->daynight;

    tisp_day_or_night_s_ctrl(arg->vinum, dn);
    ctrls->daynight = dn;

    return ret;
}

static int isp_core_tuning_event(struct isp_core_tuning_driver *tuning, unsigned int event, void *data)
{
    int ret = 0;
    switch(event){
        case TISP_EVENT_ACTIVATE_MODULE:
            ret = isp_core_tuning_activate(tuning);
            break;
        case TISP_EVENT_SLAVE_MODULE:
            ret = isp_core_tuning_slake(tuning);
            break;
        case TISP_EVENT_CORE_DAY_NIGHT:
            ret = isp_tunning_day_or_night_s_ctrl_internal(tuning, data);
        default:
            break;
    }
    return ret;
}


static struct isp_core_tuning_driver tuning_dev[] = {
    {
        .mdev = {
            .minor = MISC_DYNAMIC_MINOR,
            .name  = "isp-tuning",
            .fops  = &isp_core_tunning_fops,
        },
        .event      = isp_core_tuning_event,
        .state      = ISP_MODULE_SLAKE,
    },
};

void tuning_day_night_init(struct isp_core_tuning_driver *tuning)
{
    struct image_tuning_ctrls *ctrls = &(tuning->ctrls[0]);
    ctrls->daynight = TISP_RUNING_MODE_DAY_MODE;
}

int isp_core_tuning_init(struct jz_isp_data *parent)    // isp_core_tuning_driver
{
    int ret = misc_register(&tuning_dev[parent->index].mdev);
    if (ret != 0) {
        printk(KERN_ERR "Failed to register tuning dev!\n");
        return -1;
    }

    tuning_dev[parent->index].parent = parent;
//     tuning_dev[parent->index].parent->tuning = parent->tuning;
    tuning_dev[parent->index].state = ISP_MODULE_SLAKE;
    tuning_day_night_init(&tuning_dev[parent->index]);

    mutex_init(&tuning_dev[parent->index].mlock);

    parent->tuning = &tuning_dev[parent->index];

    return 0;
}

void isp_core_tuning_deinit(struct jz_isp_data *parent)
{
    misc_deregister(&tuning_dev[parent->index].mdev);
//     tuning_dev[parent->index].state = ISP_MODULE_SLAKE;
    parent->tuning = NULL;
}