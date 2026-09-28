# USB_UVC

> 注意：使用uvc工具时，需要打开USB的DMA功能
>
> 使用x2000 nand或nor为存储介质时文件路径（arch/mips/boot/dts/ingenic/x2000_module_base.dts）
>
> 使用x2000 mmc为存储介质时文件路径（arch/mips/boot/dts/ingenic/x2000_module_base_mmc0.dts）
>
> ![](img/1.png)

## 1. menuconfig 配置流程

```c
Device Drivers  --->
    <*> Multimedia support  --->
            [*]   Cameras/video grabbers support

Device Drivers  --->
     [*] USB support  --->
            <*>   USB Gadget Support  --->
                  <*>   USB Gadget Drivers
                           (USB functions configurable through configfs)  --->
                  [*]       USB Webcam function
```

## 2. 预览彩条使用流程

### 2.1 切换到uvc应用层目录

> cd doc/开发使用说明/USB使用说明文档/设备/USB_UVC/xburst2/
>

### 2.2 编译应用程序

> ../../../../../../buildroot/buildroot/output/host/usr/bin/mips-linux-gnu-gcc uvc-color-bar.c uvc_lib.c -o uvc-color-bar
>

### 2.3 将配置脚本和生成的可执行文件adb到可写的/usr/data目录或者/tmp目录下

> adb push uvc-device-config.sh uvc-color-bar  /usr/data
>

### 2.4 uvc-color-bar 参数介绍

> video_device=/dev/videoX 运行脚本生成的设备节点
>
> package_size=value 包大小(8~3072)
>
> io_method=mmap|userptr 申请缓冲区⽅式
>
> nbufs=value 申请缓冲区数量
>
> format=grey,width,height,fps 预览格式

### 2.5 UVC使用流程

#### 2.5.1 单路UVC使用流程

1.关闭adb功能

> /etc/init.d/S90adb stop
>

![](img/2.png)

2.配置uvc参数

> Usage :  ./uvc-device-config.sh start <uvc-ch0:fcc1,width1,height1,fps1/fcc2,width2,height2,fps2/package_size=value>

 ![](img/3.png)

```c
./uvc-device-config.sh start uvc-ch0:format=nv12,1280,720,10/format=yuyv,640,360,10/package_size=1024
运行uvc配置脚本后生成/dev/video12设备节点
```

3.运行uvc预览应用

> Usage : ./uvc-color-bar <video_device=/dev/videoX> <format=grey,width,height,fps> [package_size=value] [io_method=mmap|userptr] [nbufs=value]

```c
预览单路uvc彩条运行命令
./uvc-color-bar video_device=/dev/videoX format=nv12,1280,720,10 format=yuyv,640,360,10 package_size=1024 io_method=mmap nbufs=3

特别注意以下事项：
1./dev/videoX设备节点一定是运行uvc-device-config.sh脚本后生成的设备节点/dev/video12。
2.uvc-device-config.sh配置uvc支持的格式，应用也只能预览uvc支持的格式。
3.uvc传输包的大小范围是8～3072bit,包越大传输数度越快。
4.uvc预览支持NV12、NV21、yuyv、grey、MJPG格式。
```

#### 2.5.2 多路UVC使用流程

1.关闭adb功能

/etc/init.d/S90adb stop

![](img/2.png)

2.配置uvc参数

> Usage :  ./uvc-device-config.sh start <uvc-ch0:fcc1,width1,height1,fps1/fcc2,width2,height2,fps2/package_size=value vc-ch1:fcc1,width1,height1,fps1/fcc2,width2,height2,fps2/package_size=value

![](img/4.png)

```c
./uvc-device-config.sh start uvc-ch0:format=nv12,1280,720,10/format=yuyv,640,360,10/package_size=1024  uvc-ch1:format=grey,1280,720,10/format=nv12,640,360,10/package_size=2048
运行uvc配置脚本后生成/dev/video12 和 /dev/video13设备节点
```

3.运行uvc预览应用

> Usage : ./uvc-color-bar <video_device=/dev/videoX> <format=grey,width,height,fps> [package_size=value] [io_method=mmap|userptr] [nbufs=value]

```c
预览多路uvc彩条命令

UVC通道1
./uvc-color-bar video_device=/dev/videoX format=nv12,1280,720,10 format=yuyv,640,360,10 package_size=1024 io_method=mmap nbufs=3 &

UVC通道2
./uvc-color-bar video_device=/dev/videoY format=grey,1280,720,10 format=nv12,640,360,10 package_size=2048 io_method=mmap nbufs=3 &

特别注意以下事项：
1./dev/videoX /dev/videoY设备节点一定是运行uvc-device-config.sh脚本后生成的设备节点/dev/video12 和 /dev/video13。
2.uvc-device-config.sh配置uvc对应通道支持的格式，应用也只能预览uvc对应通道支持的格式。
3.uvc传输包的大小范围是8～3072bit,包越大传输数度越快。
4.uvc预览支持NV12、NV21、yuyv、grey、MJPG格式。
5.多路uvc预览时传输包大小总和尽量不要超过4096bit。
```

