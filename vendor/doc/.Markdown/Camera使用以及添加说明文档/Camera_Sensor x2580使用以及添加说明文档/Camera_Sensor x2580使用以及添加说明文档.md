# x2580 Camera 使用及添加 Sensor 说明文档



## 1 使用君正添加的 Camera Sensor

### 1.1 配置Camera Sensor

1. 选择x2580的config文件，进行配置

<img src="img/1.png" style="zoom:100%;" />

#### 首先配置camera驱动

2. 进入配置模块化驱动

<img src="img/2.png" style="zoom:100%;" />

3. 进入x2580 soc 相关驱动配置

<img src="img/3.png" style="zoom:100%;" />

4. 勾选 camera 驱动，并进入配置

<img src="img/4.png" style="zoom:100%;" />

5. 配置camera的相关参数

<img src="img/5.png" style="zoom:100%;" />

#### 然后配置 camera sensor

6. 回到模块化驱动界面，然后进入配置外设

<img src="img/6.png" style="zoom:100%;" />

7. camera sensor分为带isp和不带isp的，下面我们进入不带isp的camera设备列表。

<img src="img/7.png" style="zoom:100%;" />

8. 进入到camera设备列表后，就可以看到下图的界面

> 这些是目前君正x2580提供的sensor(后续会根据实际需求再添加)

<img src="img/8.png" style="zoom:100%;" />


**配置 sensor 外设**

9. 进入到sensor配置后，就可以看到sensor ov9282的配置界面

> 每种sensor需要我们配置的内容不同,下面我们看看 sensor ov9282的配置选项。

<img src="img/9.png" style="zoom:100%;" />

#### 配置I2C控制器

10. ov9282_mipi使用的是CSI接口，由下核心版资料可知，sensor的使用需要配置相关的i2c控制器

<img src="img/13.png" style="zoom:50%;" />

返回到x2580驱动列表，进行i2c的相关配置

<img src="img/10.png" style="zoom:100%;" />

11. 配置 i2c 控制器驱动

<img src="img/11.png" style="zoom:100%;" />

<img src="img/12.png" style="zoom:100%;" />


### 1.2 Camera 操作流程及接口说明

##### 1. open 函数
打开sensor设备 /dev/vic

##### 2. mmap 函数
内存映射，将应用层的地址与内核层的地址映射，主要目的是为了应用层能够正常使用从内核层得到的帧buf

##### 3. ioctl 函数
下面介绍ioctl操作的相关指令

```c
#define CMD_get_info          _IOWR('C', 120, struct camera_info) // 获得摄像头的信息
#define CMD_power_on          _IO('C', 121)   // 开启摄像头电源
#define CMD_power_off         _IO('C', 122)   // 关闭摄像头电源
#define CMD_stream_on         _IO('C', 123)   // 开启摄像头视频数据流
#define CMD_stream_off        _IO('C', 124)   // 关闭摄像头视频数据流
#define CMD_wait_frame        _IO('C', 125)   // 获取一帧录制的图像地址(未获取有效数据继续等待3s)
#define CMD_put_frame         _IO('C', 126)   // 将使用完的一帧buf还给camera驱动
#define CMD_get_frame_count   _IO('C', 127)   // 获取可用帧的数量
#define CMD_skip_frames       _IO('C', 128)   // 将几帧可用的帧主动放弃，用于存放新生成的帧图像
#define CMD_get_sensor_reg    _IO('C', 129)   // 获取sensor寄存器值
#define CMD_set_sensor_reg    _IO('C', 130)   // 设置sensor寄存器值
#define CMD_get_frame         _IO('C', 131)   // 获取一帧录制的图像地址(不等待)
#define CMD_dqbuf             _IO('C', 132)   // 获取一帧录制的图像信息(不等待)
#define CMD_dqbuf_wait        _IO('C', 133)   // 获取一帧录制的图像信息(未获取有效数据继续等待3s)
#define CMD_qbuf              _IO('C', 134)   // 释放一帧图像缓冲区
#define CMD_get_fps           _IO('C', 135)   // 获取sensor帧率
#define CMD_set_fps           _IO('C', 136)   // 设置sensor帧率
#define CMD_get_hflip         _IO('C', 137)   // 获取sensor镜像状态
#define CMD_set_hflip         _IO('C', 138)   // 设置sensor镜像状态
#define CMD_get_vflip         _IO('C', 139)   // 获取sensor翻转状态
#define CMD_set_vflip         _IO('C', 140)   // 设置sensor翻转状态
#define CMD_reset_fmt         _IO('C', 141)   // 重设camera格式
```

