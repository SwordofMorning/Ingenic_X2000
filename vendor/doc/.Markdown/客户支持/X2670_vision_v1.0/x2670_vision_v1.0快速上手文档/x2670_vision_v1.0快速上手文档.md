# 					x2670_vision_v1.0快速上手文档

## 一 硬件介绍

本开发板采用x2670 + sfc nand

供电：Type-C插入可以烧录&供电，不需额外供电

串口输出：uart2, PD12～13, 波特率：3000000   

绿色串口小板连接的是wifi固件下载的uart0  PE9～10, 支持的串口波特率是115200。

蓝色串口小板是系统log输出口，即上面的uart2, PD12～13, 波特率：3000000   

![1](x2670_vision_v1.0快速上手文档.assets/1.jpg)

![2](x2670_vision_v1.0快速上手文档.assets/2.jpg)

具体接口如下：

![3](x2670_vision_v1.0快速上手文档.assets/3.png)

## 二  获取源码方法

在获取源码之前，**都需要把你开发电脑上的相应的 public key发送给我司进行添加**，从而得到获取源码的权限。本地电脑获取 public key的方法如下： 

如果本机还没有 ssh public key,可以通过以下方法生成, 打开一个终端输入下列命令: 

```c
jiangwen@uws:~/work/x1000/zk_external_git_test$ ssh-keygen
```

//输入 ssh-keygen 后,使用 
默认配置,一路回车直到完成即可 

```c
jiangwen@uws:~/work/x1000/zk_external_git_test$ cat ~/.ssh/id_rsa.pub
```

Repo 工具下载
：

```c
$ mkdir project_workspace
$ cd project_workspace
$ wget http://git.ingenic.com.cn:8082/bj/repo
$ chmod +x repo
```

同步代码的地址如下： 

```c
./repo init -u ssh://sz_halley2@119.136.25.25:29418/mirror/linux/manifest
./repo sync
```

如若报错可使用http同步方式：

```c
./repo init -u http://sz_halley2@119.136.25.25:8089/mirror/linux/manifest
./repo sync
```

注意：

**1, 对于外部客户来说，这里的帐号不用修改，直接使用sz_halley2就可以。** 

**2, 客户下载代码的目录路径要用全英文的，不要带有特殊符号的那种，不然编译可能会有问题**

 

同步代码可能遇到的问题如下：

主要是因为系统环境,网络环境不一致导致在同步工程过程中出现的一些报错信息的解决方法。

这里的获取代码是基于Ubuntu14.04  64bit的环境进行的。

**问题1：-bash: ./repo: No such file or directory**

解决方法：

​                系统缺少repo工具或没有添加环境变量

​                wget http://git.ingenic.com.cn:8082/bj/repo  (注:此链接的repo工具为君正修改过后放在服务器上的，也可使用谷歌提供的repo工具)

​                chmod +x repo



**问题2：Permission denied (publickey).**

fatal: Could not read from remote repository.

 解决方法：

请确认是否有权限同步代码，同步代码需提交key于我司开通权限



**问题3：Their offer: diffie-hellman-group1-sha1**

解决方法：

修改~/.ssh/config，加入

Host *

​                KexAlgorithms +diffie-hellman-group1-sha1

 

**问题4：aes128-ctr，aes192-ctr，aes256-ctr**

解决方法：

修改 /etc/ssh/ssh_config 文件

删除Ciphers aes128-ctr，aes192-ctr，aes256-ctr….行前注释符号



**问题5：git config --global user.name "yourname"**

​              git config --global user.email your@email.com

解决方法：

若不需要提交代码至服务器可直接跳过执行后续步骤

若需要提交代码请根据提示命令注册姓名及邮箱地址

git config --global user.name "your name"

git config --global user.email "you email"

 

**问题6：Traceback (most recent call last):**

File "/home/jdai/work/test1/.repo/repo/main.py", line 385, in <module>

  _Main(sys.argv[1:])

