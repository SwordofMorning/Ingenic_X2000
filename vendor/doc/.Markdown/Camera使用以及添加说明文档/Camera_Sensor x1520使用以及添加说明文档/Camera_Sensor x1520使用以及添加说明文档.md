# x1520Camera使用及添加Sensor说明文档

## 1.使用君正添加的Camera Sensor

### 1.1.配置Camera Sensor

关于camera 驱动的配置请参考

1.选择x1520的config文件，进行配置

<img src="img/1.png" style="zoom:100%;" />

> 首先配置camera驱动

2.进入配置模块化驱动

![](img/2.png)

3.x1520 soc相关驱动配置

![](img/3.png)

4.勾选camera驱动，并进入配置

![](img/4.png)

5.配置camera参数，如下可知我们需要配置camera帧缓冲数。

![](img/5.png)

> 然后配置camera sensor
>

6.回到模块化驱动界面，然后进入配置外设

![](img/6.png)

7.camera sensor分为带isp和不带isp的，下面我们进入不带isp的camera设备列表。

![](img/7.png)

8.进入到camera设备列表后，就可以看到下图的界面。

> 这些是目前君正x1520提供的sensor(后续会根据实际需求再添加)

![](img/8.png)

9.进入到sensor配置后，就可以看到sensor ov9732的配置界面

> 每种sensor需要我们配置的内容不同,下面我们看看sensor ov9732的配置选项。

![](img/9.png)

### 1.2.Camera 操作流程及接口说明

1.  open函数        打开sensor设备 /dev/camera
2. mmap函数      内存映射，将应用层的地址与内核层的地址映射，主要目的是为了应用层能够   正常使用从内核层得到的帧buf

3. ioctl函数          下面介绍ioctl操作的相关指令

> #define CMD_get_info                 _IOWR('C', 120, struct camera_info)
>
> ​                                                        // 获得摄像头的信息
>
> #define CMD_power_on               _IO('C', 121)        // 开启摄像头电源
>
> \#define CMD_power_off               _IO('C', 122)        // 关闭摄像头电源
>
> \#define CMD_stream_on              _IO('C', 123)        // 开启摄像头视频数据流
>
> \#define CMD_stream_off              _IO('C', 124)        // 关闭摄像头视频数据流
>
> \#define CMD_wait_frame              _IO('C', 125)        // 等待一帧完成，并获得这一帧图像
>
> \#define CMD_put_frame                _IO('C', 126)        // 将使用完的一帧buf还给camera驱动
>
> \#define CMD_get_frame_count     _IO('C', 127)        // 获取可用帧的数量
>
> \#define CMD_skip_frames              _IO('C', 128)        // 将几帧可用的帧主动放弃，用于存放新生成的帧图像

4. close 函数

> 实际操作可参考 /libhardware2/src/cmds/camera_main.c 和 /libhardware2/src/lib/camera/camera.c 测试代码的编写。



## 2.添加Camera Sensor

### 2.1.添加流程

> 如果现有支持的camera sensor不适用，可按照下面流程添加自己的camera sensor驱动。
>
> **1.添加新的sensor模块**
>
> ​        在/devices/camera/x1520/下添加新的sensor文件夹，其中包含.c文件、Makefile。(可参考同级目录下君正已经添加的sensor代码)。
>
> **2.实现接口**
>
> vic_sensor_config.sensor_power_on()                    // 主要实现sensor的上电功能，包括时钟的使能。
>
> vic_sensor_config.sensor_power_off()                    // 实现sensor关电功能，关时钟。
>
> vic_sensor_config.sensor_stream_on()                   // 实现sensor开流
>
> vic_sensor_config.sensor_stream_off()                   // 实现sensor关流
>
> i2c_driver.probe            // i2c设备驱动的probe函数，主要实现申请相关gpio，
>
> ​                                          //以及调用vic_register_sensor注册sensor
>
> i2c_driver.remove          // i2c设备驱动的probe函数，主要实现释放相关gpio，
>
> ​                                          //以及调用vic_unregister_sensor注册sensor
>
> module_init()                 //驱动的入口函数，主要实现i2c设备驱动的添加以及i2c设备的注册
> module_exit()                //驱动的出口函数，主要实现i2c设备驱动的删除以及i2c设备的注销
>
> **3.配置Sensor结构体**
>
> ​        在新添加的sensor.c代码中定义一个struct vic_sensor_config 类型的变量，并进行配置。该结构体类型的相关说明在第2.2章。
>
> **4.注册Sensor**
>
> ​       调用vic_register_sensor()函数对新添加的Sensor进行注册，并将第一步配置的结构体变量作为实参传入。注册注销函数的相关说明在2.3章。
>
> **5.将sensor.c文件加入IConfig配置界面并且加入编译**
>
> ​       使用IConfigTool对x1520的Camera驱动进行配置，配置方法在第2.4章。
>

