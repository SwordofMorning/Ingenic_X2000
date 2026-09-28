



# x2000 libisp 使用说明文档



Linux 工程下的 libisp 包含了 x2000 平台 camera 经过 ISP 的一些库、API 以及示例代码。其中除了 sensor 出图的基本功能，还有屏幕实时预览、双摄帧同步、多通道预览的功能。

本文将介绍 libisp 的组成、应用以及调用的 API 等。



## 1 目录组成

```c
├── config.h               /* libisp配置的头文件，里面是一些配置的宏定义 */
├── Config.in              /* libisp的配置文件 */
├── include                /* libisp的头文件 */
│   └── libisp
│       ├── isp.h         /* 声明isp.c中定义的函数 */
│       └── isp_tuning.h  /* 声明isp_tuning.c中定义的函数 */
├── libisp_cmds.mk         /* 编译libisp各个模块的makefile */
├── libisp.mk              /* 编译libisp库文件的makefile */
├── Makefile               /* 编译整个libisp的Makefile */
├── README                 /* libisp说明文本 */
├── src                    /* 源代码 */
│   ├── cmds              /* Camera shell命令cmd_isp的主函数 */
│   ├── demo              /* Camera shell命令的各类demo */
│   └── lib               /* Camera shell命令cmd_isp的库函数，通过ioctl与驱动通信 */
└── tools
```

**重要部分：**

- libisp/src/cmds

  ```c
  ├── cmds
  │   ├── isp_main.c               /* Camera shell命令 cmd_isp 的主函数 */
  │   └── nv12_rotate_example.c    /* NV12 旋转示例
                                        * 该文件不参与编译, 可复制到 isp_main.c 测试功能 */
  ```

- libisp/src/demo

  ```C
  ├── demo
  │   ├── demo_frame_sync.c           /* isp shell命令 demo_frame_sync 的源码 */
  │   ├── demo_isp.c                  /* isp shell命令 demo_isp 的源码 */
  │   ├── demo_isp_h264_encode.c      /* isp shell命令 demo_isp_h264_encode 的源码 */
  │   ├── demo_isp_jpeg_encode.c      /* isp shell命令 demo_isp_jpeg_encode 的源码 */
  │   ├── demo_isp_nv12_preview.c     /* isp shell命令 demo_isp_nv12_preview 的源码 */
  │   └── demo_mult_chan_preview.c    /* isp shell命令 demo_mult_chan_preview 的源码 */
  ```

- libisp/src/lib

  ```C
  └── lib
      ├── isp.c         /* Camera shell命令 cmd_isp 的库函数，通过ioctl与驱动通信 */
      └── isp_tuning.c  /* ISP图像信号处理
                            * 主要包含图像效果设置操作，与数据流无关
                            * 仅作用于效果参数设置及Sensor控制 */
  ```




## 2 ISP 出图流程

### 2.1 camera 出图方式

x2000 的 camera 模块出图一共有三种方式：

<center style="color:#050505;text-decoration">表 2-1-1 camera 出图方式</center>

| 方式  | 接口/模块               | 数据流                                             |
| :---- | :---------------------- | :------------------------------------------------- |
| 方式1 | CIM 接口                | DVP / MIPI(0)  → CIM → DDR                         |
| 方式2 | VIC 接口（不通过 ISP ） | DVP / MIPI(0, 1) → VIC(0, 1) →DDR                  |
| 方式3 | VIC + ISP 模块          | DVP / MIPI(0, 1) → VIC(0, 1) → ISP → Mscaler → DDR |

这些方式可根据图2-1-1来理解，数据流需要过 ISP 的为**方式3**。


![1](img/1.png)
<center style="color:#808080;text-decoration">图2-1-1 camera 三种出图方式</center>

其中，tiziano 为 vic + isp + mscaler 的合称。

其余各组成部分可见下表，简单说明了它们各自的功能。

<center style="color:#050505;text-decoration">表 2-1-2  camera 出图各模块说明</center>

| 模块名称 | 模块说明                                                     |
| -------- | :----------------------------------------------------------- |
| MIPI-CSI | MIPI Camera Serial Interface，MIPI 摄像头串行接口。一种广泛采用的高速相机接口，<br />采用**串行**输出方式，主要用于摄像机和主机设备之间的点对点图像和视频传输。 |
| DVP      | Digital Video Port，数字视频端口。一种传统的sensor输出接口，采用**并行**输出方式。 |
| CIM      | Camera interface module，摄像头接口模块。君正 x2000 平台下的CIM支持DVP、<br />MIPI -CSI接口输入，支持RGB888、RGB565、YCbCr4:2:2 、MONO(8)输入数据格式。 |
| VIC      | Video Interface Control，视频接口控制器。君正 x2000 平台下一个视频输入图像<br />预处理模块，支持DVP、MIPI、BT接口输入，支持RAW8、RAW10、RAW12、<br />YUV422输入数据格式。 |
| ISP      | Image Signal Processing，图像信号处理模块，**x2000 isp 最大支持分辨率2048×2048。** |
| Mscaler  | Multiple Channel Scaler，多通道缩放器。君正 x2000 平台下用于图像后处理，<br />包括缩放、裁剪。有**3个输出通道，支持输出 NV12、NV21、GREY 数据格式**。 |
| DDR      | 内存                                                         |



#### 2.1.1 三种出图方式说明

- 方式1：CIM 控制器只能接收来自于 MIPI-CSI 0 或 DVP 接口的 sensor 图像帧数据，并直接送到 DDR。

- 方式2：两个 VIC 控制器输出的数据直接送到 DDR，其中 **VIC 0 可接收来自 MIPI-CSI 0 或 DVP 接口的数据，VIC 1 接收的是来自 MIPI-CSI 1 的数据**。

