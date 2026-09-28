# USB_UVC

## 1. menuconfig 配置流程

```c
Device Drivers  --->
    <*> Multimedia support  --->
            [*]   Cameras/video grabbers support

Device Drivers  --->
     [*] USB support  --->
            <*>   USB Gadget Support  --->
                  <*>   USB Gadget Drivers
                         <*>   USB Gadget Drivers (USB Webcam Gadget)  --->
```

## 2. 修改uvc描述符

修改的文件路径：drivers/usb/gadget/webcam.c 或者 drivers/usb/gadget/webcam_uac1/webcam_uac1.c 以具体使用文件为准

### 2.1 修改yuyv格式480P分辨率方法

#### 2.1.1 修改uvc支持格式描述符

```c
// DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 2);
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 1); //input描述符定义

/* 仅支持未压缩yuyv一种格式 */
static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 1) uvc_input_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 1, // 仅支持yuyv一种格式
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo			= 0,
	.bTerminalLink		= 3,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	// .bmaControls[1][0]	= 4,
};

```

#### 2.1.2 修改yuyv未压缩格式描述符

```c
static const struct uvc_format_uncompressed uvc_format_yuv = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 1,   // yuyv仅支持480P;
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 1,    // 选择默认帧索引为增加的480p的索引
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};
```

#### 2.1.3 增加480p帧描述符结构体

```c
static const struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuv_480p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 1, // 帧的索引
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640), // 帧的宽
	.wHeight		= cpu_to_le16(480), // 帧的高
	.dwMinBitRate		= cpu_to_le32(73728000), // 宽 * 高 * bpp * 15(fps)
	.dwMaxBitRate		= cpu_to_le32(73728000), // 宽 * 高 * bpp * 15(fps)
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(614400), // 宽 * 高 * bpp / 8
	.dwDefaultFrameInterval	= cpu_to_le32(666666), // 10000000 / 15(fps)
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= cpu_to_le32(666666), //默认使用15fps
};
```

#### 2.1.4 修改uvc流的描述符

```c
/* 高速模式uvc流描述符 */
static const struct uvc_descriptor_header * const uvc_hs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_yuv,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_360p,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_720p,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_480p,  // 增加的480p
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

/* 全速模式uvc流描述符 */
static const struct uvc_descriptor_header * const uvc_fs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_yuv,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_360p,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_720p,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_480p, // 增加的480p
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

/* 低速模式下uvc流的描述符 */
static const struct uvc_descriptor_header * const uvc_ss_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_yuv,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_360p,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_720p,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_480p,// 增加的480p
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};
```

### 2.2 修改nv12格式360P分辨率方法

#### 2.2.1 修改uvc支持格式描述符

```c
// DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 2);
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 1); //input描述符定义

/* 仅支持未压缩nv12一种格式 */
static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 1) uvc_input_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 1, // 仅支持nv12一种格式
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo			= 0,
	.bTerminalLink		= 3,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	// .bmaControls[1][0]	= 4,
};
```

#### 2.2.2 增加nv12未压缩格式描述符

```c
static const struct uvc_format_uncompressed uvc_format_nv12 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 1, // 格式索引
	.bNumFrameDescriptors	= 1,
	.guidFormat		=
		{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};
```

#### 2.2.3 增加360P帧描述符

```c
static const struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_360p = {
    .bLength        = UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
    .bFrameIndex        = 1,
    .bmCapabilities     = 0,
    .wWidth         = cpu_to_le16(640),
    .wHeight        = cpu_to_le16(360),
    .dwMinBitRate       = cpu_to_le32(640 * 360 * 12* 10),
    .dwMaxBitRate       = cpu_to_le32(640 * 360 * 12* 10),
    .dwMaxVideoFrameBufferSize  = cpu_to_le32(640 * 360),
    .dwDefaultFrameInterval = cpu_to_le32(1000000),
    .bFrameIntervalType = 1,
    .dwFrameInterval[0] = cpu_to_le32(1000000),
};
```

#### 2.2.4 修改uvc流描述符

```c
/* 高速模式uvc流描述符 */
static const struct uvc_descriptor_header * const uvc_hs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_nv12,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_360p,
	// (const struct uvc_descriptor_header *) &uvc_format_yuv,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_360p,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_720p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

/* 全速模式uvc流描述符 */
static const struct uvc_descriptor_header * const uvc_fs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_nv12,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_360p,
	// (const struct uvc_descriptor_header *) &uvc_format_yuv,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_360p,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_720p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

/* 低速模式下uvc流的描述符 */
static const struct uvc_descriptor_header * const uvc_ss_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_nv12,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_360p,
	// (const struct uvc_descriptor_header *) &uvc_format_yuv,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_360p,
	// (const struct uvc_descriptor_header *) &uvc_frame_yuv_720p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};
```

## 3. 修改uvc传输包大小

修改的文件路径：drivers/usb/gadget/f_uvc.c

```c
static unsigned int streaming_maxpacket = 1024;
```

## 4. 增加uvc预览通道

修改的文件路径：drivers/usb/gadget/webcam.c