##### 4. close 函数
关闭已打开的sensor设备。
实际操作可参考 /libhardware2/src/cmds/camera_main.c 和 /libhardware2/src/lib/camera/camera.c 测试代码的编写。




## 2 添加Camera Sensor

### 2.1 添加流程

如果现有支持的camera sensor不适用，可按照下面流程添加自己的camera sensor驱动。

##### 1. 添加新的sensor模块
在/devices/camera/x2580/下添加新的sensor文件夹，其中包含.c文件、Makefile。(可参考同级目录下君正已经添加的sensor代码)。

##### 2. 实现接口
```c
sensor_ctrl_ops.power_on()     // 主要实现sensor的上电功能，包括时钟的使能。
sensor_ctrl_ops.power_off()    // 实现sensor关电功能，关时钟。
sensor_ctrl_ops.stream_on()    // 实现sensor开流
sensor_ctrl_ops.stream_off()   // 实现sensor关流

sensor_ctrl_ops.get_register() // debug, get sensor register
sensor_ctrl_ops.set_register() // debug, set sensor register

sensor_ctrl_ops.set_fps() 	   // 设置 sensor fps

i2c_driver.probe()  // i2c设备驱动的probe函数，主要实现申请相关gpio，以及调用camera_register_sensor注册sensor
i2c_driver.remove() // i2c设备驱动的remove函数，主要实现释放相关gpio，以及调用camera_unregister_sensor注销sensor

module_init()     //驱动的入口函数，主要实现i2c设备驱动的添加以及i2c设备的注册
module_exit()     //驱动的出口函数，主要实现i2c设备驱动的删除以及i2c设备的注销
```

##### 3. 配置Sensor结构体
在新添加的sensor.c代码中定义一个struct sensor_attr 类型的变量，并进行配置。该结构体类型的相关说明在第2.2章。

##### 4. 注册Sensor
调用 camera_register_sensor()函数对新添加的Sensor进行注册，并将上一步配置的结构体变量作为实参传入。注册注销函数的相关说明在2.3章。

##### 5. 将sensor.c文件加入IConfig配置界面并且加入编译
使用IConfigTool对x2580的Camera驱动进行配置，配置方法在第2.4章。



### 2.2 配置Sensor结构体

#### 2.2.1 包含头文件

```c
#include "soc/x2580/camera/camera_sensor.h"
```

#### 2.2.2 Sensor 结构体说明

##### 摄像头配置结构体
```c
struct sensor_attr {
    char *device_name;                      //摄像头设备名称
    unsigned int cbus_addr;                 //控制总线设备地址(SPI or IIC)
    struct camera_info info;                //camera 信息       不用配置

    sensor_data_dma_mode dma_mode;          //控制器DMA输出格式选择
    sensor_data_bus_tye dbus_type;          //数据总线类型
    union {
        struct dvp_bus dvp;                 //配置dvp接口信息（dbus_type选DVP时配置）
        struct mipi_csi_bus mipi;           //配置mipi接口信息（dbus_type选MIPI时配置）
    };

    long isp_clk_rate;                      //isp时钟频率

    enum sensor_vc_mode vc_mode;            //sensor工作模式：正常 or WDR

    struct sensor_info sensor_info;         //sensor基本属性信息
    struct sensor_ctrl_ops ops;             //sensor控制配置结构体
};
```


#### 2.2.3 结构体成员详解

##### 1. camera 信息结构体

>    camera_info 结构体主要用于获取摄像头信息时使用，在添加sensor驱动时**不用配置**该结构体。

```c
struct camera_info {
    const char *name;                     // 摄像头名称
    unsigned int width;                   // 图像宽度(一行多少像素点)
    unsigned int height;                  // 图像高度(一帧多少行)
    unsigned int fps;                     // 帧率(暂时未用)
    camera_pixel_fmt data_fmt;            // 设置camera_wait_frame得到的数据格式

    unsigned int line_length;             // 一行的长度, 单位字节       不用配置
                                          // 对于 nv12,nv21, 表示y数据一行的长度
                                          // 另外由此可以算出uv数据偏移 line_length*height

    unsigned int frame_size;              // 一帧数据经过对齐之后的大小      不用配置
    unsigned int frame_nums;              // 帧缓冲总数                   不用配置
    unsigned long phys_mem;               // 用于保存帧缓冲的物理基地址      不用配置
    void *mapped_mem;                     // 用于保存 mmap 后的帧缓冲基地址  不用配置
    unsigned int frame_align_size;        // 帧对齐大小
};
```

