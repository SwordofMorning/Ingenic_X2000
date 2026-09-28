# X2000 ROTATOR硬件旋转

## 1 功能介绍

​	libhardware2中的rotator旋转是调用硬件控制器驱动提供的ioctl接口来对指定物理内存空间进行旋转。

​	libhardware2中勾选rotator接口即可在程序中调用rotator函数，它对ioctl接口进行基本的封装，并通过相关config即可设置旋转参数，如传入的待操作的内存空间的物理地址、内存对齐、像素格式、旋转角度以及翻转等参数。

​	为方便用户使用与测试，勾选rotator命令将提供在命令行中可执行的cmd_rotator命令，其功能为：将输入文件旋转并保存，同时显示在屏幕上（需要输入的文件色彩格式与屏幕支持的色彩格式相匹配） 以及 屏幕内存区颜色填充后旋转并显示。

​	该文档基于芯片平台为：x2000 ，kernel支持的版本为：4.4 或者 5.10。下面以开发板 Darwin_X2000_V2.0 为例进行说明。

## 2 软件配置

使用IConfigTool 选择主配置 x2000_darwin_factory_defconfig

![0](x2000_rotator使用说明.assets/0.png)



在 libhardware2下选择 rotator 

![1](x2000_rotator使用说明.assets/1.png)

勾选 rotator 后，将编译`libhardware2/src/lib/rotator/rotator.c`，会将`int rotator_complete_conversion(int handle, struct rotator_config_data *data)`这个应用层api编入libhardware.so库，用户编程时包含头文件即可：`#include <libhardware2/rotator.h>`



勾选 cmd_rotator 命令，将编译`libhardware2/src/cmds/rotator_main.c`，用户即可在命令行使用 cmd_rotator 命令

![2](x2000_rotator使用说明.assets/2.png)

调用 cmd_rotator 进行硬件旋转测试，会用到 rmem、fb 这两个接口。 确保 fb 、rmem 以及 rotator 相关驱动加载正常。

勾选 fb 

![2023-08-09_11-54](x2000_rotator使用说明.assets/3.png)



勾选 rmem

![4](x2000_rotator使用说明.assets/4.png)



## 3  API接口介绍

cmd_rotator 命令源码路径：ls libhardware2/src/lib/rotator/rotator.c

头文件： libhardware2/include/libhardware2/rotator.h



头文件介绍如下：

```c
// rotator配置结构体
struct rotator_config_data {
    void *src_buf;    // 目标转换数据的物理地址，可以用rmem获取，并将数据手动拷贝
    void *dst_buf;    // 转换后的物理地址

    unsigned int frame_height;    // 目标数据的高
    unsigned int frame_width;     // 目标数据的宽

    unsigned int src_stride;      // 目标内存裁剪（目标内存图片的一行的像素数）
    unsigned int dst_stride;      // 转换后的内存裁剪（裁剪后输入到内存位置的一行的像素数）

    enum rotator_fmt src_fmt;     // 目标像素格式
    enum rotator_convert_order convert_color;   // RGB顺序转换
    enum rotator_fmt dst_fmt;     // 输出像素格式

    enum rotator_mirror horizontal_mirror;    // 水平翻转（镜像）
    enum rotator_mirror vertical_mirror;      // 垂直反转（镜像）
    enum rotator_angle rotate_angle;          // 旋转角度
};

// 将rotator作为设备打开，返回设备描述符
int rotator_open(void);

/**
 * @brief 根据传入的rotator_config_data，完成旋转
 * @param handle rotator设备描述符
 * @return 无返回值
 */
int rotator_complete_conversion(int handle, struct rotator_config_data *data);

void rotator_close(int handle);
```



## 4 命令测试

### 4.1  像素图片文件旋转

```c
cmd_rotator input_file=/tmp/logo_495_280_argb8888 output_file=/tmp/logo_495_280_argb8888_270 image_width=495 image_height=280 rotate_degree=270 preview=/dev/fb0
```

命令解释：指定输入文件`/tmp/logo_495_280_argb8888`，输出文件`/tmp/logo_495_280_argb8888_270`，输入图片宽495，高280、旋转角度270度，并默认显示在 fb0 上或preview指定的设备`/dev/fb0`。

