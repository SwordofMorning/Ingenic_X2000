# 					x2600_halley7_v1.0快速上手文档

## 一 硬件介绍

本开发板采用x2600e + sfc nand

供电：需额外供电5V3A

串口输出：uart0, PE09～10, 波特率：3000000   

一根Type-C线用作烧录, 一根用作串口输出

![8](RD_X2600_HALLEY7_BASEBOARD_V1.0_开发板快速上手说明文档.assets/1.png)

![9](RD_X2600_HALLEY7_BASEBOARD_V1.0_开发板快速上手说明文档.assets/2.png)

具体接口如下：

![10](RD_X2600_HALLEY7_BASEBOARD_V1.0_开发板快速上手说明文档.assets/3.png)

## 二  获取源码方法

在获取源码之前，**都需要把你开发电脑上的相应的 public key发送给我司进行添加**，从而得到获取源码的权限。本地电脑获取 public key的方法如下： 

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
：

```
$ mkdir project_workspace
$ cd project_workspace
$ wget http://git.ingenic.com.cn:8082/bj/repo
$ chmod +x repo
```

同步代码的地址如下： 

```
./repo init -u ssh://sz_halley2@119.136.25.25:29418/mirror/linux/manifest
./repo sync
```

如若报错可使用http同步方式：

```
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

本开发板使用的工程配置文件为：x2600e_halley7_v1.0_factory_defconfig

如果需要更改默认配置，可以使用工具 IConfigTool 进行配置，详细使用方法见文档：

`doc/开发使用说明/IConfigTool 使用文档.pdf`



整体编译：

```
bhu@bhu-PC:~/work/build$ make clean            # 第一次编译需要，后续编译都不需要执行整体clean操作
bhu@bhu-PC:~/work/build$ make x2600e_halley7_v1.0_factory_defconfig
bhu@bhu-PC:~/work/build$ make 
```

编译后生成固件如下：

```c
bhu@bhu-PC:~/work/build/output$ ls -lh
总用量 16M
-rw-r--r-- 1 bhu bhu  12M 7月  18 15:37 rootfs.squashfs
-rw-r--r-- 1 bhu bhu  24K 7月  18 15:37 u-boot-spl-pad.bin
-rw-r--r-- 1 bhu bhu 4.4M 7月  18 15:37 xImage
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

**x2600e的烧录配置仅在烧录工具2.5.36及以后的版本中有添加,**请务必使用对应版本的烧录工具来烧录。具体烧录配置如下：

![1](RD_X2600_HALLEY7_BASEBOARD_V1.0_开发板快速上手说明文档.assets/4.png)

![2](RD_X2600_HALLEY7_BASEBOARD_V1.0_开发板快速上手说明文档.assets/5.png)

![3](RD_X2600_HALLEY7_BASEBOARD_V1.0_开发板快速上手说明文档.assets/6.png)

## 六 基本功能测试

### 6.1  LCD显示

```c
cmd_fb info /dev/fb0                        #展示当前fb0的硬件信息
cmd_fb enable /dev/fb0                      #使能屏幕
cmd_fb clear /dev/fb0 color=0xff00ff00      #清屏为绿⾊
cmd_fb draw_rect /dev/fb0 color=0xff0000ff frame_index=0 x=0 y=0 width=400 hei
ght=200                                     #绘制一个蓝色小色块
cmd_fb display /dev/fb0                     #显⾊
cmd_fb disable /dev/fb0                     #失能屏幕
```

开发板会默认运行lvgl demo，若不想运行可以取消

先打开IConfigTool

```c
bhu@bhu-PC:~/work/tools/iconfigtool/IConfigToolApp$ ./IConfigTool 
```

![4](RD_X2600_HALLEY7_BASEBOARD_V1.0_开发板快速上手说明文档.assets/7.png)

取消勾选开机运行lvgl

![5](RD_X2600_HALLEY7_BASEBOARD_V1.0_开发板快速上手说明文档.assets/8.png)

取消勾选lvgl平台支持

![6](RD_X2600_HALLEY7_BASEBOARD_V1.0_开发板快速上手说明文档.assets/9.png)

保存配置

![7](RD_X2600_HALLEY7_BASEBOARD_V1.0_开发板快速上手说明文档.assets/10.png)

重新编译配置

```c
bhu@bhu-PC:~/work/build$ make x2600e_halley7_v1.0_factory_defconfig

bhu@bhu-PC:~/work/build$ make 
```

### 6.2 SPK播放

```c
# aplay -l                                                                                     //当前播放设备详情

**** List of PLAYBACK Hardware Devices ****
card 0: icodecsoundcard [icodec-sound-card], device 0: x2600 icodec pcm ingenic-icodec-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0

# aplay -Dplughw:0,0 /usr/data/audio_pcm.aiff 

Playing WAVE '/usr/data/audio_pcm.aiff' : Signed 16 bit Little Endian, Rate 16000 Hz, Stereo   //播放音频文件
```

### 6.3 AMIC录音

```c
# arecord -l 

**** List of CAPTURE Hardware Devices ****
card 0: icodecsoundcard [icodec-sound-card], device 0: x2600 icodec pcm ingenic-icodec-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0

# arecord -Dhw:0,0 -d 10 -f S16_LE -r 16000 -c 2 -t wav /usr/data/test.wav                     //amic录音
Recording WAVE '/usr/data/test.wav' : Signed 16 bit Little Endian, Rate 16000 Hz, Stereo
```

### 6.4 Ethernet

```c
# ifconfig eth0 10.4.3.202 netmask 255.255.255.0 up               //网络设备开启，配置ip地址及子网掩码
# route add default gw 10.4.3.1                                   //配置网关地址
# echo nameserver 8.8.8.8  > /tmp/resolv.conf                     //添加域名解析
# cat /tmp/resolv.conf 
nameserver 8.8.8.8
# ping www.baidu.com                                              //网络测试
PING www.baidu.com (14.119.104.254): 56 data bytes
64 bytes from 14.119.104.254: seq=0 ttl=55 time=7.531 ms
64 bytes from 14.119.104.254: seq=1 ttl=55 time=7.573 ms
# ifconfig eth0 down                                              //网络设备关闭
```
