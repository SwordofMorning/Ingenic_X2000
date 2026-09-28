/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * ISP Driver
 */

#ifndef __X2580_ISP_H__
#define __X2580_ISP_H__

#include <linux/videodev2.h>
#include "camera_sensor.h"
#include "dsys.h"
#include "tiziano-core/include/tiziano-isp.h"

typedef enum {
    ISP_MODULE_UNDEFINE = 0,
    ISP_MODULE_SLAKE,
    ISP_MODULE_ACTIVATE,
    ISP_MODULE_DEINIT = ISP_MODULE_ACTIVATE,
    ISP_MODULE_INIT,
    ISP_MODULE_RUNNING,
} isp_state_t;


struct isp_version {
    char cpu[32];
    char version[64];
};

struct isp_buf_info {
    uint32_t vinum;
    uint32_t paddr;
    uint32_t size;
};

#define NOTIFICATION_TYPE_CORE_OPS      (0x1 << 24)
#define NOTIFICATION_TYPE_SENSOR_OPS    (0x2 << 24)
#define NOTIFICATION_TYPE_FS_OPS     (0x3 << 24)
#define NOTIFICATION_TYPE_TUN_OPS     (0x4 << 24)
#define NOTIFICATION_TYPE_LDC_OPS     (0x5 << 24)
#define NOTIFICATION_TYPE_OPS(n)    ((n) & (0xff << 24))

struct isp_event_initarg{
    int enable;
    int vinum;
};

struct tisp_control {
    uint32_t             id;
    __s32             value;
};

/* define common struct */
enum isp_priv_ioctl_direction {
    ISP_PRIVATE_IOCTL_SET,
    ISP_PRIVATE_IOCTL_GET,
};
struct isp_image_tuning_default_ctrl {
    int vinum;
    enum isp_priv_ioctl_direction dir;
    struct tisp_control control;
};

enum output_mbus_fmt {
    TISP_VO_FMT_YUV_SEMIPLANAR_420 = 0, //NV12
    TISP_VO_FMT_YVU_SEMIPLANAR_420,     //NV21
    TISP_VO_FMT_YUV_SEMIPLANAR_422,
    TISP_VO_FMT_YVU_SEMIPLANAR_422,
    TISP_VO_FMT_UVY_SEMIPLANAR_422,
    TISP_VO_FMT_VUY_SEMIPLANAR_422,

    TISP_VO_FMT_RAW8_1X8,
    TISP_VO_FMT_RAW16_1X16,
    TISP_VO_FMT_END,
};

enum tisp_notification {
    /* the events of subdev */
    TISP_EVENT_SUBDEV_INIT = NOTIFICATION_TYPE_CORE_OPS,
    TISP_EVENT_SYNC_SENSOR_ATTR,
    TISP_EVENT_CREATE_FRAME_CHAN_INODE,
    TISP_EVENT_SYNC_FRAME_FORMAT,
    TISP_EVENT_ADD_INTERNAL_INPUT,
    TISP_EVENT_SET_INTERNAL_INPUT,
    TISP_EVENT_GET_INTERNAL_INPUT,
    /* the events of sensor are defined as follows. */
    TISP_EVENT_SENSOR_REGISTER = NOTIFICATION_TYPE_SENSOR_OPS,
    TISP_EVENT_SENSOR_RELEASE,
    TISP_EVENT_SENSOR_ENUM_INPUT,
    TISP_EVENT_SENSOR_GET_INPUT,
    TISP_EVENT_SENSOR_SET_INPUT,
    TISP_EVENT_SENSOR_INT_TIME,
    TISP_EVENT_SENSOR_INT_TIME_SHORT,
    TISP_EVENT_SENSOR_AGAIN,
    TISP_EVENT_SENSOR_AGAIN_SHORT,
    TISP_EVENT_SENSOR_DGAIN,
    TISP_EVENT_SENSOR_FPS,
    TISP_EVENT_SENSOR_BLACK_LEVEL,
    TISP_EVENT_SENSOR_WDR,
    TISP_EVENT_SENSOR_RESIZE,
    TISP_EVENT_SENSOR_PREPARE_CHANGE,
    TISP_EVENT_SENSOR_FINISH_CHANGE,
    TISP_EVENT_SENSOR_VFLIP,
    TISP_EVENT_SENSOR_S_REGISTER,
    TISP_EVENT_SENSOR_G_REGISTER,
    TISP_EVENT_SENSOR_WDR_STOP,
    TISP_EVENT_SENSOR_WDR_OPEN,
    TISP_EVENT_SENSOR_LOGIC,
    TISP_EVENT_SENSOR_EXPO,
    TISP_EVENT_SENSOR_EXPO_SHORT,
    TISP_EVENT_DUAL_MODE,
    TISP_EVENT_BYPASS_MODE,
    TISP_EVENT_SENSOR_DGAIN_SHORT,
    TISP_EVENT_FRAME_SYNC_MODE,
    /* the events of frame-channel are defined as follows. */
    TISP_EVENT_FRAME_CHAN_BYPASS_ISP = NOTIFICATION_TYPE_FS_OPS,
    TISP_EVENT_FRAME_CHAN_GET_FMT,
    TISP_EVENT_FRAME_CHAN_SET_FMT,
    TISP_EVENT_FRAME_CHAN_STREAM_ON,
    TISP_EVENT_FRAME_CHAN_STREAM_OFF,
    TISP_EVENT_FRAME_CHAN_QUEUE_BUFFER,
    TISP_EVENT_FRAME_CHAN_DQUEUE_BUFFER,
    TISP_EVENT_FRAME_CHAN_REQUEST_BUFFER,
    TISP_EVENT_FRAME_CHAN_FREE_BUFFER,
    TISP_EVENT_FRAME_CHAN_SET_BANKS,
    TISP_EVENT_FRAME_CHAN_CREATE_PRIVATE_PIPO,
    /* the tuning node of isp's core */
    TISP_EVENT_ACTIVATE_MODULE = NOTIFICATION_TYPE_TUN_OPS,
    TISP_EVENT_SLAVE_MODULE,
    TISP_EVENT_CORE_FRAME_DONE,
    TISP_EVENT_CORE_DAY_NIGHT,
};