### 2.2.配置Sensor结构体

#### 2.2.1.包含头文件

```c
#include "soc/x1520/camera/vic_sensor.h"
```

#### 2.2.2. Sensor结构体说明

```c
摄像头配置结构体

struct vic_sensor_config {
    char *device_name;                      // 设备名称
    unsigned int cbus_addr;                 // 设备地址(SPI or IIC)
    struct camera_info info;                // 配置sensor信息
    sensor_data_dma_mode dma_mode;          /* 控制器DMA输出格式选择 */
    vic_interface vic_interface;            // 选择camera 接口类型
    union {
       struct dvp_bus_info dvp_cfg_info;    // 配置dvp接口信息
       struct mipi_csi_bus_info mipi_cfg_info;// 配置mipi接口信息
    };
    long mclk_rate;                         // 设置mclk时钟频率
    long isp_clk_rate;                      // 配置isp时钟频率

    struct sensor_win_setting *win;         // sensor窗口配置信息
    struct sensor_ctrl_ops *ops;            // sensor控制操作
};

sensor相关信息

struct sensor_ctrl_ops {
    /* base */
    int (*power_on)(void);                  // 摄像头 开电源函数
    void (*power_off)(void);                // 摄像头 关电源函数
    int (*stream_on)(void);                 // 摄像头 开启图像输出函数
    void (*stream_off)(void);               // 摄像头 关闭图像输出函数
};

struct sensor_win_setting {
    void *regs;                             // sensor初始化列表
    int width;                              // sensor初始化列表对应的 宽度
    int height;                             // sensor初始化列表对应的 高度
    sensor_pixel_fmt fmt;                   // sensor初始化列表对应的 格式
    unsigned int fps;                       // sensor初始化列表对应的 帧率[31:16] / [15:0]
};
```

#### 2.2.3.结构体成员详解

```c
摄像头信息结构体(以下信息无需填写,由驱动根据sensor信息更新相关信息)

struct camera_info {
    const char *name;                     // 摄像头名称
    unsigned int width;                   // 图像宽度(一行多少像素点)
    unsigned int height;                  // 图像高度(一帧多少行)
    unsigned int fps;                     // 帧率(暂时未用)
    camera_pixel_fmt data_fmt;            // camera_wait_frame得到的数据格式

    unsigned int line_length;             // 一行的长度,单位字节,          不用配置
                                          // 对于 nv12,nv21, 表示y数据一行的长度
                                          // 另外由此可以算出uv数据偏移 line_length*height
    unsigned int frame_size;              // 一帧数据经过对齐之后的大小      不用配置
    unsigned int frame_nums;              // 帧缓冲总数                   不用配置
    unsigned long phys_mem;               // 用于保存帧缓冲的物理基地址      不用配置
    void *mapped_mem;                     // 用于保存 mmap 后的帧缓冲基地址  不用配置
    unsigned int frame_align_size;        // 用于保存数据对齐之后的大小      不用配置
};
```

```c
选择摄像头输出数据格式(列举部分常用格式 re-define from media-bus-format.h)
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

```c
Camera支持的接口类型
typedef enum {
    VIC_bt656,                    // 目前不支持
    VIC_bt601,                    // 目前不支持
    VIC_mipi_csi,
    VIC_dvp,
    VIC_bt1120,                   // 目前不支持
} vic_interface;
```

```c
VIC DMA控制器输出重新排序格式
/*
 * 只有当数据为YUV,即VIC接口的数据类型为DVP_YUV22/MIPI_YUV422时以下宏才生效
 *
 * SENSOR_DATA_DMA_MODE_YUV422SP0           SENSOR_DATA_DMA_MODE_YUV422SP1
 * VIC输入格式                                VIC输入格式
 * Y U Y V Y U Y V                           Y U Y V Y U Y V
 * Y U Y V Y U Y V                           Y U Y V Y U Y V
 * VIC DMA输出格式                            VIC DMA输出格式
 * Y Y Y Y Y Y Y Y                           Y Y Y Y Y Y Y Y
 * U V U V U V U V                           V U V U V U V U
 *
 * SENSOR_DATA_DMA_MODE_NV12格式             SENSOR_DATA_DMA_MODE_NV21格式
 * VIC输入格式                                VIC输入格式
 * Y U Y V Y U Y V                           Y U Y V Y U Y V
 * Y U Y V Y U Y V                           Y U Y V Y U Y V
 * VIC DMA输出格式                            VIC DMA输出格式
 * Y Y Y Y Y Y Y Y                           Y Y Y Y Y Y Y Y
 * U V U V                                   V U V U
 */