*测试时，logo_495_280_argb8888 文件可以通过adb push 到设备的/tmp/目录下。*



```shell
# 查看命令使用方式
cmd_rotator -h/--help/help

# 参数说明
cmd_rotator <input_file=path> <output_file=path> [src_fmt=fmt] <image_width=value> <image_height=value> <rotate_degree=degree> [color_order=order] [mirror=mirror] [preview=device]

# 参数说明，其中< >为必填项，[ ]为选填项
# <input_file>        The pixel data file for rotate.
# <output_file>       The pixel data file after rotate.
# [src_fmt]           Pixels format:
#                     ARGB8888(default)
#                     RGB565
#                     RGB555
#                     YUV422
#                     Y8
# <image_width>       The background image weight(unit:pixel).
# <image_height>      The background image height(unit:pixel).
# <rotate_degree>     Rotate degree: 0 / 90 / 180 / 270.
# [color_order]       Input pixels' order(??? -> RGB): 
#                     RGB(default) RBG / GRB / GBR / BRG / BGR.
# [mirror]            Mirror convert: horizontal / vertical / both.
#                     (default: not set).
# [device]            Preview on fb device: /dev/fb0 
#                     (default: /dev/fb0)
```



测试原图如下

argb8888 格式， `/tmp/logo_495_280_argb8888`文件：（argb文件通过 YUView 软件进行查看）

![5](x2000_rotator使用说明.assets/5.png)

rgb565 格式， `/tmp/logo_495_280_rgb565`文件：（rgb565 文件通过 7yuv 软件查看）

![6](x2000_rotator使用说明.assets/6.png)



#### 4.1.1  argb8888 格式进行顺时针270°旋转

命令如下：

```shell
cmd_rotator input_file=/tmp/logo_495_280_argb8888 output_file=/tmp/rotate/logo_495_280_argb8888_270 image_width=495 image_height=280 rotate_degree=270
```

效果图如下：（使用 YUView 软件查看）

![7](x2000_rotator使用说明.assets/7.png)



#### 4.1.2  argb8888图片进行水平翻转

命令如下：

```shell
cmd_rotator input_file=/tmp/logo_495_280_argb8888 output_file=/tmp/logo_495_280_argb8888_mirror_h image_width=495 image_height=280 mirror=horizontal
```

效果图如下：（使用 YUView 软件查看）

![image-20230809154953529](x2000_rotator使用说明.assets/8.png)



#### 4.1.3  argb8888图片进行垂直翻转

命令如下：

```shell
cmd_rotator input_file=/tmp/logo_495_280_argb8888 output_file=/tmp/logo_495_280_argb8888_mirror_v image_width=495 image_height=280 mirror=vertical
```

效果图如下：（使用 YUView 软件查看）

![image-20230809155017688](x2000_rotator使用说明.assets/9.png)



#### 4.1.4  argb8888图片进行水平 + 垂直翻转

命令如下：

```shell
cmd_rotator input_file=/tmp/logo_495_280_argb8888 output_file=/tmp/logo_495_280_argb8888_mirror_hv image_width=495 image_height=280 mirror=both
```

效果图如下：（使用 YUView 软件查看）

![image-20230809155146367](x2000_rotator使用说明.assets/10.png)



#### 4.1.5  rgb565 颜色排序BGR格式转换为颜色排序RGB

命令如下：

```shell
cmd_rotator input_file=/tmp/logo_495_280_rgb565 output_file=/tmp/logo_495_280_rgb565_bgr2rgb src_fmt=RGB565 image_width=495 image_height=280 color_order=BGR
```

效果图如下：（使用 7yuv 软件查看）

![9](x2000_rotator使用说明.assets/11.png)



### 4.2  无输入文件屏幕颜色填充旋转

命令如下：

```shell
cmd_rotator /dev/fb0
```

命令解释：命令默认向 fb0 中填充三种颜色 R G B，再调用 rotator 接口进行旋转90，并显示在`/dev/fb0`。

效果图如下：

![10](x2000_rotator使用说明.assets/12.png)