## 3. 不过ISP的摄像头使用流程

### 3.1 切换到uvc应用层目录

> cd doc/开发使用说明/USB使用说明文档/设备/USB_UVC/xburst2/
>

### 3.2 编译应用程序

> ../../../../../../buildroot/buildroot/output/host/usr/bin/mips-linux-gnu-gcc uvc-camera.c uvc_lib.c -lhardware2 -o uvc-camera
>

### 3.3 将生成的可执行文件adb到可写的/usr/data目录或者/tmp目录下

> adb push  uvc-camera uvc-device-config.sh /usr/data
>

### 3.4 uvc-camera 参数介绍

> video_device=/dev/videoX 运行脚本生成的设备节点
>
> camera_device_path camera设备节点
>
> package_size=value 包大小(8~3072)
>
> io_method=mmap|userptr 申请缓冲区⽅式
>
> nbufs=value 申请缓冲区数量
>
> format=grey,width,height,fps 预览格式

### 3.5 UVC使用流程

#### 3.5.1 单路uvc使用流程

1.关闭adb功能

> /etc/init.d/S90adb stop
>

![](img/2.png)

2.配置uvc参数

> Usage :  ./uvc-device-config.sh start <uvc-ch0:fcc1,width1,height1,fps1/package_size=value>

![](img/5.png)

```c
./uvc-device-config.sh start uvc-ch0:format=yuyv,1920,1080,10/package_size=2048 
运行uvc配置脚本后生成/dev/video12设备节点
```

3.运行uvc预览应用

> Usage : ./uvc-camera <camera_device_path>  <video_device=/dev/videoX> <format=grey,width,height,fps> [package_size=value] [io_method=mmap|userptr] [nbufs=value]

```c
预览单路uvc摄像头运行命令
./uvc-camera /dev/vic0 video_device=/dev/videoX  format=yuyv,1920,1080,10 package_size=2048 io_method=mmap nbufs=3

特别注意以下事项：
1./dev/videoX设备节点一定是运行uvc-device-config.sh脚本后生成的设备节点/dev/video12。
2./dev/vic0是预览摄像头产生的设备节点。
3.uvc-device-config.sh配置uvc支持的格式，应用也只能预览uvc支持的格式。
4.uvc传输包的大小范围是8～3072bit,包越大传输数度越快。
5.uvc预览支持NV12、NV21、yuyv、grey、MJPG格式。
```

#### 3.5.2 多路UVC使用流程

1.关闭adb功能

> /etc/init.d/S90adb stop
>

2.配置uvc参数

> Usage :  ./uvc-device-config.sh start <uvc-ch0:fcc1,width1,height1,fps1/package_size=value> <uvc-ch1:fcc1,width1,height1,fps1/package_size=value>

![](img/6.png)

```c
./uvc-device-config.sh start uvc-ch0:format=yuyv,1920,1080,10/package_size=2048  uvc-ch1:format=yuyv,1920,1080,10/package_size=2048
运行uvc配置脚本后生成/dev/video12 和 /dev/video13设备节点
```

3.运行uvc预览应用

> Usage : ./uvc-camera <camera_device_path>  <video_device=/dev/videoX> <format=grey,width,height,fps> [package_size=value] [io_method=mmap|userptr] [nbufs=value]

```c
预览多路uvc摄像头命令

UVC通道0
./uvc-camera  /dev/vic0 video_device=/dev/videoX  format=yuyv,1920,1080,10 package_size=2048 io_method=mmap nbufs=3 &

UVC通道1
./uvc-camera  /dev/vic1  video_device=/dev/videoY format=yuyv,1920,1080,10 package_size=2048 io_method=mmap nbufs=3 &

特别注意以下事项：
1./dev/videoX /dev/videoY设备节点一定是运行uvc-device-config.sh脚本后生成的设备节点/dev/video12 和 /dev/video13。
2. /dev/vic0 和  /dev/vic1 是摄像头产生的设备节点。
3.uvc-device-config.sh配置uvc对应通道支持的格式，应用也只能预览uvc对应通道支持的格式。
4.uvc传输包的大小范围是8～3072bit,包越大传输数度越快。
5.uvc预览支持NV12、NV21、yuyv、grey、MJPG格式。
6.多路uvc预览时传输包大小总和尽量不要超过4096bit。
```