##### 2. 选择摄像头输出数据格式

列举部分常用格式 re-define from media-bus-format.h

```c
typedef enum {
    /* RGB - next is    0x1018 */
    SENSOR_PIXEL_FMT_RGB565_1X16            = 0x1017,
    SENSOR_PIXEL_FMT_BGR565_2X8_LE          = 0x1006,
    SENSOR_PIXEL_FMT_RGB565_2X8_LE          = 0x1008,
    SENSOR_PIXEL_FMT_RBG888_1X24            = 0x100e,
    SENSOR_PIXEL_FMT_BGR888_1X24            = 0x1013,
    SENSOR_PIXEL_FMT_GBR888_1X24            = 0x1014,
    SENSOR_PIXEL_FMT_RGB888_1X24            = 0x100a,
    SENSOR_PIXEL_FMT_ARGB8888_1X32          = 0x100d,

    /* YUV (including grey) - next is   0x2026 */
    SENSOR_PIXEL_FMT_Y8_1X8                 = 0x2001,
    SENSOR_PIXEL_FMT_UYVY8_2X8              = 0x2006,
    SENSOR_PIXEL_FMT_VYUY8_2X8              = 0x2007,
    SENSOR_PIXEL_FMT_YUYV8_2X8              = 0x2008,
    SENSOR_PIXEL_FMT_YVYU8_2X8              = 0x2009,
    SENSOR_PIXEL_FMT_Y10_1X10               = 0x200a,
    SENSOR_PIXEL_FMT_Y12_1X12               = 0x2013,
    SENSOR_PIXEL_FMT_UYVY8_1X16             = 0x200f,
    SENSOR_PIXEL_FMT_VYUY8_1X16             = 0x2010,
    SENSOR_PIXEL_FMT_YUYV8_1X16             = 0x2011,
    SENSOR_PIXEL_FMT_YVYU8_1X16             = 0x2012,
    SENSOR_PIXEL_FMT_VUY8_1X24              = 0x2024,
    SENSOR_PIXEL_FMT_YUV8_1X24              = 0x2025,
    SENSOR_PIXEL_FMT_AYUV8_1X32             = 0x2017,

    /* Bayer - next is  0x3019 */
    SENSOR_PIXEL_FMT_SBGGR8_1X8             = 0x3001,
    SENSOR_PIXEL_FMT_SGBRG8_1X8             = 0x3013,
    SENSOR_PIXEL_FMT_SGRBG8_1X8             = 0x3002,
    SENSOR_PIXEL_FMT_SRGGB8_1X8             = 0x3014,
    SENSOR_PIXEL_FMT_SBGGR10_1X10           = 0x3007,
    SENSOR_PIXEL_FMT_SGBRG10_1X10           = 0x300e,
    SENSOR_PIXEL_FMT_SGRBG10_1X10           = 0x300a,
    SENSOR_PIXEL_FMT_SRGGB10_1X10           = 0x300f,
    SENSOR_PIXEL_FMT_SBGGR12_1X12           = 0x3008,
    SENSOR_PIXEL_FMT_SGBRG12_1X12           = 0x3010,
    SENSOR_PIXEL_FMT_SGRBG12_1X12           = 0x3011,
    SENSOR_PIXEL_FMT_SRGGB12_1X12           = 0x3012,
} sensor_pixel_fmt;
```

##### 3. VIC DMA控制器输出重新排序格式
```c
/*
 * 只有当数据为YUV,即VIC接口的数据类型为DVP_YUV422/MIPI_YUV422时以下宏才生效
 *
 * SENSOR_DATA_DMA_MODE_NV12格式             SENSOR_DATA_DMA_MODE_NV21格式
 * VIC输入格式                               VIC输入格式
 * Y U Y V Y U Y V                           Y U Y V Y U Y V
 * Y U Y V Y U Y V                           Y U Y V Y U Y V
 * VIC DMA输出格式                           VIC DMA输出格式
 * Y Y Y Y Y Y Y Y                           Y Y Y Y Y Y Y Y
 * U V U V                                   V U V U
 */
typedef enum {
    SENSOR_DATA_DMA_MODE_RAW     = 0,    /* RAW8/10/12 数据顺序不做调整 */
    SENSOR_DATA_DMA_MODE_YUV422  = 3,    /* YUV422 数据顺序不做调整 */
    SENSOR_DATA_DMA_MODE_NV12    = 6,    /* YUV422输入格式 NV12输出 */
    SENSOR_DATA_DMA_MODE_NV21    = 7,    /* YUV422输入格式 NV21输出 */
    SENSOR_DATA_DMA_MODE_GREY    = 100,  /* 自定义 YUV422输入格式 GREY输出 */
} sensor_data_dma_mode;
```