- 方式3：VIC 0 和 VIC 1 输出的数据分别通过 ISP 0 和 ISP 1 后，再经过 Mscaler 0/Mscaler 1 到达 DDR。



#### 2.1.2 各种方式的区别

##### 1. camera 接口控制器：VIC 或 CIM

X2000芯片 支持 CIM 与 VIC 两种控制器，它们的主要特性列于表2-1-3.

- VIC控制器：一个视频输入图像预处理模块，对于硬件 DVP / MIPI 接口支持 RAW8、RAW10、RAW12、YUV422 数据输入。
  DVP / MIPI 接口输入的 **RAW 格式最大支持12bit**，DVP 接口输入的 YUV422 格式最大支持 8 bit.
- CIM控制器：通过 DVP 或 MIPI 接口获取数字图像流，支持YCbCr 4:2:2(YUV422), RGB888, RGB565、MONO(8)格式输入，**CIM 只能支持 8 bit  数据.**

<center style="color:#050505;text-decoration">表 2-1-3   vic 控制器及cim 控制器主要特点对比</center>

| 控制器 | 支持接口                                      | 支持输入数据格式                                             | 支持输出数据格式                   | 最大支持分辨率                                               |
| ------ | --------------------------------------------- | ------------------------------------------------------------ | ---------------------------------- | ------------------------------------------------------------ |
| VIC    | DVP , <br />MIPI, <br />BT（<u>暂未调试</u>） | **MIPI:** RAW8, RAW10, RAW12<br />**DVP:** RAW8, RAW10, RAW12,<br />YUV422 (Serial 8bit)<br/>BT: YUV422 | RAW8, RAW16, <br />YUV422 (8bit)   | MIPI: 8M@30fps, 6M@30fps, <br />**1080p@60fps**<br/><br />BT/DVP: 1080p@30fps, <br />720p@60fps |
| CIM    | DVP,  MIPI                                    | ITU656, YCbCr 4:2:2, <br />RGB888, RGB565, MONO(8)           | YUV422, RGB888, <br />RGB565, MONO | 2047x2047 input and output                                   |

##### 2. 是否经过ISP模块

- VIC 控制器出的数据可过 ISP
- CIM 控制器出的数据不可过 ISP



#### 2.1.3 小结

以上展示了camera 三种出图方式的框架、流程及各方式之间的不同之处。总的来说，一个经过 ISP 的 sensor 出图流程（方式3）为：**camera 的图像信息从 MIPI 或 DVP 接口发送，由 vic 控制器决定是否需要经过 isp，如果”是“，则数据送到 isp 模块，最后由 Mscaler 模块缩放后存储到 DDR。**

 Linux 工程的 libisp 则是一个基于上述流程的 ISP、Video 等多媒体功能的集成，libisp 通过调用模块驱动提供的接口，在**应用层**上实现 sensor 经过 ISP 的图像输出、保存等操作。



### 2.2 经过 ISP 的出图流程

#### 2.2.1 Iconfig 配置

根据板级信息、sensor 型号，用 IconfigTool 打开一个工程配置文件（以 x2000_darwin_nand_deconfig 配置 sensor gc2053 为例）。

