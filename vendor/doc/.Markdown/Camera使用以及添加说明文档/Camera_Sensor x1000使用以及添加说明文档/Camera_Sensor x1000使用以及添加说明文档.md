# x1000Camera使用及添加Sensor说明文档



## 1.使用君正添加的Camera Sensor

### 1.1.配置Camera Sensor

关于camera 驱动的配置请参考

1.选择x1000的config文件，进行配置

<img src="img/1.png" style="zoom:100%;" />

> 首先配置camera驱动

2.进入配置模块化驱动

![](img/2.png)

3.x1000 soc相关驱动配置

![](img/3.png)

4.勾选camera驱动，并进入配置

![](img/4.png)

5.配置camera参数，如下可知我们需要配置camera帧缓冲数。

![](img/5.png)

> 然后配置camera sensor

1.回到模块化驱动界面，然后进入配置外设

![](img/6.png)

2.camera sensor分为带isp和不带isp的，下面我们进入不带isp的camera设备列表。

![](img/7.png)

3.进入到camera设备列表后，就可以看到下图的界面。

> 这些是目前君正x1000提供的sensor(后续会根据实际需求再添加)

![](img/8.png)

4.进入到sensor配置后，就可以看到sensor gc0308的配置界面

> 每种sensor需要我们配置的内容不同,下面我们看看sensor gc0308的配置选项。

![](img/9.png)

### 1.2.Camera 操作流程及接口说明

1.  open函数        打开sensor设备 /dev/camera
2. mmap函数      内存映射，将应用层的地址与内核层的地址映射，主要目的是为了应用层能够正常使用从内核层得到的帧buf

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
> ​        在/devices/camera/x1000/下添加新的sensor文件夹，其中包含.c文件、Makefile。(可参考同级目录下君正已经添加的sensor代码)。
>
> **2.实现接口**
>
> cim_sensor_config.sensor_power_on()                    // 主要实现sensor的上电功能，包括时钟的使能。
>
> cim_sensor_config.sensor_power_off()                    // 实现sensor关电功能，关时钟。
>
> cim_sensor_config.sensor_stream_on()                   // 实现sensor开流
>
> cim_sensor_config.sensor_stream_off()                   // 实现sensor关流
>
> i2c_driver.probe            // i2c设备驱动的probe函数，主要实现申请相关gpio，
>
> ​                                          //以及调用cim_register_sensor注册sensor
>
> i2c_driver.remove          // i2c设备驱动的probe函数，主要实现释放相关gpio，
>
> ​                                          //以及调用cim_unregister_sensor注册sensor
>
> module_init()                 //驱动的入口函数，主要实现i2c设备驱动的添加以及i2c设备的注册
> module_exit()                //驱动的出口函数，主要实现i2c设备驱动的删除以及i2c设备的注销
>
> **3.配置Sensor结构体**
>
> ​        在新添加的sensor.c代码中定义一个struct cim_sensor_config 类型的变量，并进行配置。该结构体类型的相关说明在第2.2章。
>
> **4.注册Sensor**
>
> ​       调用cim_register_sensor()函数对新添加的Sensor进行注册，并将第一步配置的结构体变量作为实参传入。注册注销函数的相关说明在2.3章。
>
> **5.将sensor.c文件加入IConfig配置界面并且加入编译**
>
> ​       使用IConfigTool对x1000的Camera驱动进行配置，配置方法在第2.4章。
>

### 2.2.配置Sensor结构体

#### 2.2.1.包含头文件

```c
 #include "soc/x1000/camera/cim_sensor.h"
```

#### 2.2.2. Sensor结构体说明

```c
摄像头配置结构体

struct cim_sensor_config {
    char *device_name;                      // 设备名称
    unsigned int cbus_addr;                 // 设备地址(SPI or IIC)
    struct camera_info info;                // 配置sensor信息
    cim_interface cim_interface;            // 配置Camera的数据采样模式

    // 控制台每次从摄像头接收四个字节的数据，下面将选择这四个数据的存放顺序
    // 例如index_byte0 = CIM_DVP_BYTE0 表示将第一个接收到的字节数据存放在第一个字节内存中
    // 例如原始数据为yuyv将其转变为yvyu
    // 则配置为index_byte0 = CIM_DVP_BYTE0;
    //        index_byte0 = CIM_DVP_BYTE3;
    //        index_byte0 = CIM_DVP_BYTE2;
    //        index_byte0 = CIM_DVP_BYTE1;
    cim_data_index index_byte0;             // 选择存放在第1个字节内存的数据
    cim_data_index index_byte1;             // 选择存放在第2个字节内存的数据
    cim_data_index index_byte2;             // 选择存放在第3个字节内存的数据
    cim_data_index index_byte3;             // 选择存放在第4个字节内存的数据

    dvp_sync_polarity hsync_polarity;       // 配置行同步信号的有效电平和触发边沿
    dvp_sync_polarity vsync_polarity;       // 配置帧同步信号的有效电平和触发边沿

    dvp_pclk_sample_edge data_sample_edge;  // 选择数据的采集边沿
    long mclk_rate;                         // 设置mclk时钟频率

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
    unsigned int frame_size;              // 一帧数据经过对齐之前的大小      不用配置
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
数据采样模式
typedef enum {
    CIM_ITU656_Progressive_mode,           // 目前不支持
    CIM_ITU656_Interlace_mode,             // 目前不支持
    CIM_sync_mode,                         // 时钟同步模式

} cim_interface;
```

