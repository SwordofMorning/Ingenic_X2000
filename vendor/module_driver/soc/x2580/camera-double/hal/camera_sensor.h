/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for Sensor
 *
 */

#ifndef __X2580_CAMERA_SENSOR_H__
#define __X2580_CAMERA_SENSOR_H__

#include <linux/list.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include "camera.h"

#define ISP_SUCCESS 0

#define DUAL_CAM
/* #define SINGLE_CAM */

#ifdef SINGLE_CAM
#define SENSORNUM 1
#define CSINUM 1
#define VINNUM 1
#define VICNUM 1
#define ISPCORENUM 1
#define TIZIANONUM 1
#endif

#ifdef DUAL_CAM
#define SENSORNUM 2
#define CSINUM 1
#define VINNUM 2
#define VICNUM 1
#define ISPCORENUM 2
#define TIZIANONUM 2
#endif

enum sensor_reg_ops {
    SENSOR_REG_OP_DATA = 1,
    SENSOR_REG_OP_DELAY,
    SENSOR_REG_OP_END,
};

struct sensor_reg_op {
    unsigned int flag;
    unsigned int reg;
    unsigned int val;
};

typedef enum {
    SENSOR_DATA_BUS_MIPI = 0,
    SENSOR_DATA_BUS_DVP,
    SENSOR_DATA_BUS_BT601,      /* 暂不支持 */
    SENSOR_DATA_BUS_BT656,      /* 暂不支持 */
    SENSOR_DATA_BUS_BT1120,     /* 暂不支持 */
    SENSOR_DATA_BUS_BUTT,
} sensor_data_bus_type;

typedef enum {
    SENSOR_DATA_DMA_MODE_RAW     = 0,    /* RAW8/10/12 */
    SENSOR_DATA_DMA_MODE_YUV422  = 3,
    SENSOR_DATA_DMA_MODE_NV12    = 6,
    SENSOR_DATA_DMA_MODE_NV21    = 7,
    SENSOR_DATA_DMA_MODE_GREY    = 100, /* 自定义 */
} sensor_data_dma_mode;

typedef enum {
    DVP_RAW8            = 0,    /* 该顺序不可调整 */
    DVP_RAW10,
    DVP_YUV422,
    DVP_RESERVED1,
    DVP_RESERVED2,
    DVP_YUV422_8BIT,
} dvp_data_fmt;

typedef enum {
    DVP_PA_10BIT,
    DVP_PA_8BIT,
} dvp_gpio_mode;

typedef enum {
    DVP_HREF_MODE,
    DVP_HSYNC_MODE,
    DVP_SONY_MODE,
} dvp_timing_mode;

/*                         clk1,clk2,clk3,clk4
 * 初始yuv 4字节顺序_1_2_3_4   1    2    3    4
 * 可转变成以下顺序
 */
typedef enum {
    order_2_1_4_3,
    order_2_3_4_1,
    order_1_2_3_4,
    order_1_4_3_2,
} yuv_data_order;

typedef enum {
    POLARITY_HIGH_ACTIVE,
    POLARITY_LOW_ACTIVE,
} dvp_sync_polarity;

typedef enum {
    POLARITY_SAMPLE_RISING,
    POLARITY_SAMPLE_FALLING,
} dvp_sample_polarity;

typedef enum {
    DVP_IMG_SCAN_PROGRESS,
    DVP_IMG_SCAN_INTERLACE,
} dvp_img_scan_mode;


struct dvp_bus {
    dvp_data_fmt data_fmt;
    dvp_gpio_mode gpio_mode;
    dvp_timing_mode timing_mode;
    yuv_data_order yuv_data_order;
    dvp_sample_polarity pclk_polarity;
    dvp_sync_polarity hsync_polarity;
    dvp_sync_polarity vsync_polarity;
    dvp_img_scan_mode img_scan_mode;
};

/*
 * MIPI information
 */
typedef enum {
    MIPI_RAW8           = 0,    /* 该顺序不可调整 */
    MIPI_RAW10,
    MIPI_RAW12,
    MIPI_RESERVED1,
    MIPI_RESERVED2,
    MIPI_RESERVED3,
    MIPI_RESERVED4,
    MIPI_YUV422         = 7,
    MIPI_RESERVED5,
} mipi_data_fmt;