##### 4. DVP接口信息配置
```c
struct dvp_bus {
    dvp_data_fmt           data_fmt;            // DVP接口数据格式
    dvp_gpio_mode          gpio_mode;           // 选择DVP数据线引脚
    dvp_timing_mode        timing_mode;         // DVP 接口时序模式
    yuv_data_order         yuv_data_order;      // DVP YUV数据的转换顺序
    dvp_sample_polarity    pclk_polarity;       // DVP pclk 边沿采样电平
    dvp_sync_polarity      hsync_polarity;      // DVP行同步信号的有效电平
    dvp_sync_polarity      vsync_polarity;      // DVP帧同步信号的有效电平
    dvp_img_scan_mode      img_scan_mode;       // DVP图像扫描模式
};
```

DVP接口数据格式
```c
typedef enum {
    DVP_RAW8            = 0,    // 该顺序不可调整
    DVP_RAW10,
    DVP_RAW12,
    DVP_YUV422,
    DVP_RESERVED1,
    DVP_RESERVED2,
    DVP_YUV422_8BIT,
} dvp_data_fmt;
```

选择DVP数据线引脚
```c
typedef enum {
    DVP_PA_10BIT,               // 10位 DVP 数据引脚
    DVP_PA_8BIT,                // 8位 DVP 数据引脚
} dvp_gpio_mode;
```

DVP 接口时序模式
```c
typedef enum {
    DVP_HREF_MODE,
    DVP_HSYNC_MODE,             // 目前不支持
    DVP_SONY_MODE,              // 目前不支持
} dvp_timing_mode;
```

选择图像yuv格式转换顺序
```c
/*                          clk1,clk2,clk3,clk4
 * 初始 yuv 4字节顺序_1_2_3_4: 1    2    3    4
 * 可根据自己的需求，转变成以下顺序
 */
typedef enum {
    order_2_1_4_3,
    order_2_3_4_1,
    order_1_2_3_4,
    order_1_4_3_2,
} yuv_data_order;
```

DVP 同步时序的有效电平
```c
typedef enum {
    POLARITY_HIGH_ACTIVE,   // 高电平有效
    POLARITY_LOW_ACTIVE,    // 低电平有效
} dvp_sync_polarity;
```

DVP pclk 边沿采样有效电平
```c
typedef enum {
    POLARITY_SAMPLE_RISING, //上升沿采样
    POLARITY_SAMPLE_FALLING //下降沿采样
} dvp_sample_polarity;
```

DVP图像扫描模式
```c
typedef enum {
    DVP_IMG_SCAN_PROGRESS,  // 逐行扫描
    DVP_IMG_SCAN_INTERLACE, // 交错扫描(奇数行与偶数行分开扫描)
} dvp_img_scan_mode;
```

##### 5. MIPI接口信息配置
```c
struct mipi_csi_bus {
    mipi_data_fmt data_fmt; // mipi 数据格式
    int lanes;              // mipi 接收通道个数
    int clk;                // mipi 时钟频率，单位：Mbps/lane
    struct mipi_csi_crop mipi_crop;  // mipi 裁剪设置
};
```

mipi 数据格式
```c
typedef enum {
    MIPI_RAW8       = 0,    /* 该顺序不可调整 */
    MIPI_RAW10,
    MIPI_RAW12,
    MIPI_RESERVED1,
    MIPI_RESERVED2,
    MIPI_RESERVED3,
    MIPI_RESERVED4,
    MIPI_YUV422     = 7,
    MIPI_RESERVED5,
} mipi_data_fmt;
```

 mipi 裁剪设置