typedef enum {
    SENSOR_DATA_DMA_MODE_RAW       = 0,    /* RAW8/10/12 数据顺序不做调整 */
    SENSOR_DATA_DMA_MODE_YUV422    = 3,    /* YUV422 数据顺序不做调整 */
    SENSOR_DATA_DMA_MODE_YUV422SP0 = 4,    /* YUV422数据顺序按照半平面方式排列0 */
    SENSOR_DATA_DMA_MODE_YUV422SP1 = 5,    /* YUV422数据顺序按照半平面方式排列1 */
    SENSOR_DATA_DMA_MODE_NV12      = 6,    /* YUV422输入格式 NV12输出 */
    SENSOR_DATA_DMA_MODE_NV21      = 7,    /* YUV422输入格式 NV21输出 */
    SENSOR_DATA_DMA_MODE_GREY      = 100,  /* 自定义 YUV422输入格式 GREY输出 */
} sensor_data_dma_mode;
```

```c
DVP接口信息配置
struct dvp_bus_info {
    dvp_data_fmt           dvp_data_fmt;            // DVP接口数据格式
    dvp_gpio_mode          dvp_gpio_mode;           // 选择DVP数据线引脚
    dvp_timing_mode        dvp_timing_mode;         // DVP 接口时序模式
    yuv_data_order         dvp_yuv_data_order;      // DVP YUV数据的转换顺序
    dvp_sync_polarity      dvp_hsync_polarity;      // DVP行同步信号的有效电平
    dvp_sync_polarity      dvp_vsync_polarity;      // DVP帧同步信号的有效电平
    dvp_img_scan_mode      dvp_img_scan_mode;       // DVP图像扫描模式
};
```

```c
DVP接口数据格式
typedef enum {
    DVP_RAW8,
    DVP_RAW10,
    DVP_RAW12,
    DVP_YUV422,
    DVP_RGB565,                  // 目前不支持
} dvp_data_fmt;
```

```c
选择DVP数据线引脚 (相关引脚信息可查看x1520_PM手册的3.5章节)
typedef enum {
    DVP_PA_LOW_10BIT,            // 低10位引脚
    DVP_PA_HIGH_10BIT,           // 低10位引脚
    DVP_PA_12BIT,                // 12位引脚全部使用
    DVP_PA_LOW_8BIT,             // 低8位引脚
    DVP_PA_HIGH_8BIT,            // 高8位引脚
} dvp_gpio_mode;
```

```c
DVP 接口时序模式(时序的详解可参考x1520_PM手册的3.7.1章节)
typedef enum {
    DVP_href_mode,
    DVP_hsync_mode,               // 目前不支持
    DVP_sony_mode,                // 目前不支持
} dvp_timing_mode;
```

```c
选择图像yuv格式转换顺序
/*                         clk1,clk2,clk3,clk4
 * 初始yuv 4字节顺序_1_2_3_4   1    2    3    4
 * 可根据自己的需求，转变成以下顺序
 */
typedef enum {
    order_2_1_4_3,
    order_2_3_4_1,
    order_1_2_3_4,
    order_1_4_3_2,
} yuv_data_order;
```

```c
DVP 同步时序的有效电平
typedef enum {
    POLARITY_HIGH_ACTIVE,           // 高电平有效
    POLARITY_LOW_ACTIVE,            // 低电平有效
} dvp_sync_polarity;
```

```c
DVP图像扫描模式
typedef enum {
    DVP_img_scan_progress,          // 逐行扫描
    DVP_img_scan_interlace,         // 交错扫描(奇数行与偶数行分开扫描)
} dvp_img_scan_mode;
```

```c
MIPI接口信息配置
struct mipi_csi_bus_info {
    mipi_data_fmt data_fmt;            // mipi 数据格式
    int lanes;                         // mipi接收通道个数
    int clk;                           // mipi 时钟频率
};
```

```c
mipi 数据格式
typedef enum {
    MIPI_RAW8,
    MIPI_RAW10,
    MIPI_RAW12,
    MIPI_RGB555,
    MIPI_RGB565,
    MIPI_RGB666,
    MIPI_RGB888,
    MIPI_YUV422,
    MIPI_YUV422_10BIT,                 // 目前不支持
} mipi_data_fmt;
```

### 2.3.注册Sensor

#### 2.3.1. 注册注销函数说明

```c
int vic_register_sensor(struct vic_sensor_config *sensor)