typedef enum {
    MIPI_CTRL_YUV422    = 0x1e,
    MIPI_CTRL_RAW8      = 0x2a,
    MIPI_CTRL_RAW10     = 0x2b,
    MIPI_CTRL_RAW12     = 0x2c,
    MIPI_CTRL_RESERVED,
} mipi_ctrl_data_type;



struct mipi_sensor_ctrl {
    int hcrop_diff_en;
    int mipi_vcomp_en;
    int mipi_hcomp_en;
    int line_sync_mode;
    int work_start_flag;
    int data_type_en;
    mipi_ctrl_data_type data_type_value;
    int del_start;
    int sensor_fid_mode;
};
struct mipi_csi_crop {
    unsigned char enable;
    struct mipi_sensor_ctrl sensor_ctrl;
    unsigned short start0x;
    unsigned short start0y;
    unsigned short start1x;
    unsigned short start1y;
    unsigned short start2x;
    unsigned short start2y;
    unsigned short start3x;
    unsigned short start3y;
    int output_width;
    int output_height;
};

enum sensor_vc_mode {
    SENSOR_DEFAULT_MODE = 0,
    SENSOR_NOT_VC_MODE,
    SENSOR_VC_MODE,
};

enum sensor_frame_mode {
    SENSOR_DEFAULT_FRAME_MODE = 0,
    SENSOR_WDR_2_FRAME_MODE,
    SENSOR_WDR_3_FRAME_MODE,
    SENSOR_WDR_4_FRAME_MODE,
};

struct mipi_csi_bus {
    mipi_data_fmt data_fmt;
    int lanes;
    int clk;
    // enum sensor_frame_mode frame_mode;
    struct mipi_csi_crop mipi_crop;
    unsigned short clk_settle_time;  /* unit: ns, range: 95 ~ 300ns */
    unsigned short data_settle_time; /* unit: ns, range: 85 ~ 145ns + 10*UI */
};

struct mipi_switch_ctrl {
        uint8_t  gpio_state;
        int32_t  gpio;
};

enum sensor_data_type {
    SENSOR_DATA_TYPE_LINEAR = 0,
    SENSOR_DATA_TYPE_WDR_FS,
    SENSOR_DATA_TYPE_WDR_DOL,
    SENSOR_DATA_TYPE_WDR_NATIVE,
};

struct sensor_info {
    void *private_init_setting;

    /* The following attributes are determined by private_init_setting */
    int width;
    int height;
    enum sensor_frame_mode frame_mode;
    sensor_pixel_fmt fmt;
    enum sensor_data_type data_type;
    unsigned int fps;
    unsigned int min_fps;
    unsigned int max_fps;
    unsigned int max_again;    //the format is .16
    unsigned int max_dgain;    //the format is .16
    unsigned int again;
    unsigned int dgain;
    unsigned short min_integration_time;
    unsigned short min_integration_time_native;
    unsigned short max_integration_time_native;
    unsigned short integration_time_limit;
    unsigned int integration_time;
    unsigned short total_width;
    unsigned short total_height;
    unsigned short max_integration_time;
    unsigned short integration_time_apply_delay;
    unsigned short again_apply_delay;
    unsigned short dgain_apply_delay;
    unsigned short one_line_expr_in_us;
    unsigned short min_integration_time_short;
    unsigned short max_integration_time_short;
    unsigned int integration_time_short;
    unsigned int max_again_short;    //the format is .16
    unsigned int again_short;
    int wdr_en;
    unsigned int wdr_cache;    //the format is .16
    unsigned int expo;
    unsigned int expo_short;
    unsigned int expo_fs;
    unsigned int sensor_num;
    unsigned int max_dgain_short;    //the format is .16
    unsigned int dgain_short;
    unsigned short again_short_apply_delay;
    unsigned short dgain_short_apply_delay;
    unsigned short integration_time_short_apply_delay;
    unsigned int sfmt_change;
    unsigned int shvflip;
};

struct sensor_dbg_register {
    unsigned long long reg;
    unsigned long long val;
    unsigned int size;      /* val size, unit:byte */
};

typedef enum {
    IMPISP_FLIP_NORMAL_MODE = 0,
    IMPISP_FLIP_ISP_H_MODE,
    IMPISP_FLIP_ISP_V_MODE,
    IMPISP_FLIP_ISP_HV_MODE,
    IMPISP_FLIP_MODE_BUTT,
} ISP_CORE_HVFLIP;