```c
struct mipi_csi_crop {
    unsigned char enable;   //mipi 裁剪使能
    struct mipi_sensor_ctrl sensor_ctrl;    // mipi 摄像头控制参数
    int hcrop_start;        //mipi 裁剪水平起始点
    int vcrop_start;        //mipi 裁剪垂直起始点
    int output_width;       //mipi 输出宽
    int output_height;      //mipi 输出高
};
```

mipi 摄像头控制设置
```c
struct mipi_sensor_ctrl {
    int hcrop_diff_en;
    int mipi_vcomp_en;
    int mipi_hcomp_en;
    int line_sync_mode;
    int work_start_flag;
    int data_type_en;       // mipi 数据类型过滤使能，0 使能（默认），1 关闭
    mipi_ctrl_data_type data_type_value;    // mipi 支持数据类型
    int del_start;          //mipi 每个通道start后延迟的行数（0～15）
    int sensor_fid_mode;
};
```

mipi 数据类型

```c
typedef enum {
    MIPI_CTRL_YUV422 = 0x1e,
    MIPI_CTRL_RAW8   = 0x2a,
    MIPI_CTRL_RAW10  = 0x2b,
    MIPI_CTRL_RAW12  = 0x2c,
    MIPI_CTRL_RESERVED,
} mipi_ctrl_data_type;
```

##### 6. 摄像头工作模式
```c
enum sensor_vc_mode {
    SENSOR_DEFAULT_MODE = 0,    // 默认模式
    SENSOR_NOT_VC_MODE,         // DOL WDR no VC mode
    SENSOR_VC_MODE,             // DOL WDR VC mode
};
```

##### 7. sensor基本属性信息
```c
struct sensor_info {
    void *private_init_setting;                         // sensor 寄存器初始化配置列表

    /* The following attributes are determined by private_init_setting */
    int width;                                          // sensor输出图像宽
    int height;                                         // sensor输出图像高
    enum sensor_frame_mode frame_mode;                  // sensor输出模式：默认 or WDR
    sensor_pixel_fmt fmt;                               // sensor输出格式
    enum sensor_data_type data_type;                    // sensor数据类型：线性 or WDR
    unsigned int fps;                                   // 帧率 fps = Numerator / denominator
    unsigned int min_fps;                               // 最大帧率 max_fps = Numerator / denominator
    unsigned int max_fps;                               // 最小帧率 min_fps = Numerator / denominator
    unsigned int max_again;                             // 最大模拟增益, the format is .16
    unsigned int max_dgain;                             // 最大数字增益（保留）, the format is .16
    unsigned int again;                                 /* 模拟增益 （无需填写, 查sensor模拟增益表.
                                                         * 表中为sensor 1x～16x模拟增益对应的寄存器值;
                                                         * 使用君正提供工具calgain生成模拟增益表）
                                                         */
    unsigned int dgain;                                 // 数字增益（保留）
    unsigned short min_integration_time;                // 最小曝光时间（单位：行 大于等于2行）
    unsigned short min_integration_time_native;         // 最小曝光时间（单位：行 大于等于2行）
    unsigned short max_integration_time_native;         // 最大曝光时间（单位：行, 小于帧长vts）
    unsigned short integration_time_limit;              // 最大曝光时间（单位：行, 小于帧长vts）
    unsigned int integration_time;                      // 曝光时间（无需填写）

    unsigned short total_width;                         // 行长 hts
    unsigned short total_height;                        // 帧长 vts
    unsigned short max_integration_time;                // 最大曝光时间（单位：行, 小于帧长vts）
    unsigned short integration_time_apply_delay;        // 曝光生效delay帧（保留）
    unsigned short again_apply_delay;                   // 增益生效delay帧（保留）
    unsigned short dgain_apply_delay;                   // 数字增益delay帧（保留）
    unsigned short one_line_expr_in_us;                 /* 一行的曝光时间(单位：us， 无需填写）
                                                         *（one_line_expr_in_us = 1s / fps / vts）
                                                         */

    /* 以下带有_short的表示WDR模式下短帧的参数, wdr_en 开启时配置 *_short* 的参数 */
    unsigned short min_integration_time_short;
    unsigned short max_integration_time_short;
    unsigned int integration_time_short;
    unsigned int max_again_short;                       //the format is .16
    unsigned int again_short;
    int wdr_en;                                         // WDR 使能
    unsigned int wdr_cache;                             //the format is .16

    unsigned int expo;                                  // sensor 曝光参数，高16位是积分时间
                                                        // 低16位是模拟增益（无需填写）
    unsigned int expo_short;
    unsigned int expo_fs;
    unsigned int sensor_num;                            // 多摄情况下sensor个数 （保留，但无需填写）
    unsigned int max_dgain_short;                       //the format is .16
    unsigned int dgain_short;
    unsigned short again_short_apply_delay;
    unsigned short dgain_short_apply_delay;
    unsigned short integration_time_short_apply_delay;
    unsigned int shvflip;                               //sensor horizontal & vertical flip 使能
};
以上参数值均来源于sensor寄存器配置表
该结构体中成员值的获取/计算公式等可参考驱动模板:add_sensor_driver_example.c
```

