# x2000-darwin 如何使用USB摄像头

## 1.Darwin_x2000_V2.0板子  芯片x2000H

### 1.1.系统准备

按照文档  doc/FAE文档/X2000H_Darwin_X2000_V2.0/x2000H_Darwin_v2.0_开发板快速上手说明.pdf   进行编译。



1.2 配置kernel

按照  /doc/开发使用说明/USB使用说明文档/主机/USB_STORAGE.pdf 进行kernel的配置，让usb处于 host模式。然后再进行如下配置



![2022-09-23_17-14](x2000H-darwin如何使用USB摄像头.assets/1.png)









![2022-09-23_17-21](x2000H-darwin如何使用USB摄像头.assets/2.png)



配置完以后，编译烧录，然后运行如下命令。前提是adb不能使用。插上USB摄像头以后，在串口进行如下操作：

ffmpeg -i /dev/video4 -pix_fmt nv12 -f x2000_fb /dev/fb0