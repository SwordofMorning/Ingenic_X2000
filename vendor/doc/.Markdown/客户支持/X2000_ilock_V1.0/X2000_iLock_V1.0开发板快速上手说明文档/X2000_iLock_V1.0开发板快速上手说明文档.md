# X2000_iLock_V1.0开发板快速上手说明文档

## 一 开发板介绍

### 1 硬件简介

​			X2000_iLock_V1.0 这个开发板默认支持x2000，x2000E，x2000H。支持两种启动方式：

​					I .SDIO flash，接主控的MSC2通路

​					II. eMMC，接主控的MSC0通路

### 2 硬件板子

​							<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_01.png" style="zoom:150%;" />							

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_02-1669952778505-4.png" style="zoom:150%;" />

​							

## 二 获取 X2000_iLock_V1.0 开发板的源码方法

​			在获取源码之前，都需要把你开发电脑上的相应的 public key发送给我司进行添加，从而得到获取源码的权限。本地电脑获取 public key的方法如下： 

如果本机还没有 ssh public key,可以通过以下方法生成, 打开一个终端输入下列命令: 

```
jiangwen@uws:~/work/x1000/zk_external_git_test$ ssh-keygen
```

//输入 ssh-keygen 后,使用 
默认配置,一路回车直到完成即可 

```
jiangwen@uws:~/work/x1000/zk_external_git_test$ cat ~/.ssh/id_rsa.pub
```



Repo 工具下载
`$ mkdir project_workspace`
`$ cd project_workspace`
`$ wget http://git.ingenic.com.cn:8082/bj/repo`
`$ chmod +x repo`

同步代码的地址如下： 

```
./repo init -u ssh://sz_halley2@119.136.25.25:29418/mirror/linux/manifest
```

`./repo sync`

如若报错可使用http同步方式：

```
./repo init -u http://sz_halley2@119.136.25.25:8089/mirror/linux/manifest
```

`./repo sync`

**对于外部客户来说，这里的帐号不用修改，直接使用sz_halley2就可以。** 

**客户下载代码的目录路径要用全英文的，不要带有特殊符号的那种，不然编译可能会有问题**

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

## 三 获取最新烧录工具方法

**在ubuntu下执⾏如下命令可以免密下载**：

ubuntu版本：

```c
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz
```

windows版本：

```c
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windows.zip
```



## 四 X2000_iLock_V1.0 开发板编译及烧录方法

### 1 编译配置		

​		由于x2000，x2000E，x2000H 在硬件封装上pin对pin,可以直接替换,硬件电路不需要改动。
​		这三款芯片的主要差别为内部封装的DDR型号和容量不同，而这些DDR型号和容量不同体现在烧录工具中烧录选项的不同，所以代码上不需要额外重新配置和编译。根据实际硬件条件, 编译配置如下:

#### 1.1 X2000 + SDIO flash硬件的编译配置：

```
sxyzhang@T430:~/x2000_sz/build$ make x2000_ilock_mmc2_factory_test_defconfig
sxyzhang@T430:~/x2000_sz/build$ make clean
sxyzhang@T430:~/x2000_sz/build$ make
```

####             1.2 X2000E + SDIO flash硬件的编译配置：

```
sxyzhang@T430:~/x2000_sz/build$ make x2000_ilock_mmc2_factory_test_defconfig
sxyzhang@T430:~/x2000_sz/build$ make clean
sxyzhang@T430:~/x2000_sz/build$ make
```

#### 1.3 X2000H + SDIO flash硬件的编译配置：

```
sxyzhang@T430:~/x2000_sz/build$ make x2000_ilock_mmc2_factory_test_defconfig
sxyzhang@T430:~/x2000_sz/build$ make clean
sxyzhang@T430:~/x2000_sz/build$ make 
```

#### 1.4 X2000 + eMMC flash硬件的编译配置：

```
sxyzhang@T430:~/x2000_sz/build$ make x2000_ilock_mmc0_factory_test_defconfig
sxyzhang@T430:~/x2000_sz/build$ make clean
sxyzhang@T430:~/x2000_sz/build$ make
```

