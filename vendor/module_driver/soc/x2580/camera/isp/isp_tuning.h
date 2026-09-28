#ifndef __ISP_TUNING_H__
#define __ISP_TUNING_H__

#include <linux/sched.h>
#include <linux/cdev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include "isp.h"
#include "tiziano_core_tuning.h"


#define VIDIOC_PRIVATE_G_CTRL     _IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct v4l2_control)
#define VIDIOC_PRIVATE_S_CTRL     _IOWR('V', BASE_VIDIOC_PRIVATE + 2, struct v4l2_control)




typedef enum isp_core_module_ops_mode {
    ISPCORE_MODULE_DISABLE,
    ISPCORE_MODULE_ENABLE,
    ISPCORE_MODULE_BUTT,            /**< 用于判断参数的有效性，参数大小必须小于这个值 */
} ISPMODULE_OPS_MODE_E;

typedef enum isp_core_module_ops_type {
    ISPCORE_MODULE_AUTO,
    ISPCORE_MODULE_MANUAL,
} ISPMODULE_OPS_TYPE_E;

/* the defination of mode of isp during the day or night */
// typedef enum isp_core_mode_day_and_night {
//     ISP_CORE_RUNING_MODE_DAY_MODE,
//     ISP_CORE_RUNING_MODE_NIGHT_MODE,
//     ISP_CORE_RUNING_MODE_BUTT,
// } ISP_CORE_MODE_DN_E;

// typedef enum isp_core_mode_day_and_night {
// 	TISP_RUNING_MODE_DAY_MODE,
// 	TISP_RUNING_MODE_NIGHT_MODE,
// 	TISP_RUNING_MODE_CUSTOM_MODE,
// 	TISP_RUNING_MODE_BUTT,
// } TISP_MODE_DN_E;

// struct isp_core_sensor_attr{
// 	unsigned int hts;/* sensor hts */
// 	unsigned int vts;/* sensor vts */
// 	unsigned int fps;/* sensor fps: */
// 	unsigned int width;/* sensor width*/
// 	unsigned int height;/* sensor height*/
// };

// typedef enum {
// 	IMPISP_FLIP_NORMAL_MODE = 0,
// 	IMPISP_FLIP_ISP_H_MODE,
// 	IMPISP_FLIP_ISP_V_MODE,
// 	IMPISP_FLIP_ISP_HV_MODE,
// 	IMPISP_FLIP_MODE_BUTT,
// } ISP_CORE_HVFLIP;

// typedef struct {
//         ISP_CORE_HVFLIP sensor_mode;
//         ISP_CORE_HVFLIP isp_mode[3];
// } tisp_hv_flip_t;

struct isp_core_weight_attr{
    unsigned char weight[15][15];
};

struct isp_core_ae_sta_info{
    unsigned char ae_histhresh[4];
    unsigned short ae_hist[5];
    unsigned char ae_stat_nodeh;
    unsigned char ae_stat_nodev;
};

/* ev */
struct isp_core_ev_attr {
    unsigned int ae_manual;
    unsigned int ev;
    unsigned int integration_time;
    unsigned int min_integration_time;
    unsigned int max_integration_time;
    unsigned int integration_time_us;
    unsigned int sensor_again;
    unsigned int max_sensor_again;
    unsigned int sensor_dgain;
    unsigned int max_sensor_dgain;
    unsigned int isp_dgain;
    unsigned int max_isp_dgain;
    unsigned int total_gain;
};

/* expr */
enum isp_core_expr_mode {
    ISP_CORE_EXPR_MODE_AUTO = 0,
    ISP_CORE_EXPR_MODE_MANUAL,
};

enum isp_core_integration_time_unit {
    ISP_CORE_INTEGRATION_TIME_UNIT_LINE,
    ISP_CORE_INTEGRATION_TIME_UNIT_US,
};

struct isp_core_integration_time {
    enum isp_core_integration_time_unit unit;
    unsigned int time;
};

struct isp_core_expr_attr {
    enum isp_core_expr_mode mode;
    struct isp_core_integration_time integration_time;
    unsigned int again;
};

// /* awb */
// enum isp_core_wb_mode {
//     ISP_CORE_WB_MODE_AUTO = 0,
//     ISP_CORE_WB_MODE_MANUAL,
//     ISP_CORE_WB_MODE_DAY_LIGHT,
//     ISP_CORE_WB_MODE_CLOUDY,
//     ISP_CORE_WB_MODE_INCANDESCENT,
//     ISP_CORE_WB_MODE_FLOURESCENT,
//     ISP_CORE_WB_MODE_TWILIGHT,
//     ISP_CORE_WB_MODE_SHADE,
//     ISP_CORE_WB_MODE_WARM_FLOURESCENT,
//     ISP_CORE_WB_MODE_CUSTOM,
// };

// struct isp_core_wb_attr {
//     enum isp_core_wb_mode mode;
//     unsigned short rgain;
//     unsigned short bgain;
// };

struct isp_core_gamma_attr{
    unsigned short gamma[129];
};

/* isp core tuning */
#define TISP_TUNING_CID_PRIVATE_BASE  0x08000000
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

struct image_tuning_ctrls {

    /* Enable - vertically flip */
    /* unsigned int vflip; */
    /* Enable - horizontally flip */
    /* unsigned int hflip; */

    /* Enable Wide Dynamic Range module */
    unsigned int wdr;

    /* used and inited */
    unsigned char contrast;
    unsigned char saturation;
    unsigned char brightness;
    unsigned char sharpness;
    unsigned char hue;

    /* sensor output fps */
    unsigned int fps;

    /* sensor mirr or flip */
    ISP_CORE_HVFLIP shvflip;

    /* The mode of isp day and night */
    TISP_MODE_DN_E daynight;
};

typedef struct isp_core_tuning_driver {
    char device_name[16];
    struct miscdevice mdev;

    struct jz_isp_data *parent;        // which is that the driver belongs to.

    // tisp_core_tuning_t *core_tuning;
    struct image_tuning_ctrls       ctrls[TIZIANONUM];
    spinlock_t      slock;
    struct mutex    mlock;
    isp_state_t     state;
    struct file_operations *fops;
    int (*event)(struct isp_core_tuning_driver *tuning, unsigned int event, void *data);
}image_tuning_vdrv_t;

#define mdev_to_tuningdriver(dev) (container_of(dev, struct isp_core_tuning_driver, mdev))

int isp_core_tuning_init(struct jz_isp_data *parent);
void isp_core_tuning_deinit(struct jz_isp_data *parent);


#endif //__ISP_TUNING_H__