功能：注册一个sensor到camera驱动
参数：sensor         // 将第2.2章配置的sensor结构体变量传入
返回值：0  注册成功  非0  注册失败

void vic_unregister_sensor(struct vic_sensor_config *sensor)

功能：注销sensor
参数：sensor           // 将第2.2章配置的sensor结构体变量传入
返回值：无
```

#### 2.3.2. 注册注销实例

可以参考下面君正添加的ov9732 sensor 是如何注册sensor的

```c
static int sensor_ov9732_probe(struct i2c_client *client,
                 const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    /**********注册ov9732 sensor*************/
    ret = vic_register_sensor(&ov9732_sensor_config);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int sensor_ov9732_remove(struct i2c_client *client)
{
    /**********注销ov9732 sensor*************/
    vic_unregister_sensor(&ov9732_sensor_config);
    deinit_gpio();
    return 0;
}
```

## 2.4.加入IConfig配置界面以及加入编译

在package/devices/camera/x1520/目录下，添加新的camera sensor模块文件夹，

文件夹包含Config.in文件、.mk文件

### 2.4.1 加入IConfig配置界面

1.Config.in文件可参考以下ov9732 sensor的Config.in实例,也可借鉴其他目录的Config.in文件：

```c
menuconfig MD_X1520_SENSOR_OV9732
    bool "sensor ov9732 (dvp,raw10)"
    select MD_X1520_CAMERA
    depends on MD_SOC_X1520

config MD_X1520_OV9732_GPIO_RESET
    string "gpio reset(sensor 复位脚)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X1520_OV9732_GPIO_PWDN
    string "gpio pwdn(sensor 电源控制引脚, 低有效)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X1520_OV9732_I2C_BUSNUM
    int "i2c bus num (sensor 挂接的i2c总线号)"
    default -1

config MD_X1520_OV9732_I2C_ADDR
    int "i2c slave address (sensor 的i2c地址)"
    default 0x36
    choice from X1520_OV9732_I2C_ADDRS

array X1520_OV9732_I2C_ADDRS
    int "ov9732 的i2c地址列表"
    item "0x10" 0x10
    item "0x36" 0x36

endmenu # MD_X1520_SENSOR_OV9732
```

2.在package/device/camera/Config.in文件末尾添加：

source package/devices/camera/x1520/"添加的camera模块文件夹"/Config.in

>   IConfig详细说明请参考《IConfig说明文档》

### 2.4.2 加入编译

1  .mk文件可参考以下ov9732 sensor的ov9732.mk实例.

```c
#-------------------------------------------------------
package_name = sensor_ov9732
package_depends = utils soc_camera
package_module_src = devices/camera/x1520/ov9732
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ov9732_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_ov9732_init_file = output/sensor_ov9732.sh

define sensor_ov9732_finalize_hook
    $(Q)cp devices/camera/x1520/ov9732/sensor_ov9732.ko output/
    $(Q)echo -n 'insmod sensor_ov9732.ko' > $(sensor_ov9732_init_file)
    $(Q)echo -n ' reset_gpio=$(MD_X1520_OV9732_GPIO_RESET)' >> $(sensor_ov9732_init_file)
    $(Q)echo -n ' pwdn_gpio=$(MD_X1520_OV9732_GPIO_PWDN)' >> $(sensor_ov9732_init_file)
    $(Q)echo -n ' i2c_bus_num=$(MD_X1520_OV9732_I2C_BUSNUM)' >> $(sensor_ov9732_init_file)
    $(Q)echo -n ' i2c_addr=$(MD_X1520_OV9732_I2C_ADDR)' >> $(sensor_ov9732_init_file)
    $(Q)echo  >> $(sensor_ov9732_init_file)
endef
```

在package/device/camera/camera.mk文件末尾添加

package-$("新添加sensor模块对应的宏控") += package/devices/camera/x1520/"新添加sensor模块文件夹"/".mk文件"

> .mk文件的详细说明请参考《模块驱动添加流程》