typedef struct {
        ISP_CORE_HVFLIP sensor_mode;
        ISP_CORE_HVFLIP isp_mode[3];
} tisp_hv_flip_t;

/**
 * camera功能开关
 */
typedef enum {
    CAMERA_OPS_MODE_DISABLE = 0,    /* 不使能该模块功能 */
    CAMERA_OPS_MODE_ENABLE,         /* 使能该模块功能 */
} camera_ops_mode;

struct sensor_ctrl_ops {
    /* base */
    int (*power_on)(void);
    void (*power_off)(void);
    int (*stream_on)(void);
    void (*stream_off)(void);

    /* debug */
    int (*get_register)(struct sensor_dbg_register *reg);
    int (*set_register)(struct sensor_dbg_register *reg);

    /* isp tuning */
    unsigned int (*alloc_integration_time)(unsigned int it, unsigned char shift, unsigned int *sensor_it);
    int (*set_integration_time)(int value);
    unsigned int (*alloc_again)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again);
    int (*set_analog_gain)(int value);
    unsigned int (*alloc_dgain)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain);
    int (*set_digital_gain)(int value);
#if 1
    unsigned int (*alloc_integration_time_short)(unsigned int it, unsigned char shift, unsigned int *sensor_it);
    int (*set_integration_time_short)(int value);
    unsigned int (*alloc_again_short)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again);
    int (*set_analog_gain_short)(int value);
    unsigned int (*alloc_dgain_short)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain);
    int (*set_digital_gain_short)(int value);
    int (*set_expo)(int value);
    int (*set_expo_short)(int value);

    int (*get_black_pedestal)(int value);
    int (*set_wdr)(int wdr_en);
    int (*set_hvflip)(tisp_hv_flip_t *hvflip);

#endif

    int (*set_fps)(int fps);
    /* for vic and cim only */
    int (*get_hflip)(camera_ops_mode *mode);
    int (*set_hflip)(camera_ops_mode mode);
    int (*get_vflip)(camera_ops_mode *mode);
    int (*set_vflip)(camera_ops_mode mode);

    int (*reset_fmt)(struct camera_info *info);

    /* !!note: run in interrupt context */
    int (*frame_start_callback)(void);
    int (*frame_done_callback)(void);
};

struct sensor_attr {
    char *device_name;
    unsigned int cbus_addr;
    struct camera_info info;

    sensor_data_dma_mode dma_mode;  /* 控制器DMA输出格式选择 */
    sensor_data_bus_type dbus_type;
    union {
        struct dvp_bus dvp;
        struct mipi_csi_bus mipi;
    };

    long isp_clk_rate;

    enum sensor_vc_mode vc_mode;

    struct sensor_info sensor_info;
    struct sensor_ctrl_ops ops;

};

enum frame_data_status {
    frame_status_free,
    frame_status_trans,
    frame_status_usable,
    frame_status_user,
};

struct frame_data {
    struct list_head link;
    int status;
    volatile int cam_id; /* frame 来自哪个sensor */
    void *addr;
    struct frame_info info;
};

struct camera_device {
    struct sensor_attr *sensor[2];
    unsigned int is_power_on[2];
    unsigned int is_stream_on[2];
};


static inline void m_msleep(int ms)
{
    usleep_range(ms*1000, ms*1000);
}

static inline unsigned long long get_time_us(void)
{
    struct timespec ts;
    unsigned long long timestamp;

    ktime_get_ts(&ts);
    timestamp = (unsigned long long)ts.tv_sec * USEC_PER_SEC + ts.tv_nsec / NSEC_PER_USEC;

    return timestamp;
}


int camera_register_sensor(int index, struct sensor_attr *sensor);
void camera_unregister_sensor(int index, struct sensor_attr *sensor);

void camera_enable_sensor_mclk(int index, unsigned long clk_rate);
void camera_disable_sensor_mclk(int index);
void vic_set_switch_gpio(struct mipi_switch_ctrl *gpio);
void vic_get_switch_gpio(struct mipi_switch_ctrl *gpio);

int dvp_init_select_gpio(struct dvp_bus *dvp_bus,int dvp_gpio_func);
void dvp_deinit_gpio(void);

#endif /* __X2580_CAMERA_SENSOR_H__ */