File "/home/jdai/work/test1/.repo/repo/main.py", line 365, in _Main

result = repo._Run(argv) or 0

 File "/home/jdai/work/test1/.repo/repo/main.py", line 137, in _Run

解决方法：

rm –rf ***\*.\****repo  (repo前面有***\*.\****)

 ./repo init –u ssh://……….  

使用repo init 同步没有执行完会产生缓存需删除缓存后再次执行。

 

**问题7：Testing colorized output (for 'repo diff', 'repo status'):**

 black   red    green   yellow  blue   magenta  cyan   white 

 bold   dim    ul    reverse 

Enable color display in this user account (y/N)?

解决方法：

直接按回车键

   注意：若在同步过程中长时间卡住不动，有可能是因为库太大，或进程卡死，请ctrl+c 键退出再执行，支持断点续传。



**问题8：Bad owner or permissions on .ssh/config** 

解决方法：

`sudo chmod 600 .ssh/config`

如果已经获取到了源码，可以见详细的源码获取方法：

`doc/FAE文档/获取Linux平台源码的方法.pdf`

doc/FAE文档/获取源码问题总结.pdf

## 三 编译方法

本开发板使用的工程配置文件为：x2670_vision_v10_nand_5.10_factory_defconfig

如果需要更改默认配置，可以使用工具 IConfigTool 进行配置，详细使用方法见文档：

`doc/开发使用说明/IConfigTool 使用文档.pdf`



整体编译：

```c
sxyzhang@T430:~/x2670_sz/build$ make clean            # 第一次编译需要，后续编译都不需要执行整体clean操作
sxyzhang@T430:~/x2670_sz/build$ make x2670_vision_v10_nand_5.10_factory_defconfig
sxyzhang@T430:~/x2670_sz/build$ make
```

编译后生成固件如下：

```c
sxyzhang@T430:~/my/work/linux/x2670_sz/build$ ll output/
总用量 15292
drwxrwxr-x 2 sxyzhang sxyzhang     4096 6月  15 11:28 ./
drwxrwxr-x 6 sxyzhang sxyzhang     4096 6月  15 12:05 ../
-rw-r--r-- 1 sxyzhang sxyzhang 11051008 6月  15 11:28 rootfs.squashfs
-rw-rw-r-- 1 sxyzhang sxyzhang    24576 6月  15 11:28 u-boot-spl-pad.bin
-rw-rw-r-- 1 sxyzhang sxyzhang  4571200 6月  15 12:05 xImage
```

## 四  最新烧录工具获取

在ubuntu下执行如下命令可以免密下载：

##### **ubuntu版本：**

```c
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz
```

##### windows版本：

```c
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windows.zip
```

## 五 烧录方法

**x2670的烧录配置仅在烧录工具2.5.35及以后的版本中有添加**，请务必使用对应版本的烧录工具来烧录。具体烧录配置如下：

<img src="x2670_vision_v1.0快速上手文档.assets/4.png" alt="4" style="zoom:150%;" />

<img src="x2670_vision_v1.0快速上手文档.assets/5.png" alt="5" style="zoom:150%;" />

<img src="x2670_vision_v1.0快速上手文档.assets/6.png" alt="6" style="zoom:150%;" />

<img src="x2670_vision_v1.0快速上手文档.assets/7.png" alt="7" style="zoom:150%;" />

<img src="x2670_vision_v1.0快速上手文档.assets/8.png" alt="8" style="zoom:150%;" />

## 六 基本功能测试

本配置生成的固件支持工厂测试模式，已经对基础功能运行脚本测试了。具体执行的测试脚本为： /etc/init.d/S99factory_test_shell， 感兴趣的话，可以看一下当前针对各个功能模块测试所执行的命令。

### 6.1  查看当前安装的驱动

```c
lsmod 								   #查看当前已安装的驱动
#insmod [驱动文件]						#安装驱动
#rmmod [驱动文件]						#卸载驱动
```