详细配置流程请查看 *开发使用手册/Camera使用以及添加说明文档/**Camera_Sensor 通用使用手册.pdf***。

需要完成表2-2-1所列的模块配置。

<center style="color:#050505;text-decoration">表2-2-1 过isp的Iconfig模块配置</center>

| 配置模块   | 配置说明                                                     |
| :--------- | :----------------------------------------------------------- |
| sensor     | 配置 sensor 的名称、power 引脚、power down 引脚、reset 引脚、<br />i2c 总线号、i2c 设备地址、sensor 挂接的 vic/cim 总线号 |
| i2c        | 配置 I2C 总线号、i2c 时钟引脚、i2c 数据引脚                  |
| soc camera | 配置 mclk 引脚、ISP 使能控制、frame buffer 数量              |
| libisp     | 启用 x2000 ISP 接口、命令                                    |

配置过程示例如下：

##### 1. 配置 sensor

路径: [root] - [模块化驱动] - [外设] - [camera设备列表] - [sensor gc2053 (mipi, raw10) ] - [sensor0 config]

说明：

- gpio reset 、gpio pwdn、gpio power 要根据电路原理图来选择引脚
- i2c 总线号依据原理图中 sensor 的 SCL, SDA 引脚而填写，i2c bus num 和 gpio 的对应关系在 i2c 模块配置
- i2c device addr 是 sensor 的 i2c 地址，可通过 sensor 的数据手册获得
- cam bus num 是 sensor 挂接的 vic/cim 总线号，**可供填写的有 0、1、2，分别对应 VIC0、VIC1、CIM**（在 soc camera 模块配置）

![2](img/2.png)

注：

- 只开单摄时，gpio power 按照电路原理图配置引脚；
- 开双摄时，若 **sensor0、 sensor1的电源引脚相同**，则先将 sensor0、sensor1 的 gpio power 都设为 -1，**通过名字**选择电源管理器(例如：“GPIO-POWER0” 表示 regulator0)，随后配置电源管理器的 regulator pin 为 sensor电源引脚；若 sensor0、 sensor1 电源引脚不同，则 gpio power 按照电路原理图配置即可。

例如：开启双摄时

1）将 sensor0 和 sensor1 的 gpio power 都改为 -1;

2）regulator name 都写 “GPIO-POWER0";

3）到电源管理器路径下配置 regulator0 pin、有效电平。

电源管理器路径：[ root ] - [ 模块化驱动 ] - [ 外设 ] - [ 设备电源管理 ] - [ gpios regulator ] - [ regulator0 config ]

![3](img/3.png)

##### 2. 配置 i2c

 路径：[模块化驱动] - [X2000驱动列表] - [i2c 控制器驱动]

说明：

- 根据需要选择 I2C 号
- 按照需求，进一步选择GPIO SCL、GPIO SDA组

![4](img/4.png)
![5](img/5.png)

##### 3. 配置 soc camera（过 ISP )

路径： [root] - [模块化驱动] - [X2000驱动列表] - [camera 驱动（vic/cim控制器）]

说明：

- 开启的vic/cim控制器接口要和sensor配置的"cam bus num"对应：**开启VIC0，则cam bus num 写 0；开启VIC1，则cam bus num 写 1；开启CIM，则cam bus num 写 2**
- MCLK 引脚根据原理图选择
- 数据通路的选取决定过不过 ISP （选择CIM控制器时无此选项）
- Camera 缓冲帧数一般设为 **2**（此处配置的缓冲帧数对**不通过 ISP 时有效**；过 ISP 时，帧缓冲数由libisp 应用代码控制）

![6](img/6.png)

选择“VIC 数据通过ISP模块”。

![7](img/7.png)

##### 4. 配置 ISP 接口/命令

路径：[root] - [libisp (X2000 ISP接口/命令) ]

说明：

- 当 vic 控制器选择通过 ISP 模块时，需要勾选 libisp
- 进入libisp，根据需要开启命令、接口

![8](img/8.png)

![9](img/9.png)

保存配置。

**注意**：

- libisp 中的各个接口之间的配置可能有差异，勾选的接口不一定全都能使用，例如demo_isp_nv12_preview 还需要配置屏幕相关驱动等；

- 完成以上配置后，支持的 ISP 接口有：isp shell 命令、demo_isp、demo_isp_jpeg_encode



### 2.3 libisp 应用

> **应用须知：**
>
> ​    **选择通过ISP模块时，camera 设备节点名为 mscaler。**
>
> - **开启单摄时为 mscaler0 或 mscaler1，对应的是 sensor0 或 sensor1；**
> - **开启双摄时为 mscaler0 和 mscaler1，分别对应 sensor0 和 sensor1；**
> - **每个mscaler支持3个 channel，故完整的设备名为 mscalerx-chx.**
>
> **例如：mscaler0-ch0 表示isp0，通道0的设备节点。**

libisp 有基本的 isp 出图接口以及多个模块测试 demo，下表展示各个接口/模块的使用流程和功能简介。

<center style="color:#050505;text-decoration">表2-3-1 libisp 模块功能及使用流程</center>

| 接口/模块             | 功能                                                         | 使用流程                                                     |
| --------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| isp shell 命令        | 1. 摄像头初始化/取消初始化<br />2. 上电/掉电<br />3. 开流/关流<br />4. 获取/丢弃图像帧 | cmd_isp  init  [device_path]  [width=value]<br /> [height=value]  [frame_nums]  [format] <br/>cmd_isp  power_on   [device_path] <br/>cmd_isp  stream_on  [device_path] <br/>cmd_isp  get_frame  [device_path]  >  [save_path] |
| demo_isp              | 持续出图并保存                                               | demo_isp  [device_path]                                      |
| demo_isp_nv12_preview | 1. 屏幕预览<br />2. 限制帧数的屏幕预览                       | 屏幕预览：<br />demo_isp_nv12_preview   [device_path]<br /> 限制帧数的屏幕预览：<br />demo_isp_nv12_preview   [device_path]  [frame_conut] |
| mult_chan_preview     | 1. 单通道预览<br />2. 双通道预览                             | 单通道：<br />demo_mult_chan_preview  [device_path] <br/>双通道：<br />demo_mult_chan_preview  [device_path0] [device_path1] |
| frame sync            | 双摄帧同步                                                   | demo_frame_sync                                              |
| demo_isp_jpeg_encode  | 图像编码为jpeg格式并保存                                     | demo_isp_jpeg_encode  output=[save_path]                     |
| demo_isp_h264_encode  | 图像编码为h264格式并保存                                     | demo_isp_h264_encode  output=[save_path]                     |

注：demo_isp_nv12_preview   限制帧数的屏幕预览是**只预览一定帧数的图像**，超过指定帧数就停止预览；jpeg、h264 图像编解码相关内容请参考 *开发使用手册/**VPU编解码驱动和应用***。

#### 2.3.1 camera isp shell 命令

<center style="color:#050505;text-decoration">表2-3-2 camera isp shell 命令的功能及使用示例</center>

| shell 命令              | 功能                     | 使用示例                                                     |
| ----------------------- | ------------------------ | ------------------------------------------------------------ |
| **cmd_isp init**        | 初始化摄像头             | cmd_isp init /dev/mscaler0-ch0<br/>or<br />cmd_isp init /dev/mscaler0-ch0 width=1920 height=1080 <br />frame_nums=3 format=NV12 |
| **cmd_isp power_on**    | 使能 camera 设备，上电   | cmd_isp power_on /dev/mscaler0-ch0                           |
| cmd_isp power_off       | 关闭 camera 设备，掉电   | cmd_isp power_off /dev/mscaler0-ch0                          |
| **cmd_isp stream_on**   | 开始 camera 图像录制     | cmd_isp stream_on /dev/mscaler0-ch0                          |
| cmd_isp stream_off      | 结束 camera 图像录制     | cmd_isp stream_off /dev/mscaler0-ch0                         |
| cmd_isp info            | 获取 camera 设备信息     | cmd_isp info /dev/mscaler0-ch0                               |
| **cmd_isp get_frame**   | 获取一帧的数据到标准输出 | cmd_isp get_frame /dev/mscaler0-ch0                          |
| cmd_isp drop_all_frames | 丢弃已录制的图像数据帧   | cmd_isp drop_all_frames /dev/mscaler0-ch0                    |
| cmd_isp deinit          | 取消初始化               | cmd_isp deinit /dev/mscaler0-ch0                             |

**cmd_isp 出图示例：**

```c
cmd_isp init /dev/mscaler0-ch0 width=1920 height=1080 frame_nums=3 format=NV12
/*初始化， format 是图像输出格式，支持三种输出格式：NV12 NV21 GREY*/
cmd_isp power_on /dev/mscaler0-ch0                  /*使能 camera 设备*/
cmd_isp stream_on /dev/mscaler0-ch0                 /*打开图像录制*/
cmd_isp get_frame /dev/mscaler0-ch0 > /tmp/frame    /*获取图片并保存*/
```



#### 2.3.2 camera demo isp shell命令

<center style="color:#050505;text-decoration">表2-3-3 demo isp 模块 shell 命令功能及使用示例</center>

| demo_isp shell 命令    | 功能                                    | 使用示例                                                     |
| ---------------------- | --------------------------------------- | ------------------------------------------------------------ |
| demo_isp               | 持续出图并保存到<br />/tmp/frame.nv12   | demo_isp /dev/mscaler0-ch0                                   |
| demo_isp_nv12_preview  | 1. 屏幕预览<br />2.  限制帧数的屏幕预览 | demo_isp_nv12_preview   /dev/mscaler0-ch0<br />or<br />demo_isp_nv12_preview   /dev/mscaler0-ch0  **100** （只预览**100** 帧） |
| demo_mult_chan_preview | 1. 单通道预览<br />2. 双通道预览        | 单通道：<br />demo_mult_chan_preview  /dev/mscaler0-ch0<br/>双通道：<br />demo_mult_chan_preview  /dev/mscaler0-ch0 /dev/mscaler1-ch0 |
| demo_frame_sync        | 双摄帧同步                              | 使用默认时间戳（15ms）开始帧同步：<br />demo_frame_sync<br />自定义时间戳（e.g. 21ms）开始帧同步：<br />demo_frame_sync  -t  21000 |
| demo_isp_jpeg_encode   | 图像编码为jpeg格式并保存                | demo_isp_jpeg_encode  output=/tmp/isp.jpeg                   |
| demo_isp_h264_encode   | 图像编码为h264格式并保存                | demo_isp_h264_encode  output=/tmp/output                     |



**如果想在电脑端查看图片**

（1）将板子*/tmp* 里的图像拉到电脑某路径下

示例：

```c
NV12/NV21/GREY格式:
    adb pull /tmp/frame.nv12  /tmp/photo