```c
static int __init
webcam_config_bind(struct usb_configuration *c)
{
	/* uvc通道0
	 * uvc_ss_control_cls  uvc预览支持的格式根据需求自行修改
     * uvc_fs_streaming_cls
      * uvc_hs_streaming_cls
    */
	uvc_bind_config(c, uvc_fs_control_cls, uvc_ss_control_cls,
		uvc_fs_streaming_cls, uvc_hs_streaming_cls,
		uvc_ss_streaming_cls);

	/* uvc通道1
	 * uvc_ss_control_cls  uvc预览支持的格式根据需求自行修改
     * uvc_fs_streaming_cls
      * uvc_hs_streaming_cls
    */
	uvc_bind_config(c, uvc_fs_control_cls, uvc_ss_control_cls,
		uvc_fs_streaming_cls, uvc_hs_streaming_cls,
		uvc_ss_streaming_cls);
	return 0;
}
```

## 5. 预览彩条使用流程

### 5.1 切换到uvc应用层目录

```c
cd doc/开发使用说明/USB使用说明文档/设备/USB_UVC/xburst1/
```

### 5.2 编译应用程序

```c
../../../../../../buildroot/buildroot/output/host/usr/bin/mips-linux-gnu-gcc uvc-color-bar.c uvc_lib.c -o uvc-color-bar
```

### 5.3 将生成的可执行文件拷贝到文件系统然后打包到镜像中

```c
cp uvc-color-bar ../../../../../../buildroot/buildroot/output/target/

cd build    #切换到build目录

make   # 编译修改的kernel 和 文件系统
```

### 5.4 uvc-color-bar 参数介绍

> video_device=/dev/videoX v4l2设备节点
>
> format=nv12|yuyv|grey 预览格式
>
> width=value 预览数据的宽
>
> height=value 预览数据的高
>
> fps=value 预览数据的帧率
>
> package_size=value 包大小(8~3072)
>
> io_method=mmap|userptr 申请缓冲区⽅式
>
> nbufs=value 申请缓冲区数量
>

### 5.5 uvc使用流程

#### 5.5.1 单路uvc使用流程

> Usage : ./uvc-color-bar <video_device=/dev/videoX> <format=nv12|yuyv|grey> <width=value> <height=value> <fps=value> <package_size=value> [io_method=mmap|userptr] [nbufs=value]

```c
预览单路uvc彩条运行命令
./uvc-color-bar video_device=/dev/videoX format=yuyv width=640 height=480 fps=15 package_size=1024

特别注意以下事项：
1.package_size的大小需要与修改的uvc传输的包大小streaming_maxpacket一样.
2./dev/videoX videoX节点需要用uvc的video节点替换。
3.预览的格式和分辨率需要与kernel支持的格式和分辨率一致。
```

#### 5.5.2 多路uvc使用流程

> Usage : ./uvc-color-bar <video_device=/dev/videoX> <format=nv12|yuyv|grey> <width=value> <height=value> <fps=value> <package_size=value> [io_method=mmap|userptr] [nbufs=value]

```c
预览单路uvc彩条运行命令

UVC通道0
./uvc-color-bar video_device=/dev/videoX format=yuyv width=640 height=480 fps=15 package_size=1024 &

UVC通道1
./uvc-color-bar video_device=/dev/videoY format=nv12 width=1280 height=720 fps=15 package_size=1024 &

特别注意以下事项：
1.package_size的大小需要与修改的uvc传输的包大小streaming_maxpacket一样.
2./dev/videoX 和 /dev/videoY videoX和videoY节点需要用uvc的video节点替换。
3.预览的格式和分辨率需要与kernel支持的格式和分辨率一致。
```

## 6. 不过ISP的摄像头使用流程

### 6.1 切换到uvc应用层目录

```c
cd doc/开发使用说明/USB使用说明文档/设备/USB_UVC/xburst1/
```

### 6.2 编译应用程序

```c
../../../../../../buildroot/buildroot/output/host/usr/bin/mips-linux-gnu-gcc uvc-camera.c uvc_lib.c -lhardware2 -o uvc-camera
```

### 6.3 将生成的可执行文件拷贝到文件系统然后打包到镜像中

```c
cp uvc-camera ../../../../../../buildroot/buildroot/output/target/

cd build    #切换到build目录

make   # 编译修改的kernel 和 文件系统
```

### 6.4 uvc-camera 参数介绍

> video_device=/dev/videoX v4l2设备节点
>
> camera_device_path camera设备节点
>
> package_size=value 包大小(8~3072)
>
> io_method=mmap|userptr 申请缓冲区⽅式
>
> nbufs=value 申请缓冲区数量
>

### 6.5 uvc使用流程

#### 6.5.1 单路uvc使用流程

> Usage : ./uvc-camera <camera_device=/dev/camera> <video_device=/dev/video0> <package_size=value> [fps=value] [io_method=mmap|userptr] [nbufs=value]