<img src="x2670_vision_v1.0快速上手文档.assets/9.png" alt="9" style="zoom:150%;" />



### 6.2  LCD显示

```c
cmd_fb info /dev/fb0                        #展示当前fb0的硬件信息
cmd_fb enable /dev/fb0                      #使能屏幕
cmd_fb clear /dev/fb0 color=0xff00ff00      #清屏为绿⾊
cmd_fb draw_rect /dev/fb0 color=0xff0000ff frame_index=0 x=0 y=0 width=400 hei
ght=200                                     #绘制一个蓝色小色块
cmd_fb display /dev/fb0                     #显⾊
cmd_fb disable /dev/fb0                     #失能屏幕
```

### 6.3  wifi使用

本开发板使用wifi芯片为Hi3861L。 固件需要重新烧录，出厂没有自带固件。

由原理图可知，当前wifi下载固件使用的是uart0   PE9～10。

![10](x2670_vision_v1.0快速上手文档.assets/10.png)

固件下载的烧录工具的使用方法如下：

wireless\hisilicon\hi3861l\firmware\ReleaseDoc\zh\04 工具资料\Hi3861V100／Hi3861LV100 HiBurn工具 使用指南.pdf

固件下载的烧录工具位置如下：

wireless\hisilicon\hi3861l\firmware\src\Tools\burn\HiBurn.exe

待烧录的固件为：

wireless\hisilicon\hi3861l\firmware\Hi3861L_demo_allinone.bin

具体烧录方法如下（需要整体板子断电后重新上电才能进入烧录模式）：

<img src="x2670_vision_v1.0快速上手文档.assets/11.png" alt="11" style="zoom:200%;" />

烧录完成后整个板子重新上电，将能看到wifi热点枚举

![12](x2670_vision_v1.0快速上手文档.assets/12.png)

具体相关操作信息见：doc\开发使用说明\bcm bt&wifi使用说明文档\Hi3861l-WiFi模块使用说明文档.pdf

### 6.4  视频播放

#### 6.4.1  硬件解码性能

x2670 主控的硬件H.264 解码支持：

- 最大分辨率:2560x2048
- 最大解码能力:1920x1080@60fps
- 输出格式:NV12

#### 6.4.2 选中U盘的支持

当前测试是将视频源放在U盘中进行的，所以要可以挂载U盘，需要以下配置：

<img src="x2670_vision_v1.0快速上手文档.assets/13.png" alt="13" style="zoom:150%;" />

#### 6.4.3 选中player_test命令相关软件支持

<img src="x2670_vision_v1.0快速上手文档.assets/14.png" alt="14" style="zoom:150%;" />

<img src="x2670_vision_v1.0快速上手文档.assets/15.png" alt="15" style="zoom:150%;" />

<img src="x2670_vision_v1.0快速上手文档.assets/16.png" alt="16" style="zoom:150%;" />

#### 6.4.4 buildroot中打开ffmpeg

确定当前buildroot的配置：

```c
sxyzhang@T430:~/x2670_sz/build$ cat configs/x2670_vision_v10_nand_5.10_factory_defconfig | grep buildroot
APP_br_dir=../buildroot/buildroot
APP_br_config_file=configs/buildroot/buildroot_x2600_510_video_deconfig
```

可知当前buildroot路径为：../buildroot/buildroot

buildroot 默认配置为：configs/buildroot/buildroot_x2600_510_video_deconfig

进入buildroot目录，配置ffmpeg：

```c
sxyzhang@T430:~/x2670_sz/build$ cd ../buildroot/buildroot
sxyzhang@T430:~/x2670_sz/buildroot/buildroot$ make menuconfig
```

<img src="x2670_vision_v1.0快速上手文档.assets/17.png" alt="17" style="zoom:150%;" />

保存退出后，将buildroot的上述配置添加进默认配置，以免在build中重新执行make x2670_vision_v10_nand_5.10_factory_defconfig时覆盖上述配置。