JPEG格式:
    adb pull /tmp/isp.jpeg  /tmp/isp.jpeg
H264格式:
    adb pull /tmp/output  /tmp/output
```

（2）查看图像

- NV12/NV21/GREY格式

  工具：可用软件 7yuv

  设置：图片尺寸与格式信息

  <center style="color:#050505;text-decoration">表2-3-4 libisp 输出图像格式与 7yuv 对应关系</center>

  | 初始化格式 |    7yuv Format     |
  | :--------: | :----------------: |
  |    NV12    | YVU420 planer NV12 |
  |    NV21    | YVU420 planer NV21 |
  |    GREY    |       8 bpp        |

  示例：若取图初始化为 width=1920  height=1080  format=NV12。则在 7yuv 工具选择格式、分辨率，如下图所示。

  ![10](img/10.png)
  <center style="color:#808080;text-decoration">图2-3-1 7yuv工具设置图片格式与分辨率</center>



  出图效果：

  ![11](img/11.png)
  <center style="color:#808080;text-decoration">图2-3-2 nv12图片</center>

- JPEG格式

  PC直接双击显示。

- H264格式

  linux 终端进入电脑上图片存放路径，用 ffplay shell 命令打开。

  ```shell
  $ ffplay output
  ```



## 3 ISP 接口

### 3.1 接口结构体

libisp 中主要的isp接口结构体：

<center style="color:#050505;text-decoration">表3-1-1 libisp 中主要的isp接口结构体</center>

| 结构体                                             | 定义路径                        | 说明                                                         |
| -------------------------------------------------- | ------------------------------- | ------------------------------------------------------------ |
| camera 信息结构体<br />struct  camera_info         | /module_driver/include/camera.h | 1. 存放摄像头相关的信息<br />2. 成员变量通过 ioctl 从模块驱动获取<br />3. 不可更改 |
| 图像帧信息结构体<br />struct  frame_info           | /module_driver/include/camera.h | 1. 存放图像帧的相关等信息<br />2. 成员变量是通过 ioctl 从模块驱动获取<br />3. 不可更改 |
| isp 出图属性结构体<br />struct  frame_image_format | /libisp/include/isp.h           | 1. 存放输出图片的大小、格式相关信息<br />2. **可修改**结构体的成员变量 |

下面将展开介绍以上结构体。



#### 3.1.1 camera 信息结构体

##### 1. 结构体定义

camera_info 结构体的定义位于：*/module_driver/include/camera.h*

```c
struct camera_info {
    char name[64];                  //摄像头名称
    unsigned int width;             //每行像素数
    unsigned int height;            //行数
    unsigned int fps;               //camera 帧率
    camera_pixel_fmt data_fmt;      //camera 帧数据格式