```c
数据接收顺序
typedef enum {
    CIM_DVP_BYTE0,                         // 第1个接收到的一字节数据
    CIM_DVP_BYTE1,                         // 第2个接收到的一字节数据
    CIM_DVP_BYTE2,                         // 第3个接收到的一字节数据
    CIM_DVP_BYTE3,                         // 第4个接收到的一字节数据

} cim_data_index;
```

```c
配置DVP 同步时序的有效电平和触发边沿
typedef enum {
    POLARITY_HIGH_ACTIVE,                 // 有效电平为高电平，触发边沿为上升沿
    POLARITY_LOW_ACTIVE,                  // 有效电平为低电平，触发边沿为下升沿

} dvp_sync_polarity;
```

```c
采样边沿
typedef enum {
    DVP_RISING_EDGE,                      // 采样边沿为上升沿
    DVP_FALLING_EDGE,                     // 采样边沿为下降沿

} dvp_pclk_sample_edge;
```

### 2.3.注册Sensor

#### 2.3.1. 注册注销函数说明

```c
int cim_register_sensor(struct cim_sensor_config *sensor)

功能：注册一个sensor到camera驱动
参数：sensor         // 将第2.2章配置的sensor结构体变量传入
返回值：无

void cim_unregister_sensor(struct cim_sensor_config *sensor)

功能：注销sensor
参数：sensor           // 将第2.2章配置的sensor结构体变量传入
返回值：无
```

#### 2.3.2. 注册注销实例

可以参考下面君正新添加的gc0308 sensor 是如何注册sensor的

```c
1.在probe函数中调用cim_register_sensor函数对sensor进行注册
static int sensor_gc0308_probe(struct i2c_client *client,
                 const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    /**********注册gc0308 sensor*************/
    ret = cim_register_sensor(&gc0308_sensor_config);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}
与之对应的要在remove函数中调用cim_unregister_sensor对sensor进行注销
static int sensor_gc0308_remove(struct i2c_client *client)
{
    /**********注销gc0308 sensor*************/
    cim_unregister_sensor(&gc0308_sensor_config);
    deinit_gpio();
    return 0;
}
```

## 2.4.加入IConfig配置界面以及加入编译

在package/devices/camera/x1000/目录下，添加新的camera sensor模块文件夹，

文件夹包含Config.in文件、.mk文件

### 2.4.1 加入IConfig配置界面

1.Config.in文件可参考以下gc0308 sensor的Config.in实例,也可借鉴其他目录的Config.in文件：

```c
menuconfig MD_X1000_SENSOR_GC0308
    bool "sensor gc0308 (dvp,yuyv)"
    select MD_X1000_CAMERA
    depends on MD_SOC_X1000

config MD_X1000_GC0308_GPIO_RESET
    string "gpio reset(sensor 复位脚)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X1000_GC0308_GPIO_PWDN
    string "gpio pwdn(sensor 电源控制引脚)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X1000_GC0308_GPIO_POWER_EN
    string "gpio power_en(sensor 模块电源控制引脚)"
    choice from SOC_GPIO_PINS
    default -1

config MD_X1000_GC0308_I2C_BUSNUM
    int "i2c bus num (sensor 挂接的i2c总线号)"
    default -1

choice "    选择格式"
    default MD_X1000_SENSOR_GC0308_FMT_YUYV

config MD_X1000_SENSOR_GC0308_FMT_YUYV
    bool "fmt yuyv"

config MD_X1000_SENSOR_GC0308_FMT_Y8
    bool "fmt y8"

endchoice

endmenu # MD_X1000_SENSOR_GC0308
```

2.在package/device/camera/Config.in文件末尾添加：

source package/devices/camera/x1000/"添加的camera模块文件夹"/Config.in

>   IConfig详细说明请参考《IConfig说明文档》。

### 2.4.2 加入编译

1  .mk文件可参考以下gc0308 sensor的gc0308.mk实例.

```c
#-------------------------------------------------------
package_name = sensor_gc0308
package_depends = utils soc_camera
package_module_src = devices/camera/x1000/gc0308
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_gc0308_finalize_hook
package_clean_hook =
\#-------------------------------------------------------
sensor_gc0308_init_file = output/sensor_gc0308.sh

define sensor_gc0308_finalize_hook

$(Q)cp devices/camera/x1000/gc0308/sensor_gc0308.ko output/
$(Q)echo -n 'insmod sensor_gc0308.ko' > $(sensor_gc0308_init_file)
$(Q)echo -n ' power_gpio=$(MD_X1000_GC0308_GPIO_POWER_EN)' >> $(sensor_gc0308_init_file)
$(Q)echo -n ' reset_gpio=$(MD_X1000_GC0308_GPIO_RESET)' >> $(sensor_gc0308_init_file)
$(Q)echo -n ' pwdn_gpio=$(MD_X1000_GC0308_GPIO_PWDN)' >> $(sensor_gc0308_init_file)
$(Q)echo -n ' i2c_bus_num=$(MD_X1000_GC0308_I2C_BUSNUM)' >> $(sensor_gc0308_init_file)
$(Q)echo -n ' use_y8=$(if $(MD_X1000_SENSOR_GC0308_FMT_Y8),1,0)'
    >> $(sensor_gc0308_init_file)
$(Q)echo >> $(sensor_gc0308_init_file)
endef
```

在package/device/camera/camera.mk文件末尾添加

package-$("新添加sensor模块对应的宏控") += package/devices/lcd/x1000/"新添加sensor模块文件夹"/".mk文件"

> .mk文件的详细说明请参考《模块驱动添加流程》