## 4. 过ISP的摄像头使用流程



### 4.1 切换到uvc应用层目录

> cd doc/开发使用说明/USB使用说明文档/设备/USB_UVC/xburst2/
>

### 4.2 编译应用程序

> ../../../../../../buildroot/buildroot/output/host/usr/bin/mips-linux-gnu-gcc uvc-isp.c uvc_lib.c -lisp -lhardware2 -o uvc-isp
>

### 4.3 将生成的可执行文件adb到可写的/usr/data目录或者/tmp目录下

> adb push  uvc-device-config.sh uvc-isp /usr/data
>

### 4.4 uvc-isp 参数介绍

> video_device=/dev/videoX 运行脚本生成的设备节点
>
> isp_device_path isp设备节点
>
> package_size=value 包大小(8~3072)
>
> io_method=mmap|userptr 申请缓冲区⽅式
>
> nbufs=value 申请缓冲区数量
>
> format=grey,width,height,fps 预览格式

### 4.5 UVC使用流程

#### 4.5.1 单路uvc使用流程

1.关闭adb功能

> /etc/init.d/S90adb stop

![](img/2.png)

2.配置uvc参数

> Usage :  ./uvc-device-config.sh start <uvc-ch0:fcc1,width1,height1,fps1/package_size=value>

![](img/7.png)

```c
./uvc-device-config.sh start uvc-ch0:format=mjpg,1280,720,10/package_size=1024
运行uvc配置脚本后生成/dev/video12设备节点
```

3.运行uvc预览应用

> Usage : ./uvc-isp <isp_device_path> <video_device=/dev/videoX> <format=grey,width,height,fps> [package_size=value] [io_method=mmap|userptr] [nbufs=value]

```c
预览单路uvc摄像头运行命令
./uvc-isp  /dev/mscaler0-ch0 video_device=/dev/videoX format=mjpg,1280,720,10 package_size=1024

特别注意以下事项：
1./dev/videoX设备节点一定是运行uvc-device-config.sh脚本后生成的设备节点/dev/video12。
2./dev/mscaler0-ch0 是预览摄像头产生的设备节点。
3.uvc-device-config.sh配置uvc支持的格式，应用也只能预览uvc支持的格式。
4.uvc传输包的大小范围是8～3072bit,包越大传输数度越快。
5.uvc预览支持NV12、NV21、yuyv、grey、MJPG格式。
```

#### 4.5.2 多路uvc使用流程

1.关闭adb功能

> /etc/init.d/S90adb stop

![](img/2.png)

2.配置uvc参数

> Usage :  ./uvc-device-config.sh start <uvc-ch0:fcc1,width1,height1,fps1/package_size=value> <uvc-ch1:fcc1,width1,height1,fps1/package_size=value>

![](img/8.png)

```c
./uvc-device-config.sh start uvc-ch0:format=nv12,640,360,10/package_size=2048  uvc-ch1:format=mjpg,1280,720,10/package_size=1024
运行uvc配置脚本后生成/dev/video12 和 /dev/video13设备节点
```

3.运行uvc预览应用

> Usage :  ./uvc-isp start <isp_device_path> <video_device=/dev/videoX> uvc-ch0:fcc1,width1,height1,fps1/package_size=value uvc-ch1:fcc1,width1,height1,fps1/package_size=value

```c
预览多路uvc摄像头命令

UVC通道0
./uvc-isp /dev/mscaler0-ch0 video_device=/dev/videoX  format=nv12,640,360,10 package_size=2048 &

UVC通道1
./uvc-isp /dev/mscaler1-ch0 video_device=/dev/videoY  format=mjpg,1280,720,10 package_size=1024 &

特别注意以下事项：
1./dev/videoX /dev/videoY设备节点一定是运行uvc-device-config.sh脚本后生成的设备节点/dev/video12 和 /dev/video13。
2. /dev/mscaler0-ch0 和  /dev/mscaler1-ch0 是摄像头产生的设备节点。
3.uvc-device-config.sh配置uvc对应通道支持的格式，应用也只能预览uvc对应通道支持的格式。
4.uvc传输包的大小范围是8～3072bit,包越大传输数度越快。
5.uvc预览支持NV12、NV21、yuyv、grey、MJPG格式。
6.多路uvc预览时传输包大小总和尽量不要超过4096bit。
```

## 5. uvc 预览使用常见问题

问题1：挂载usb设备失败 Error: usb configfs already mounted

答：需要先卸载其他usb设备，确保/sys/kernel/config/目录没有文件被挂载



问题2：创建uvc.0功能目录失败 mkdir: can't create directory 'functions/uvc.0': No such file or directory

答：kernel的 USB Webcam function功能没有配置，需要重新配置编译