    unsigned int line_length;       //一行的⻓度,单位字节
    unsigned int frame_size;        //一帧数据经过对⻬之前的大小
    unsigned int frame_nums;        //帧缓冲总数
    unsigned long phys_mem;         //帧缓冲的物理基地址
    void *mapped_mem;               //mmap 后的帧缓冲基地址
    unsigned int frame_align_size;  //帧对⻬大小
};
```

其中 camera_pixel_fmt 是枚举型的像素格式集合，其定义位于 */module_driver/include/camera_pixel_format.h*

##### 2. 用途

camera_info 结构体是用于存放和摄像头相关的信息，其成员变量是通过 ioctl 从模块驱动获取，因此**不能更改。**



#### 3.1.2 图像帧信息结构体

##### 1. 结构体定义

frame_info 结构体的定义位于：*/module_driver/include/camera.h*

```c
struct frame_info {
    unsigned int index;             // 缓存编号
    unsigned int sequence;          // 帧序列号

    unsigned int width;             // 帧宽
    unsigned int height;            // 帧高
    unsigned int pixfmt;            // 帧的图像格式
    unsigned int size;              // 帧所占用空间大小
    void *vaddr;                    // 帧的虚拟地址
    unsigned long paddr;            // 帧的物理地址

    unsigned long long timestamp;   // 帧的时间戳，单位微秒，单调时间
    unsigned int isp_timestamp;     // isp时间戳，单位微秒，在vic通过isp clk计数算出
                                    // 最大值10000秒左右（和isp clk相关），大于最大值
                                    // 重新清零计时
    unsigned int shutter_count;     // 曝光计数
};
```

##### 2. 用途

frame_info 结构体包含了图像帧的序号、大小、地址、时间戳、计数等信息，用于实现对图像帧的存取、保存、释放，各成员变量是通过 ioctl 从模块驱动获取，因此也**不能更改**。



#### 3.1.3  isp 出图属性结构体

##### 1. 结构体定义

摄像头出图属性结构体 frame_image_format 的定义位于：*/libisp/include/libisp/isp.h*

```c
struct frame_image_format {
    unsigned int width;            //ISP MScaler 输出的宽
    unsigned int height;           //ISP MScaler 输出的高
    unsigned int pixel_format;     //ISP MScaler 输出格式
    unsigned int frame_size;       //ISP MScaler 帧大小

    struct channel_scaler scaler;  //ISP MScaler 缩放属性
    struct channel_crop crop;      //ISP MScaler 裁剪属性

    int frame_nums;                //ISP MScaler 缓存个数，一般写2（驱动+1, 最后申请frame个数是3）
};
```

 其中 ISP MScaler 输出格式 pixel_format 是 */module_driver/include/camera_pixel_format.h* 中定义的一部分格式，libisp 仅支持三种输出格式：

```c
        CAMERA_PIX_FMT_NV12,        // NV12
        CAMERA_PIX_FMT_NV21,        // NV21
        CAMERA_PIX_FMT_GREY,        // GREY