#### 1.5 X2000E + eMMC flash硬件条件的编译配置：

```
sxyzhang@T430:~/x2000_sz/build$ make x2000_ilock_mmc0_factory_test_defconfig
sxyzhang@T430:~/x2000_sz/build$ make clean
sxyzhang@T430:~/x2000_sz/build$ make
```

#### 1.6 X2000H + eMMC flash硬件条件的编译配置：

```
sxyzhang@T430:~/x2000_sz/build$ make x2000_ilock_mmc0_factory_test_defconfig
sxyzhang@T430:~/x2000_sz/build$ make clean
sxyzhang@T430:~/x2000_sz/build$ make
```



### 2 编译配置参数修改：

​		使用配置工具可以修改编译配置参数：

```
bhyuan@bhyuan-ingenic:~/workspace/X2000/tools/iconfigtool/IConfigToolApp$ ./IConfigTool 
```

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_03-1669607348243-6.png" style="zoom:150%;" />

​	点击Open打开配置文件，修改后按Ctrl + S保存

​	问题：

若打开配置 ./IConfigTool工具后出现闪退现象，将 lib文件夹内的这两个文件删除即可。

```
tools/iconfigtool$ cd IConfigToolApp/lib/
tools/iconfigtool/IConfigToolApp/lib$ rm libQtCore.so.4 libQtGui.so.4
```

### 3 文件烧录方法：

​	编译完成后，生成的固件在build/output目录下：

```
bhyuan@bhyuan-ingenic:~/workspace/X2000/build/output$ ls -lh
总用量 16M
-rw-r--r-- 1 bhyuan bhyuan  12M Nov 25 16:24 rootfs.squashfs
-rw-rw-r-- 1 bhyuan bhyuan  24K Nov 25 16:24 u-boot-spl-pad.bin
-rw-rw-r-- 1 bhyuan bhyuan 3.6M Nov 25 16:24 xImage
```

 	进入烧录工具目录下，打开烧录工具，记住使用管理权限打开:

```
bhyuan@bhyuan-ingenic:~/cloner-2.5.17-ubuntu_alpha$ sudo ./cloner 
```

#### 3.1 X2000 + SDIO flash硬件的烧录配置：

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_04.png" style="zoom:150%;" />



<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_05.png" style="zoom:150%;" />

#### 3.2 X2000E + SDIO flash硬件的烧录配置：

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_06.png" style="zoom:150%;" />

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_05-1670553879659-2.png" style="zoom:150%;" />

#### 3.3 X2000H + SDIO flash硬件的烧录配置：

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_07.png" style="zoom:150%;" />

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_05-1670555331471-5.png" style="zoom:150%;" />

#### 3.4 X2000 + eMMC flash硬件的烧录配置：

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_08.png" style="zoom:150%;" />

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_09.png" style="zoom:150%;" />

#### 3.5 X2000E + eMMC flash硬件条件的烧录配置：

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_10.png" style="zoom:150%;" />

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_09-1670555659151-10.png" style="zoom:150%;" />

#### 3.6  X2000H + eMMC flash硬件条件的烧录配置：

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_11.png" style="zoom:150%;" />

<img src="X2000_iLock_V1.0开发板快速上手说明文档.assets/X2000_iLock_V1.0_09-1670555791787-13.png" style="zoom:150%;" />

配置好烧录工具后，点击开始，然后接上 TYPE_C_USB_Power接口,长按Boot Key按键，再按下Reset Key按键再释放，最后再释放Boot Key按键，这样进入烧录模式,等待烧录完毕以后即可。

## 五 常用硬件基本功能验证:

### 	1 LCD

​			使用 LCD 之前需要开启屏幕背光,本配置已默认开启背光,可以查看到背光设备:

```
# ls /sys/class/backlight/
backlight_gpio0
```

​			fb操作相关命令:

