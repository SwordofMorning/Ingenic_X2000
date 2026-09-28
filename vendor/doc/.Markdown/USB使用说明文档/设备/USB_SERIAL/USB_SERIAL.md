# USB_SERIAL

## 1. menuconfig 配置流程

```c
Device Drivers  --->
         [*] USB support  --->
                  <*>   USB Gadget Support  --->
                             <*>   USB Gadget Drivers
                                         (USB functions configurable through configfs)  --->
                             [*]       Generic serial bulk in/out
                             [*]       Abstract Control Model (CDC ACM)
```

## 2. serial设备参数配置

在终端执行serial设备参数配置脚本 （文件路径：doc/开发使用说明/usb使用说明文档/设备/USB_SERIAL/serial_device_config.sh ）

> 命令：./serial_config.sh start
>
> 执行命令后会产生/dev/ttyGS0节点

## 3. serial设备数据收发流程

> 测试应用文件 tty_receive.c 用来接收， tty_send.c 用来发送（文件路径：doc/开发使用说明/USB使用说明文档/设备/USB_SERIAL）

### 3.1.PC端发送数据，设备端接收数据



> cd doc/开发使用说明/USB使用说明文档/设备/USB_SERIAL
>
> ../../../../../buildroot/buildroot/output/host/usr/bin/mips-linux-gnu-gcc tty_receive.c -o tty_receive
>
> gcc tty_send.c -o tty_send



设备端先执行命令进行接收数据

> 用法：./tty_receive <设备节点> <接收数据大小>
>
> ./tty_receive /dev/ttyGS0 468890 > data1.txt

PC端再执行命令进行发送数据

> 用法：/tty_send  <发送文件路径> <设备节点> <一次性发送长度>
>
> ./tty_send ./data0.txt /dev/ttyACM0 468890

### 3.2.设备端发送数据，PC端接收数据

> cd doc/开发使用说明/USB使用说明文档/设备/USB_SERIAL
>
> ../../../../../buildroot/buildroot/output/host/usr/bin/mips-linux-gnu-gcc  tty_send.c -o tty_send
>
> gcc tty_receive.c -o tty_receive



PC端先执行命令进行接收数据

> 用法：./tty_receive <设备节点> <接收数据大小>
>
> ./tty_receive /dev/ttyACM0 468890 > data1.txt

设备端再执行命令进行发送数据

> 用法：/tty_send  <发送文件路径> <设备节点> <一次性发送长度>
>
> ./tty_send ./data0.txt /dev/ttyGS0 468890