```

缩放与裁剪属性结构体：

```c
struct channel_crop {              //裁剪属性
    int enable;                    //裁剪使能
    unsigned int top;              //裁剪顶部起始
    unsigned int left;             //裁剪左侧起始
    unsigned int width;            //裁剪后的宽
    unsigned int height;           //裁剪后的高
};
```

```c
struct channel_scaler {            //缩放属性
    int enable;                    //缩放使能
    unsigned int width;            //缩放后的宽
    unsigned int height;           //缩放后的高
};
```

##### 2. 用途

frame_image_format 结构体用于设置输出图片的大小、格式，用户根据自身需求，**可修改**该结构体的成员变量，以选择是否缩放、是否裁剪以及图像最终输出格式。

##### 3. 示例

前提：sensor 原始输出分辨率为 1920×1080。

（1） 输出原始图像大小  （scaler.enable = 0 ；crop.enable = 0）

此时缩放使能、裁剪使能都为0，width 和 height 为原始大小即可，输出大小 output_size = 1920×1080 。

> ```c
> static struct frame_image_format output_fmt = {
>     .width              = 1920,
>     .height             = 1080,
>     .pixel_format       = CAMERA_PIX_FMT_NV12,
>
>     .scaler.enable      = 0,
>     .scaler.width       = 1920,
>     .scaler.height      = 1080,
>
>     .crop.enable        = 0,
>     .crop.top           = 0,
>     .crop.left          = 0,
>     .crop.width         = 1920,
>     .crop.height        = 1080,
>
>     .frame_nums         = 2,
> };
> ```

（2） 缩放使能  （scaler.enable = 1 ；crop.enable = 0）

在 sensor 原始分辨率1920×1080的基础上缩小到1280 × 720，开启缩放使能，width 和 height 为输出大小， output_size = 1280×720。

 sensor(1920×1080) --> scaler(1280×720)：

> ```c
> static struct frame_image_format output_fmt = {
>     .width              = 1280,
>     .height             = 720,
>     .pixel_format       = CAMERA_PIX_FMT_NV12,
>
>     .scaler.enable      = 1,
>     .scaler.width       = 1280,
>     .scaler.height      = 720,
>   ......
> };
> ```

（3） 裁剪使能 （scaler.enable = 0；crop.enable = 1）

在 sensor 原始分辨率1920×1080的基础上裁剪为640×480，开启裁剪使能。crop.top 和 crop.left 是裁剪时左上角起始位置，crop.width和 crop.height是裁剪的宽高尺寸。 width 和 height 为输出大小， output_size =  640×480。

 sensor(1920×1080) -->  crop(640×480)：

> ```c
> static struct frame_image_format output_fmt = {
>     .width              = 640,
>     .height             = 480,
>     .pixel_format       = CAMERA_PIX_FMT_NV12,
>   ......
>     .crop.enable        = 1,
>     .crop.top           = 100,    //裁剪左上角位置（100, 180）
>     .crop.left          = 180,
>     .crop.width         = 640,    //裁剪大小（640×480）
>     .crop.height        = 480,
>   ......
> };
> ```

（4）缩放+裁剪 （scaler.enable = 1 ；crop.enable = 1）

缩放、裁剪同时开启时，mscaler **先缩放再裁剪**。

在 sensor 原始分辨率1920×1080的基础上先缩小到1280 × 720，再裁剪到 640×480，开启缩放、裁剪使能，width 和 height 为裁剪后的大小， output_size =  640×480。

sensor(1920×1080) --> scaler(1280×720) --> crop(640×480) ：

> ```c
> static struct frame_image_format output_fmt = {
>     .width              = 640,
>     .height             = 480,
>     .pixel_format       = CAMERA_PIX_FMT_NV12,
>
>     .scaler.enable      = 1,
>     .scaler.width       = 1280,
>     .scaler.height      = 720,
>
>     .crop.enable        = 1,
>     .crop.top           = 100,
>     .crop.left          = 180,
>     .crop.width         = 640,
>     .crop.height        = 480,
>  ......
> };
> ```

图3-1-1展示了图像先缩放后裁剪的处理过程。

![12](img/12.png)
<center style="color:#808080;text-decoration">图3-1-1 缩放后裁剪示意图</center>



#### 3.1.4  isp 图像缩放算法结构体

##### 1. 结构体定义

 mscaler_algo_attr 结构体的定义位于：*/libisp/include/libisp/isp.h*

```c
struct mscaler_algo_attr {
    enum mscaler_algo_mode mode;        // 插值算法
    struct mscaler_algo_coef avg_coef;  // 均值插值系数
};
```

其中插值算法和均值插值系数：

```c
enum mscaler_algo_mode {                // 插值算法
    MSCALER_ALGO_AVG,                   // 均值插值
    MSCALER_ALGO_BICUBIC,               // 双三次插值
};
```

```c
struct mscaler_algo_coef {              // 均值插值系数
    short pw1, pw2, pw3, pw4;           // 四个像素的权重（和为512）
};
```

注：一般放大和轻微缩小双三次插值效果好，缩小较多均值插值效果好。

##### 2. 用途

当缩放使能打开，**默认使用均值插值算法**（由模块驱动实现）；也可指定双三次插值，在摄像头 stream_on 后调用 isp scale 接口函数 :

```c
int isp_set_scale_algo(int fd, struct mscaler_algo_attr *attr);
```



### 3.2. isp接口函数

*libisp/src/lib* 下的 isp.c 定义了多个接口函数。

- 设备开、关

- isp 输出信息

- 获取 sensor 信息

- buffer 申请、映射

- camera 使能、开流

- isp 图像帧处理

- sensor 寄存器

- 设置 scale 算法

- isp 输出图像大小

  ……

具体使用请查看 isp.c 及其头⽂件的函数注释。

#### 3.2.1 设备开、关

| 函数定义                                        | 函数说明             |
| ----------------------------------------------- | -------------------- |
| int isp_open(const char* device_path)           | 获取 camera 设备句柄 |
| int isp_close(int fd, struct camera_info *info) | 关闭 camera 设备句柄 |

#### 3.2.2 isp 输出信息

| 函数定义                                                   | 函数说明                                       |
| ---------------------------------------------------------- | ---------------------------------------------- |
| int isp_set_format(int fd, struct frame_image_format *fmt) | 设置输出信息、分辨率、buffer 个数等            |
| int isp_get_format(int fd, struct frame_image_format *fmt) | 获取输出信息,，frame_size (未 PageSize 对齐)等 |

#### 3.2.3 获取 sensor 信息

| 函数定义                                                  | 函数说明                                         |
| --------------------------------------------------------- | ------------------------------------------------ |
| int isp_get_info(int fd, struct camera_info *info)        | 获得输出 camera 经过 ISP、Mscaler 的 sensor 信息 |
| int isp_get_sensor_info(int fd, struct camera_info *info) | 获得输出 camera 未经过 ISP 的原始 sensor 信息    |

#### 3.2.4 buffer 申请、映射

| 函数定义                                                     | 函数说明                 |
| ------------------------------------------------------------ | ------------------------ |
| int isp_requset_buffer(int fd, struct frame_image_format *fmt) | 申请帧 buffer            |
| int isp_free_buffer(int fd)                                  | 释放帧 buffer            |
| int isp_mmap(int fd, struct camera_info *info)               | 获得输出 camera 映射地址 |

#### 3.2.5 camera 使能、开流

| 函数定义                        | 函数说明               |
| ------------------------------- | ---------------------- |
| **int isp_power_on(int fd)**    | 使能 camera 设备, 上电 |
| **int isp_power_off(int fd)**   | 关闭 camera 设备, 掉电 |
| **int isp_stream_on(int fd)**   | 开始 camera 图像录制   |
| **int isp_stream_off(int fd) ** | 结束 camera 图像录制   |

#### 3.2.6 isp 图像帧处理

| 函数定义                                                 | 函数说明                                             |
| -------------------------------------------------------- | ---------------------------------------------------- |
| **void *isp_wait_frame(int fd)**                         | **获取一帧录制的图像数据(未获取有效数据继续等待3s)** |
| **void *isp_get_frame(int fd)**                          | **获取一帧录制的图像数据(不等待)**                   |
| **int isp_put_frame(int fd, void *mem)**                 | **释放一帧图像缓冲区**                               |
| **int isp_dqbuf_wait(int fd, struct frame_info *frame)** | **获取一帧录制的图像数据(未获取有效数据继续等待3s)** |
| **int isp_dqbuf(int fd, struct frame_info *frame)**      | **获取一帧录制的图像数据(不等待)**                   |
| **int isp_qbuf(int fd, struct frame_info *frame)**       | **释放一帧图像**                                     |
| int isp_get_avaliable_frame_count(int fd)                | 获取已录制的图像数据帧的数目                         |
| int isp_drop_frames(int fd, unsigned int frames)         | 在驱动中丢弃已缓存的图像数据帧                       |

其中，获取图像帧（等待）的接口函数有 isp_wait_frame、isp_dqbuf_wait；获取图像帧（不等待）的接口函数有 isp_get_frame、isp_dqbuf；释放图像帧的的接口函数有 isp_put_frame、isp_qbuf，它们两两之间**功能相似、用法不一致**。

下方表格列出了它们在使用上的差异，**有获取操作就要有对应的释放操作，获取和释放的指针类型要一致**。

| 获取/释放图像帧接口函数（等待）          | 区别                                                      |
| ---------------------------------------- | --------------------------------------------------------- |
| *isp_wait_frame( )<br />isp_put_frame( ) | 获取**图像缓冲区指针**<br />释放**图像缓冲区指针**        |
| isp_dqbuf_wait( )<br />isp_qbuf( )       | 获取**帧信息指针 *frame** <br />释放**帧信息指针 *frame** |

| 获取/释放图像帧接口函数（不等待）       | 区别                                                      |
| --------------------------------------- | --------------------------------------------------------- |
| *isp_get_frame( )<br />isp_put_frame( ) | 获取**图像缓冲区指针**<br />释放**图像缓冲区指针**        |
| isp_dqbuf( )<br />isp_qbuf( )           | 获取**帧信息指针 *frame** <br />释放**帧信息指针 *frame** |

总结这些接口函数的区别主要是：

- 要获取的指针是图像缓冲区指针 *mem 还是图像帧信息指针 *frame；
- 是否需要等待（wait）。

图像帧的操作相关接口函数使用亦可通过图3-2-1所示内容理解：

![13](img/13.png)
<center style="color:#808080;text-decoration">图3-2-1 isp 图像帧处理接口函数使用流程</center>

#### 3.2.7 sensor 寄存器

| 函数定义                                                     | 函数说明           |
| ------------------------------------------------------------ | ------------------ |
| int isp_get_sensor_reg(int fd, struct sensor_dbg_register *reg) | 获取 sensor 寄存器 |
| int isp_set_sensor_reg(int fd, struct sensor_dbg_register *reg) | 设置 sensor 寄存器 |

#### 3.2.8 设置 scale 算法

| 函数定义                                                     | 函数说明              |
| ------------------------------------------------------------ | --------------------- |
| int isp_set_scale_algo(int fd, struct mscaler_algo_attr *attr) | 设置 mscaler 缩放参数 |

#### 3.2.9 isp 输出图像大小

| 函数定义                                                     | 函数说明                        |
| ------------------------------------------------------------ | ------------------------------- |
| int isp_get_max_scaler_size(int fd, int *width, int *height) | 获得 ISP scaler 的最大尺寸      |
| int isp_get_line_align_size(int fd, int *align_size)         | 获得 ISP 行对齐大小，单位：像素 |



### 3.3 isp接口使用流程

以上 isp 接口结构体、isp 接口函数的基本使用流程如下：

```c
int dev_fd;
struct camera_info info;
struct frame_image_format output_fmt = {
    ......
};