摄像头帧输出模式：默认 or WDR
```c
enum sensor_frame_mode {
    SENSOR_DEFAULT_FRAME_MODE = 0,
    SENSOR_WDR_2_FRAME_MODE,
    SENSOR_WDR_3_FRAME_MODE,
    SENSOR_WDR_4_FRAME_MODE,
};
```

摄像头数据类型：线性 or WDR
```c
enum sensor_data_type {
    SENSOR_DATA_TYPE_LINEAR = 0,
    SENSOR_DATA_TYPE_WDR_FS,
    SENSOR_DATA_TYPE_WDR_DOL,
    SENSOR_DATA_TYPE_WDR_NATIVE,
};
```

##### 8. sensor控制配置结构体

```c
struct sensor_ctrl_ops {
    /* sensor工作的基本功能 */
    int (*power_on)(void);      // 摄像头外部时钟使能、上电、寄存器初始化
    void (*power_off)(void);    // 摄像头掉电、外部时钟使能
    int (*stream_on)(void);     // 摄像头开流
    void (*stream_off)(void);   // 摄像头关流

    /* sensor register debug */
    int (*get_register)(struct sensor_dbg_register *reg);   // 获取摄像头寄存器的值
    int (*set_register)(struct sensor_dbg_register *reg);   // 设置摄像头寄存器的值

    /* sensor过ISP时调用的接口 */
    int (*set_expo)(int value);
    unsigned int (*alloc_integration_time)(unsigned int it, unsigned char shift, unsigned int *sensor_it);
    int (*set_integration_time)(int value);
    unsigned int (*alloc_again)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again);
    int (*set_analog_gain)(int value);
    unsigned int (*alloc_dgain)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain);
    int (*set_digital_gain)(int value);

    int (*set_fps)(int fps);                // 设置 sensor 输出帧率
    int (*set_hvflip)(int hvflip_mode);     // 设置 sensor 翻转/镜像

	/* 如果sensor支持WDR，可配置以下接口 */
#if 1
    int (*set_expo_short)(int value);
    unsigned int (*alloc_integration_time_short)(unsigned int it, unsigned char shift, unsigned int *sensor_it);
    int (*set_integration_time_short)(int value);
    unsigned int (*alloc_again_short)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again);
    int (*set_analog_gain_short)(int value);
    unsigned int (*alloc_dgain_short)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain);
    int (*set_digital_gain_short)(int value);

    int (*get_black_pedestal)(int value);   // 暂不支持
    int (*set_wdr)(int wdr_en);
#endif

    /* for vic only */
    int (*get_hflip)(camera_ops_mode *mode);    // 获取 sensor 镜像状态
    int (*set_hflip)(camera_ops_mode mode);     // 设置 sensor 镜像状态
    int (*get_vflip)(camera_ops_mode *mode);    // 获取 sensor 翻转状态
    int (*set_vflip)(camera_ops_mode mode);     // 设置 sensor 翻转状态

    int (*reset_fmt)(struct camera_info *info); // 重置 sensor 格式

    /* !!note: run in interrupt context */
    int (*frame_start_callback)(void);      // 帧开始回调
    int (*frame_done_callback)(void);       // 帧结束回调
};
```

sensor 寄存器属性
```c
struct sensor_dbg_register {
    unsigned long long reg; /* 寄存器地址 */
    unsigned long long val; /* 寄存器值 */
    unsigned int size;      /* 寄存器值占用字节数, 单位：byte */
};
```

摄像头功能开关
```c
typedef enum {
    CAMERA_OPS_MODE_DISABLE = 0,    /* 不使能该模块功能 */
    CAMERA_OPS_MODE_ENABLE,         /* 使能该模块功能 */
} camera_ops_mode;
```