```
cmd_fb enable /dev/fb0                        #使能屏幕设备节点
cmd_fb clear /dev/fb0  color=0xffff0000       #清屏:将屏幕上所有像素点均设为红色
cmd_fb display /dev/fb0                       #显色:使颜色在屏幕显示出来，此时屏幕全屏红色
cmd_fb clear /dev/fb0  color=0xff00ff00       #清屏:将屏幕上所有像素点均设为绿色
cmd_fb display /dev/fb0                       #显色:此时屏幕全屏绿色
cmd_fb clear /dev/fb0  color=0xff0000ff       #清屏:将屏幕上所有像素点均设为蓝色
cmd_fb display /dev/fb0                       #显色:此时屏幕全屏蓝色
cmd_fb disable /dev/fb0                       #失能屏幕,此时屏幕恢复成没有颜色状态
```

​		对应cmd_fb命令源文件在libhardware2/src/cmds/fb_main.c

### 2 Camera

​		摄像头按照使用方法分为两种,一种是经过ISP处理的,一种是不经过ISP处理的,本开发板摄像头ir不经过ISP处理,vis经过ISP处理.

查看设备节点及设备信息：

```
# ls /dev/cim                                    #摄像头不经过ISP，查看设备节点(master sensor)
/dev/cim

# ls /dev/vic*                                   #摄像头不经过ISP，查看设备节点(slave sensor)
/dev/vic1

# cmd_camera info /dev/cim                       #摄像头不经过ISP，查看设备信息(master sensor)
name: sc031-ir0
width: 640
height: 480
fps: 30
data_fmt: BYR2
line_length: 800
frame_size: 614400
frame_nums: 5
phys_mem: 06800000
mapped_mem: 0x7778e000


# ls /dev/mscaler*                               #摄像头经过ISP，查看设备节点
/dev/mscaler0-ch0  /dev/mscaler0-ch1  /dev/mscaler0-ch2

# cmd_isp init /dev/mscaler0-ch0 width=640 height=480 frame_nums=3 format=NV12  
摄像头经过ISP需要先初始化,width,height为图像宽和高,frame_nums为mscaler缓存帧个数,format为设备支持格式(仅支持NV12，NV21 YV12，JZ12，GREY)

# cmd_isp info /dev/mscaler0-ch0                 #摄像头经过ISP，查看设备信息
name        : gc2053-vis
width       : 640
height      : 480
fps         : 30
data_fmt    : NV12
line_length : 640
frame_size  : 460800
frame_nums  : 4
phys_mem    : 07400000
mapped_mem  : (nil)

```

​		具体使用流程:

```
# cmd_camera power_on /dev/cim                     #使能不经过isp摄像头ir,上电(master sensor)
# cmd_camera stream_on /dev/cim                    #开始图像录制
# cmd_camera get_frame /dev/cim > /tmp/frame       #获取图片命名为frame并保存到/tmp目录下
# cmd_camera stream_off /dev/cim                   #结束图像录制
# cmd_camera power_off /dev/cim                    #关闭设备,掉电
```

```
# cmd_isp init /dev/mscaler0-ch0 width=640 height=480 frame_nums=3 format=NV12   #初始化经过isp摄像头vis
# cmd_isp power_on /dev/mscaler0-ch0                                             #使能经过isp摄像头vis,上电
# cmd_isp stream_on /dev/mscaler0-ch0                                            #开始图像录制
# cmd_isp get_frame /dev/mscaler0-ch0 > /tmp/frame                               #获取图片命名为frame并保存到/tmp目录下
# cmd_isp stream_off /dev/mscaler0-ch0                                           #结束图像录制
# cmd_isp power_off /dev/mscaler0-ch0                                            #关闭设备,掉电
```

保存到的图片可以通过adb pull到本地进行查看,推荐使用7yuv 工具,选择图片格式与图片分辨率打开就可以看到图像效果了.对应cmd_camera,cmd_isp命令源文件

在libhardware2/src/cmds/camera_main.c，libisp/src/cmds/isp_main.c。

​		也可以通过命令直接预览摄像头所拍摄的图片,相关命令:

```
# cmd_camera_nv12_preview /dev/cim                  #不经过isp摄像头ir预览图像(master sensor)
# demo_isp_nv12_preview /dev/mscaler0-ch0           #经过isp摄像头vis预览图像
```

对应cmd_camera_nv12_preview，demo_isp_nv12_preview命令源文件在libhardware2/src/cmds/camera_nv12_preview.c，libisp/src/demo/demo_isp_nv12_preview.c.