```c
预览单路uvc摄像头运行命令
./uvc-camera camera_device=/dev/camera video_device=/dev/videoX package_size=1024 fps=15 io_method=mmap nbufs=3

特别注意以下事项：
1.package_size的大小需要与修改的uvc传输的包大小streaming_maxpacket一样.
2./dev/videoX videoX节点需要用uvc的video节点替换。
3.预览的格式、宽、高是摄像头出图的格式、宽、高进行预览。
4.预览的格式和分辨率需要与kernel支持的格式和分辨率一致。
```

#### 6.5.2 多路uvc使用流程

> Usage : ./uvc-camera <camera_device=/dev/camera> <video_device=/dev/video0> <package_size=value> [fps=value] [io_method=mmap|userptr] [nbufs=value]

```c
预览多路uvc摄像头命令

UVC通道0
./uvc-camera camera_device=/dev/camera video_device=/dev/videoX package_size=1024 fps=15 io_method=mmap nbufs=3 &

UVC通道1
./uvc-camera camera_device=/dev/camera1 video_device=/dev/videoY package_size=1024 fps=15 io_method=mmap nbufs=3 &

特别注意以下事项：
1.package_size的大小需要与修改的uvc传输的包大小streaming_maxpacket一样.
2./dev/videoX 和 /dev/videoY videoX和videoY节点需要用uvc的video节点替换。
3.uvc-camera预览数据的宽高是以摄像头出图的宽高进行预览。
4.预览的格式和分辨率需要与kernel支持的格式和分辨率一致。
```

## 7. 过ISP的摄像头使用流程

### 7.1 切换到uvc应用层目录

```c
cd doc/开发使用说明/USB使用说明文档/设备/USB_UVC/xburst1/
```

### 7.2 编译应用程序

```c
../../../../../../buildroot/buildroot/output/host/usr/bin/mips-linux-gnu-gcc uvc-isp.c uvc_lib.c -limpf -lrt -o uvc-isp
```

### 7.3 将生成的可执行文件拷贝到文件系统然后打包到镜像中

```c
cp uvc-isp ../../../../../../buildroot/buildroot/output/target/

cd build    #切换到build目录

make   # 编译修改的kernel 和 文件系统
```

### 7.4 uvc-isp 参数介绍

> video_device=/dev/videoX v4l2设备节点
>
> sensor_name=name sensor的名称需要与libimpf/src/cmds/sensor_info.h文件的sensor名称一致
>
> i2c_device_addr=value sensor的i2c设备地址
>
> i2c_adapter_id=value sensor挂载i2c的总线号
>
> format=nv12|yuyv|grey 预览格式
>
> width=value 预览数据的宽
>
> height=value 预览数据的高
>
> fps=value 预览数据的帧率
>
> package_size=value 包大小(8~3072)
>
> io_method=mmap|userptr 申请缓冲区⽅式
>
> nbufs=value 申请缓冲区数量

### 7.5 uvc使用流程

#### 7.5.1 单路uvc使用流程

> Usage : ./uvc-isp <video_device=/dev/videoX> <sensor_name=name> <i2c_device_addr=value> <i2c_adapter_id=value> <format=nv12|yuyv|grey> <width=value> <height=value> <fps=value> <package_size=value> [io_method=mmap|userptr] [nbufs=value]

```c
预览单路uvc摄像头运行命令
./uvc-isp video_device=/dev/videoX sensor_name=ov2735 i2c_device_addr=0x3c i2c_adapter_id=0 format=yuyv width=640 height=360 fps=10 package_size=1024 io_method=mmap nbufs=3

特别注意以下事项：
1.sensor_name的名称需要与libimpf/src/cmds/sensor_info.h文件定义的sensor名称一致，否则不能使用isp。
2.package_size的大小需要与修改的uvc传输的包大小streaming_maxpacket一样。
3./dev/videoX videoX节点需要用uvc的video节点替换。
4.预览的格式和分辨率需要与kernel支持的格式和分辨率一致。
```

#### 7.5.2 多路uvc使用流程

> Usage : ./uvc-isp <video_device=/dev/videoX> <sensor_name=name> <i2c_device_addr=value> <i2c_adapter_id=value> <format=nv12|yuyv|grey> <width=value> <height=value> <fps=value> <package_size=value> [io_method=mmap|userptr] [nbufs=value]

```c
预览多路uvc摄像头命令

UVC通道0
./uvc-isp video_device=/dev/videoX sensor_name=ov2735 i2c_device_addr=0x3c i2c_adapter_id=0 format=mjpg width=1280 height=720 fps=10 package_size=1024 io_method=mmap nbufs=3 &

UVC通道1
./uvc-isp video_device=/dev/videoY sensor_name=ov9732 i2c_device_addr=0x10 i2c_adapter_id=1 format=nv12 width=640 height=360 fps=10 package_size=1024 io_method=mmap nbufs=3 &

特别注意以下事项：
1.sensor_name的名称需要与libimpf/src/cmds/sensor_info.h文件定义的sensor名称一致，否则不能使用isp。
2.package_size的大小需要与修改的uvc传输的包大小streaming_maxpacket一样。
3./dev/videoX 和 /dev/videoY videoX和videoY节点需要用uvc的video节点替换。
4.预览的格式和分辨率需要与kernel支持的格式和分辨率一致。
```