### 2.3 注册Sensor

#### 2.3.1 注册注销函数说明

```c
int camera_register_sensor(int index, struct sensor_attr *sensor)

功能：注册一个sensor到camera驱动
参数：sensor         // 将第2.2章配置的sensor结构体变量传入
     index          // 仅限为0
返回值：0  注册成功;  非0  注册失败

void camera_unregister_sensor(int index, struct sensor_attr *sensor)

功能：注销sensor
参数: sensor         // 将第2.2章配置的sensor结构体变量传入
     index          // 仅限为0
返回值：无
```

#### 2.3.2 注册注销实例

可以参考下面君正添加的ov9282 sensor 是如何注册sensor的

```c
static int ov9282_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int i;
    int ret = init_gpio();
    if (ret)
        return ret;

    /**********注册ov9282 sensor*************/
    ret = camera_register_sensor(camera_index, &ov9282_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int ov9282_remove(struct i2c_client *client)
{
    /**********注销ov9282 sensor*************/
    camera_unregister_sensor(camera_index, &ov9282_sensor_attr);
    deinit_gpio();
    return 0;
}
```



### 2.4 加入IConfig配置界面以及加入编译

在package/devices/camera/x2580/目录下，添加新的camera sensor模块文件夹，

文件夹包含Config.in文件、.mk文件

#### 2.4.1 加入IConfig配置界面

##### 1. sensor Config.in文件
可参考以下ov9282_mipi sensor的Config.in实例，也可借鉴其他目录的Config.in文件：

```shell
menuconfig MD_X2580_SENSOR_OV9282_MIPI
    bool "sensor ov9282 (mipi,raw8)"
    select MD_X2580_CAMERA_VIC
    depends on MD_SOC_X2580

config MD_X2580_OV9282_GPIO_RESET
    string "gpio reset(sensor 复位脚)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X2580_OV9282_GPIO_PWDN
    string "gpio pwdn(sensor 电源控制引脚, 低有效)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X2580_OV9282_GPIO_POWER
    string "gpio power(sensor 电源控制引脚, 高有效)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X2580_OV9282_I2C_BUSNUM
    int "i2c bus num (sensor 挂接的i2c总线号)"
    default -1

endmenu # MD_X2580_SENSOR_OV9282_MIPI
```

##### 2. x2580 camera Config.in文件
在package/device/camera/Config.in文件末尾添加：

**source package/devices/camera/x2580/"添加的camera模块文件夹"/Config.in**

以ov9282_mipi为例：source package/devices/camera/x2580/ov9282_mipi/Config.in

> IConfig 详细说明请参考《IConfig说明文档》

#### 2.4.2 加入编译

##### 1. sensor .mk文件
可参考以下ov9282_mipi sensor的ov9282_mipi.mk实例.

```shell
#-------------------------------------------------------
package_name = sensor_ov9282_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2580/ov9282_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov9282_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ov9282_mipi_init_file = output/sensor_ov9282_mipi.sh

define sensor_ov9282_mipi_finalize_hook
	$(Q)cp devices/camera/x2580/ov9282_mipi/sensor_ov9282_mipi.ko output/
	$(Q)echo -n 'insmod sensor_ov9282_mipi.ko' > $(sensor_ov9282_mipi_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2580_OV9282_GPIO_POWER)' >> $(sensor_ov9282_mipi_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2580_OV9282_GPIO_RESET)' >> $(sensor_ov9282_mipi_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2580_OV9282_GPIO_PWDN)' >> $(sensor_ov9282_mipi_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2580_OV9282_I2C_BUSNUM)' >> $(sensor_ov9282_mipi_init_file)
	$(Q)echo -n ' resolution=$(MD_X2580_OV9282_MIPI_RESOLUTION)' >> $(sensor_ov9282_mipi_init_file)
	$(Q)echo  >> $(sensor_ov9282_mipi_init_file)
endef
```

##### 2. x2580 camera .mk文件
在 package/device/camera/camera.mk 文件末尾添加

**package-$("新添加sensor模块对应的宏控") += package/devices/camera/x2580/"新添加sensor模块文件夹"/".mk文件"**

以ov9282_mipi为例：

package-$(MD_X2580_SENSOR_OV9282_MIPI) +=  package/devices/camera/x2580/ov9282_mipi/ov9282_mipi.mk

> .mk文件的详细说明请参考《模块驱动添加流程》