```
sxyzhang@T430:~/x2670_sz/buildroot/buildroot$ cp .config ../../build/configs/buildroot/buildroot_x2600_510_video_deconfig
```

#### 6.4.5 修改kernel的dts文件

播放大视频时，需要更多的缓存支持。所以应该在kernel的dts中添加相应reserved memory配置。

确定当前kernel路径及配置：

```c
sxyzhang@T430:~/x2670_sz/build$ cat configs/x2670_vision_v10_nand_5.10_factory_defconfig | grep kernel
APP_kernel_dir=../kernel/kernel
APP_kernel_config=x2600_module_base_linux_sfc_nand_defconfig
```

可知当前x2670_vision_v10_nand_5.10_factory_defconfig配置的kernel编译路径为：../kernel/kernel

kernel 配置文件为：x2600_module_base_linux_sfc_nand_defconfig

确定当前kernel的dts文件：

```c
sxyzhang@T430:~/x2670_sz/build$ cd ../kernel/kernel
sxyzhang@T430:~/x2670_sz/kernel/kernel$ cat arch/mips/configs/x2600_module_base_linux_sfc_nand_defconfig | grep dts
CONFIG_DT_X2600_MODULE_BASE_DTS_FILE="x2600_module_base.dts"
sxyzhang@T430:~/x2670_sz/kernel/kernel$ find ./ -name x2600_module_base.dts
./module_drivers/dts/x2600_module_base.dts
```

可见，当前kernerl加载的dts文件为：kernel/kernel/module_drivers/dts/x2600_module_base.dts，添加reserved memory配置：

<img src="x2670_vision_v1.0快速上手文档.assets/18.png" alt="18" style="zoom:150%;" />

<img src="x2670_vision_v1.0快速上手文档.assets/19.png" alt="19" style="zoom:200%;" />

#### 6.4.6 编译烧录

```c
sxyzhang@T430:~/x2670_sz/build$ make x2670_vision_v10_nand_5.10_factory_defconfig
sxyzhang@T430:~/x2670_sz/build$ make
```

根据原理图可见，开发板的host口如要使用，需给其供电PC14为高电平：

<img src="x2670_vision_v1.0快速上手文档.assets/20.png" alt="20" style="zoom:150%;" />

进行如下操作来挂载U盘：

<img src="x2670_vision_v1.0快速上手文档.assets/21.png" alt="21" style="zoom:150%;" />

查看格式：

```c
# ffmpeg -i /tmp/mass_storage/sda1/media/other/naxiehuaer.mp4 

ffmpeg version 4.2.2 Copyright (c) 2000-2019 the FFmpeg developers
  built with gcc 7.2.0 (Ingenic Linux-Release5.1.0-Default(xburst2(fp64)
  configuration: --enable-cross-compile --cross-prefix=/home/sxyzhang/my/work/linux/x2670_sz/buildroot/buildroot/output/host/bin/mips-linux-gnu- --sysroot=/home/sxyzhanc
  libavutil      56. 31.100 / 56. 31.100
  libavcodec     58. 54.100 / 58. 54.100
  libavformat    58. 29.100 / 58. 29.100
  libavdevice    58.  8.100 / 58.  8.100
  libavfilter     7. 57.100 /  7. 57.100
  libavresample   4.  0.  0 /  4.  0.  0
  libswscale      5.  5.100 /  5.  5.100
  libswresample   3.  5.100 /  3.  5.100
Input #0, mov,mp4,m4a,3gp,3g2,mj2, from '/tmp/mass_storage/sda1/media/other/naxiehuaer.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 1
    compatible_brands: isomavc1
    creation_time   : 2015-07-01T12:20:36.000000Z
    album           : Yinyuetai
    artist          : yinyuetai.com
    comment         : Yinyuetai-1UP1200
    date            : 07/01/15 20:20:36
  Duration: 00:05:02.58, start: 0.000000, bitrate: 6140 kb/s
    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080, 6007 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (default)
    Metadata:
      creation_time   : 2015-07-01T12:20:36.000000Z
      handler_name    : 264@GPAC0.5.1-DEV-rev5472
    Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706D), 44100 Hz, stereo, fltp, 128 kb/s (default)
    Metadata:
      creation_time   : 2015-07-01T12:10:27.000000Z
      handler_name    : Sound Media Handler
At least one output file must be specified
```

