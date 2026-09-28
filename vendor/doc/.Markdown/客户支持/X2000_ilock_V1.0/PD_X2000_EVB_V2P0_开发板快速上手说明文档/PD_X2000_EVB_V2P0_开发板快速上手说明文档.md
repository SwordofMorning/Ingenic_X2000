# 一 编译方法

**整体编译命令如下:**

```c
bhu@bhu-PC:~/work$ make x2000_nand_defconfig

bhu@bhu-PC:~/work$ make 
```

**编译以后固件目录,如下:**

```c
bhu@bhu-PC:~/work/build/output$ ls -lh
总用量 8.0M
-rw-r--r-- 1 bhu bhu 4.4M 4月  24 14:45 rootfs.squashfs
-rw-r--r-- 1 bhu bhu  24K 4月  24 14:45 u-boot-spl-pad.bin
-rw-r--r-- 1 bhu bhu 3.7M 4月  24 14:45 xImage
```



# 二 最新烧录工具获取

**ubuntu版本**

```c
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz 
```

**windows版本**

```c
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windows.zip
```



# 三 烧录方法

以ubuntu版工具为例

接上板子上的TypeC接口，该板子供电和烧录是同一接口，进入烧录工具

```c
bhu@bhu-PC:~/Desktop/cloner-2.5.33-ubuntu_alpha$ sudo ./cloner 
```

![1](PD_X2000_EVB_V2P0_开发板快速上手说明文档.assets/1.png)

![2](PD_X2000_EVB_V2P0_开发板快速上手说明文档.assets/2.png)

![3](PD_X2000_EVB_V2P0_开发板快速上手说明文档.assets/3.png)

配置好后保存配置，板子上长按PWR-KEY不动，再长按USB-BOOT_KEY不动，直到进入烧录模式，进度条开始走，松开



# 四 常用功能



## 1. 串口调试

> **该板子使用到的串口为uart3, 默认配置都是uart2,因此需要修改bootloader/uboot-x2000目录下的boards.cfg文件，才能看见打印**

修改内容在x2000_base_xImage_sfc_nand行最后添加 SYS_UART_INDEX=3

```c
x2000_base_xImage_sfc_nand   mips        xburst2    x2000_base   ingenic    x2000_v12   x2000_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_BOOT,RMEM_MB=16,SPL_PARAMS_FIXER,SYS_UART_INDEX=3
```

> **开机后，需长按PWR-KEY几秒才会进到系统看见打印，波特率为3000000**



## 2. 辅助命令

该板为针板，无外设，若要验证连接外设，可使用辅助命令，具体用法，可见doc/开发使用说明/Linux辅助开发

```c
/# cmd_

cmd_adc                      cmd_mcu
cmd_aes                      cmd_mscaler
cmd_camera                   cmd_nemc
cmd_camera_h264_encode       cmd_ps2
cmd_camera_jpeg_encode       cmd_pwm
cmd_camera_nv12_preview      cmd_pwm_audio
cmd_camera_software_preview  cmd_pwm_battery
cmd_dtrng                    cmd_rector_play
cmd_efuse                    cmd_rotator
cmd_fb                       cmd_rsa
cmd_fb_scale                 cmd_rtc
cmd_gpio                     cmd_sc
cmd_gpio_counter             cmd_spi
cmd_hash                     cmd_sslv
cmd_i2c                      cmd_uevent
cmd_inputdev_listen          cmd_usb_device_state
cmd_isp                      cmd_v4l2_camera
cmd_keyboard                 cmd_watchdog
cmd_keyboard_test            cmd_wifi

# cmd_adc

Usage1:         cmd_adc <operation> <channel>
Example1:
        cmd_adc get_value 0
        cmd_adc get_voltage 0

Usage2:         cmd_adc <operation> [ref_voltage]
Example2:
        cmd_adc set_vref 1200
        cmd_adc get_vref
Usage3:cmd_adc [-h/--help]
Example3:
        cmd_adc --help
```