int main()
{
    int ret = 0;

    int fd = isp_open("device_path");
    if (fd == -ENODEV)
        return -ENODEV;

    dev_fd = fd;

    ret = isp_set_format(fd, &output_fmt);
    if (ret < 0) {
        close(fd);
        return ret;
    }

    ret = isp_requset_buffer(fd, &output_fmt);
    if (ret < 0) {
        close(fd);
        return ret;
    }

    isp_get_info(fd, &info);

    ret = isp_mmap(fd, &info);
    if (ret < 0) {
        isp_free_buffer(fd);
        close(fd);
        return ret;
    }

    ret = isp_power_on(fd);
    if (ret)
        goto close_fd;

    ret = isp_stream_on(fd);
    if (ret)
        goto close_fd;

    ......
    /*Add any else API functions you want.*/
    ......

    isp_stream_off(fd);
    isp_power_off(fd);

close_fd:
    isp_free_buffer(fd);
    isp_close(fd, &info);
    dev_fd = -1;

    return ret;
}
```

isp 接口使用流程图如图3-3-1，图中左侧是程序流程图，中间是对应流程所使用到的接口函数，右侧是由isp_open() 函数生成 camera 设备句柄 fd，以及需要作为参数传给接口函数调用的接口结构体。

fd 由所有彩色框内的函数调用，摄像头出图属性结构体 frame_image_format 由绿色框内的函数调用，camera 信息结构体 camera_info 由橙色框内的函数调用。



![14](img/14.png)
<center style="color:#808080;text-decoration">图3-3-1 isp 接口函数、接口结构体使用流程图</center>



## 4 ISP tuning 接口

*libisp/src/lib* 下的 **isp_tuning.c 定义了多个isp效果调试模块**接口结构体和接口函数，支持用户对摄像头的某些效果（曝光、白平衡、亮度、饱和度等）进行调试。

### 4.1 isp tuning 接口结构体

libisp 中主要的 isp tuning 接口结构体的定义位于/libisp/include/isp_tuning.h，下表简单介绍了 isp tuning 结构体的功能。

<center style="color:#050505;text-decoration">表4-1-1 libisp 中主要的 isp tuning 接口结构体</center>

| 结构体/共用体                                  | 说明                                                         |
| ---------------------------------------------- | ------------------------------------------------------------ |
| isp EV 参数结构体<br />struct  isp_ev_attr     | 存放 isp EV 参数信息，包括曝光模式（自动或手动）、曝光值、<br />曝光时间（当前、最大、最小）、模拟增益、数字增益、总增益。 |
| 曝光参数结构体<br />struct  isp_core_expr_attr | 设置曝光模式、曝光时间、模拟增益                             |
| isp 权重信息结构体<br />struct  isp_weight     | 设置 15 × 15 各区域 AE 权重                                  |
| 白平衡参数结构体<br />struct  isp_core_wb_attr | 设置白平衡模式（自动、手动）；<br />在手动模式下，可以设置白平衡红色增益、蓝色增益。 |
| gamma 参数结构体<br />struct  isp_gamma        | gamma 参数数组，有129个点构成 gamma 曲线                     |

以上结构体的具体定义请查看 isp_tuning.c 及其头文件相关注释。



### 4.2 isp tuning 接口函数

*libisp/src/lib* 下的 isp_tuning.c 定义了多个 isp 效果调试模块接口函数。

- tuning 设备开关

- isp 工作模式（正常、夜视）

- 图像翻转（水平、垂直）

- 帧率设置

- 亮度、对比度、饱和度、锐度调节

- 抗频闪

- 设置曝光参数

- 强光抑制

- 白平衡

- gamma

- ADR强度

  ……

函数具体的声明、定义请查看 **isp_tuning.c** 及其头⽂件 isp_tuning.h。



### 4.3 isp tuning 接口使用

> #####  ISP tuning 接口使用前提：
>
> 1、sensor 经过 isp；2、isp 效果参数已经确定。
>
> 注：效果参数文件由 isp 驱动加载，该文件的路径（板子）：/etc/sensor/xxx-x2000.bin。不同的 sensor 模组需要专门调试出属于自身的效果参数文件，若 sensor 没有效果参数文件，则使用默认的效果参数，出图效果较差。
>

#### 4.3.1 代码修改

isp tuning 接口在 demo_isp.c 文件中通过宏定义的方式灵活选择调用或不调用。

这些 **tuning 接口的宏默认状态是不开启**，如需使用 tuning 接口，可自行修改 demo_isp.c 中接口宏定义取值（0或1），1表示开启，0表示关闭。最后编译 libisp 进板子后方可使用 **demo_isp  shell 命令**查看结果。

```c
#define TEST_ISP_TUNING_RUNNING_MODE         0  //运行模式（day or night）
#define TEST_ISP_TUNING_HFLIP                0  //水平翻转
#define TEST_ISP_TUNING_VFLIP                0  //垂直翻转
#define TEST_ISP_TUNING_SENSOR_FPS           0  //设置帧率
#define TEST_ISP_TUNING_BRIGHTNESS           0  //亮度
#define TEST_ISP_TUNING_CONTRAST             0  //对比度
#define TEST_ISP_TUNING_SATURATION           0  //饱和度
#define TEST_ISP_TUNING_SHARPNESS            0  //锐度
#define TEST_ISP_TUNING_ANTI_FLICKER_ATTR    0  //抗频闪
#define TEST_ISP_TUNING_EV_ATTR              0  //EV参数
#define TEST_ISP_TUNING_EXPR                 0  //曝光参数
#define TEST_ISP_TUNING_MAX_AGAIN            0  //最大模拟增益
#define TEST_ISP_TUNING_AE_MIN               0  //AE最小参数
#define TEST_ISP_TUNING_AE_LUMA              0  //AE亮度参数
#define TEST_ISP_TUNING_HI_LIGHT_DEPRESS     0  //强光抑制
#define TEST_ISP_TUNING_AE_WEIGHT            0  //AE区域权重
#define TEST_ISP_TUNING_WB                   0  //白平衡
#define TEST_ISP_TUNING_GAMMA                0  //gamma
#define TEST_ISP_TUNING_ADR_STRENGTH         0  //ADR强度
```

当开启某个模块功能时，**先把 TEST_ISP_TUNING_xxx 设为1**，再到宏对应模块修改参数。

例如：当需要调用亮度调试接口 TEST_ISP_TUNING_BRIGHTNESS 时，按下方步骤操作。

1) 接口设为1

```c
#define TEST_ISP_TUNING_BRIGHTNESS           1  //调用亮度接口
```

2. 设置目标亮度值

   亮度默认值为128，大于128增加亮度，小于128降低亮度。

```c
#if TEST_ISP_TUNING_BRIGHTNESS
    unsigned char brightness;
    isp_tuning_set_brightness(fd_tuning, 130);          //设置亮度值
    isp_tuning_get_brightness(fd_tuning, &brightness);  //获取亮度值
    fprintf(stderr, "----Get_brightness, brightness = %u\n\n", brightness);
#endif
```

#### 4.3.2 串口调试

代码改了以后：**保存，编译 libisp，编译文件系统 buildroot，单独烧录文件系统到板子。**

串口 shell 命令示例：

```
demo_isp /dev/mscaler0-ch0
```

回车后打印结果：

```
...... (sensor 信息)
Open isp tuning device isp-tuning0
----Get_brightness, brightness = 130

......(Save Frame to File(/tmp/frame.nv12))
```

注：isp tuning 设备节点名称与 demo_isp 命令打开的的 mscaler 设备节点有关，**mscaler0 （所有channel） 对应 isp-tuning0；mscaler1 （所有channel） 对应 isp-tuning1。**

获取的亮度值与设定的一致，可拉图片到电脑端查看（参考 2.3.2 内容）效 果。