播放：

```c
# player_test /tmp/mass_storage/sda1/media/other/naxiehuaer.mp4 

Input #0, mov,mp4,m4a,3gp,3g2,mj2, from '/tmp/mass_storage/sda1/media/other/naxiehuaer.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 1
    compatible_brands: isomavc1
    creation_time   : 2015-07-01T12:20:36.000000Z
    album           : Yinyuetai
    artist          : yinyuetai.com
    comment         : Yinyuetai-1UP1200
    date            : 07/01/15 20:20:36
  Duration: 00:05:02.58, start: 0.000000, bitrate: 6140 kb/s
    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080, 6007[  601.819727] h264_decode_init done!
 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (default)
[  601.820197] Create instance [0]@ee6fae63 m2m_ctx=70ffcc7f

      creation_time   : 2015-07-01T12:20:36.000000Z
      hand[  601.820718] 13300000.felix vcodec [0]

ler_name    : 264@GPAC0.5.1-DEV-rev5472
    Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706[  601.821307] tmp_w 256 tmp_h 120, w 160 h 120
D), 44100 Hz, stereo, fltp, 128 kb/s (default)
    Metadata:
 [  601.821799] [0] vcodec release
     creation_time   : 2015-07-01T12:10:27.000000Z
      handle[  601.822213] total_alloc_cnt: 1, total_alloc_size: 131072, total_free_size:131072, total_free_cnt:1
r_name    : Sound Media Handler
[h264_v4l2m2m @ 0x1fb2880] driver 'felix-vdec' on ca[  601.823196] h264_decode_init done!
rd 'vpu-felix'
[h264_v4l2m2m @ 0x1fb2880] Using devi[  601.823554] Create instance [0]@ee6fae63 m2m_ctx=562f3ea5
ce /dev/video2
[  601.824107] 13300000.felix vcodec [0]
[h264_v4l2m2m @ 0x1fb2880] driver 'felix-vdec' on car[  601.824463] tmp_w 1920 tmp_h 1080, w 1920 h 1080
d 'vpu-felix'
[  601.824967] tmp_w 1920 tmp_h 1080, w 1920 h 1080
```

### 6.5  LVGL demo UI 播放视频

#### 6.5.1 设置触摸屏配置

<img src="x2670_vision_v1.0快速上手文档.assets/22.png" alt="22" style="zoom:150%;" />

#### 6.5.2 设置fb支持旋转

因为一般的片源都是横屏分辨率，而本开发板默认lcd屏幕设备的分辨率是：720X1280, 那么就需要fb控制器支持旋转成横屏。

<img src="x2670_vision_v1.0快速上手文档.assets/23.png" alt="23" style="zoom:150%;" />

<img src="x2670_vision_v1.0快速上手文档.assets/24.png" alt="24" style="zoom:150%;" />

fb控制器需要打开多层显示，建议设置如下：

<img src="x2670_vision_v1.0快速上手文档.assets/25.png" alt="25" style="zoom:150%;" />

#### 6.5.3 打开lvgl驱动支持

<img src="x2670_vision_v1.0快速上手文档.assets/26.png" alt="26" style="zoom:150%;" />

对应代码在：x2670_sz\third_party\lvgl

#### 6.5.4 设置开机自启动lvgl ui展示

<img src="x2670_vision_v1.0快速上手文档.assets/27.png" alt="27" style="zoom:150%;" />

对应脚本为：x2670_sz\buildroot\buildroot_patch\rootfs_config\file\third_party_lvgl\S32run_third_party_video_player