### 3 WIFI

​		使用 WiFi 需要设置想要连接 WiFi 的名称及密码,将配置写入文件,启动 WiFi 时运行配置文件即可

```
# cd /usr/data/                                    #进入/usr/data/目录更改wifi配置文件

# cat wpa_supplicant.conf 

ctrl_interface=/var/run/wpa_supplicant
update_config=1

network={
    ssid="Guest"                                   #需要连接的wifi名称，不要是中文名称
    scan_ssid=1
    psk="ingenic_guest"                            #需要连接的wifi密码
    priority=1 
    }
```

​		配置完后重启,默认是开机自启wifi

```
# ifconfig                                                                  #可以看到有wlan0出现

lo        Link encap:Local Loopback  
          inet addr:127.0.0.1  Mask:255.0.0.0
          UP LOOPBACK RUNNING  MTU:65536  Metric:1
          RX packets:5 errors:0 dropped:0 overruns:0 frame:0
          TX packets:5 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1 
          RX bytes:186 (186.0 B)  TX bytes:186 (186.0 B)

wlan0     Link encap:Ethernet  HWaddr 70:3A:2D:14:C6:90                     
          inet addr:12.10.70.118  Bcast:12.10.70.255  Mask:255.255.255.0
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:10 errors:0 dropped:0 overruns:0 frame:0
          TX packets:10 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000 
          RX bytes:10 (10.0 B)  TX bytes:10 (10.0 B)
          
# wifi_
wifi_down.sh  wifi_up.sh                                                     #可以通过wifi_up和wifi_down打开和关闭wifi

# ping www.baidu.com                                                         #ping百度看是否有网络

PING www.baidu.com (14.215.177.38): 56 data bytes
64 bytes from 14.215.177.38: seq=0 ttl=55 time=10.647 ms
64 bytes from 14.215.177.38: seq=1 ttl=55 time=24.899 ms
64 bytes from 14.215.177.38: seq=2 ttl=55 time=36.635 ms
64 bytes from 14.215.177.38: seq=3 ttl=55 time=10.538 ms
64 bytes from 14.215.177.38: seq=4 ttl=55 time=14.128 ms
64 bytes from 14.215.177.38: seq=5 ttl=55 time=11.692 ms
64 bytes from 14.215.177.38: seq=6 ttl=55 time=32.595 ms
^C
--- www.baidu.com ping statistics ---
7 packets transmitted, 7 packets received, 0% packet loss
round-trip min/avg/max = 10.538/20.162/36.635 ms
    
```

### 4 录音和播放

​		本开发板录音采用内部 codec (简称 icodec ) + amic (采样音频模拟信号的传感器),从而,接上 amic 之后,硬件上可以实现录音.

```
# arecord -l                                                                       #查看当前alsa设备列表，当前card 0,device 0

**** List of CAPTURE Hardware Devices ****
card 0: icodecsoundcard [icodec-sound-card], device 0: x2000 icodec pcm internal-codec-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
# arecord -Dhw:0,0 -d 10 -f S16_LE -r 16000 -c 2 -t wav /usr/data/test.wav         #录音      
	-D 指定了录音设备 0,0 指card 0,device 0
	-d 指定录音的时⻓,单位秒
	-f 指定录音格式
	-r 指定了采样率,单位Hz
	-c 指定 channel 个数
	-t 指定生成的文件格式
# aplay -Dplughw:0,0   /usr/data/test.wav                                          #放音

```

​			录音和播放还可以使用cmd_alsa实现:

```
# cmd_alsa record device=hw:0,0 rate=48000 channels=2 time=10 > /usr/data/test.wav  #录音
# cmd_alsa play device=plughw:0,0 rate=48000 channels=2 file=/usr/data/test.wav      #放音
# cmd_alsa list_ctls card=hw:0                                                       #查看音量范围
# cmd_alsa set_ctl card=hw:0 ctl="Master Playback Volume" value=50                   #设置播放音量
```

​       	对应cmd_alsa命令源文件在libhardware2/src/cmds/alsa_main.c