enum tisp_i2c_index {
    TISP_I2C_SET_AGAIN,
    TISP_I2C_SET_AGAIN_SHORT,
    TISP_I2C_SET_DGAIN,
    TISP_I2C_SET_INTEGRATION,
    TISP_I2C_SET_INTEGRATION_SHORT,
    TISP_I2C_SET_HVFLIP,
    TISP_I2C_SET_EXPO,
    TISP_I2C_SET_EXPO_SHORT,
    TISP_I2C_SET_DGAIN_SHORT,
    TISP_I2C_SET_FPS,
    TISP_I2C_SET_BUTTON,
};
struct tisp_i2c_msg {
    unsigned int flag;
    unsigned int value;
};

struct jz_isp_data {
    int index;
    int is_finish;

    int irq;
    const char *irq_name;

    struct mutex lock;
    spinlock_t slock;

    /* Camera Device */
    struct camera_device camera;

    /* the private parameters */
    struct task_struct *process_thread[TIZIANONUM];
    // tisp_core_t core;
    struct isp_core_tuning_driver *tuning;

    /* frame state */
    volatile unsigned int frame_state; // 0 : idle, 1 : processing
    unsigned int vflip_state; //0:disable, 1: enable
    unsigned int vflip_change; //0:disable, 1: enable
    unsigned int hflip_state; //0:disable, 1: enable
    unsigned int hflip_change; //0:disable, 1: enable
    unsigned int isp_daynight_switch[ISPCORENUM];

    int state[ISPCORENUM];

    /* IRQ callbacks */
    int (*irq_func_cb[96])(int vinum);
    void *irq_func_data[96];

    /* i2c sync messages */
    struct tisp_i2c_msg i2c_msgs[ISPCORENUM][TISP_I2C_SET_BUTTON];

    /* err cnt */
    int isp_err;
    int isp_err1;
    int isp_overflow;
    int isp_breakfrm;

    /* default bin file path */
    isp_bin_path bpath[ISPCORENUM];
    /* wdr state */
    unsigned int wdr_en[ISPCORENUM]; //0:disable, 1: enable

#ifdef SOC_CAMERA_DEBUG
    struct kobject *dsysfs_parent_kobj;
    struct kobject dsysfs_kobj;
#endif

};

int isp_stream_on(int index, struct sensor_attr *attr);
int isp_stream_off(int index, struct sensor_attr *attr);
int isp_power_on(int index);
void isp_power_off(int index);

int isp_component_bind_sensor(int index, struct sensor_attr *sensor);
void isp_component_unbind_sensor(int index, struct sensor_attr *sensor);

int jz_isp_drv_init(int index);
void jz_isp_drv_deinit(int index);

#endif /* __X2580_ISP_H__ */