当然了，对于本开发板来说，因为音视频源存放在U盘中，而U盘又需要额外gpio供电才可以正常枚举和挂载。所以需手动启动lvgl的播放程序：

```c
cmd_gpio set_func pc14 output1                   # 给U盘供电使其正常挂载
```

```c
# lv_widgets_player&       # 运行lvgl的播放程序
# argv[0] = lv_widgets_player
argv[0] len = 17
btn_userdata->name = /tmp/mass_storage/sda1/media/other/nufang.mp4
Input #0, mov,mp4,m4a,3gp,3g2,mj2, from '/tmp/mass_storage/sda1/media/other/nufang.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 512
    compatible_brands: isomiso2avc1mp41
    encoder         : Lavf58.76.100
  Duration: 00:04:46.93, start: 0.000000, bitrate: 461 kb/s
    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 512x294 [SAR 1:1 DAR 256:147], 331 kb/s, 15 fps, 15 tbr, 1)
    Metadata:
      handler_name    : VideoHandler
    Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706D), 44100 Hz, stereo, fltp, 126 kb/s (default)
    Metadata:
      handler_name    : SoundHandler
btn_userdata->name = /tmp/mass_storage/sda1/media/other/wodengdaohuaer.mp4
Input #0, mov,mp4,m4a,3gp,3g2,mj2, from '/tmp/mass_storage/sda1/media/other/wodengdaohuaer.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 1
    compatible_brands: isomavc1iso6
    creation_time   : 2015-07-22T01:18:20.000000Z
    album           : Yinyuetai
    artist          : yinyuetai.com
    comment         : Yinyuetai-1TR1026
    date            : 07/22/15 09:18:20
  Duration: 00:05:27.61, start: 0.000000, bitrate: 6121 kb/s
    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080, 6021 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (def)
    Metadata:
      creation_time   : 2015-07-22T01:18:20.000000Z
      handler_name    : 264@GPAC0.5.1-DEV-rev5472
    Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706D), 44100 Hz, stereo, fltp, 95 kb/s (default)
    Metadata:
      creation_time   : 2015-07-22T01:11:01.000000Z
      handler_name    : Sound Media Handler
btn_userdata->name = /tmp/mass_storage/sda1/media/other/[4K123]4K-super_car2.mp4
Input #0, mov,mp4,m4a,3gp,3g2,mj2, from '/tmp/mass_storage/sda1/media/other/[4K123]4K-super_car2.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 512
    compatible_brands: isomiso2avc1mp41
    encoder         : Lavf57.41.100
  Duration: 00:02:44.59, start: 0.000000, bitrate: 2943 kb/s
    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080 [SAR 1:1 DAR 16:9], 2744 kb/s, 29.97 fps, 29.97 )
    Metadata:
      handler_name    : VideoHandler
    Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 190 kb/s (default)
    Metadata:
      handler_name    : SoundHandler
btn_userdata->name = /tmp/mass_storage/sda1/media/other/naxiehuaer.mp4
Input #0, mov,mp4,m4a,3gp,3g2,mj2, from '/tmp/mass_storage/sda1/media/other/naxiehuaer.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 1
    compatible_brands: isomavc1
    creation_time   : 2015-07-01T12:20:36.000000Z
    album           : Yinyuetai
    artist          : yinyuetai.com
    comment         : Yinyuetai-1UP1200
    date            : 07/01/15 20:20:36
  Duration: 00:05:02.58, start: 0.000000, bitrate: 6140 kb/s
    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080, 6007 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (def)
    Metadata:
      creation_time   : 2015-07-01T12:20:36.000000Z
      handler_name    : 264@GPAC0.5.1-DEV-rev5472
    Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706D), 44100 Hz, stereo, fltp, 128 kb/s (default)
    Metadata:
      creation_time   : 2015-07-01T12:10:27.000000Z
      handler_name    : Sound Media Handler

```

至此，屏幕上应该就能看到文件列表，可以点击播放试试。