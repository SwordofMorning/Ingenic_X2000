# 录音和内部Codec的播放

## **一 原理介绍**

### 	1.1 录音有两种方式

​	第一，采用dmic硬件采样音频模拟信号并转成数字信号，软件直接读dmic输出的数字信号并编码成指定的文件格式，从而录音产生相应格式的音频文件，比如wav格式。

​	第二，采用内部codec（简称icodec） +  amic（采样音频模拟信号的传感器），从而，接上amic之后，硬件上可以实现录音。

### 	1.2  播放

​	播放统一采用内部codec（简称icodec）这个设备实现。所以，icodec也是要软件配置的。

## 二 具体实现

### 2.1  iConfigTool中 dmic和icodec的配置流程

![2023-11-24_14-47](录音和内部CODEC的播放说明文档.assets/01.png)

选择主配置文件：x2600e_vast_v20_nand_5.10_factory_defconfig



![2023-11-24_15-44](录音和内部CODEC的播放说明文档.assets/02.png)

选中 x2600_510 icodec(内部codec)创建一个声卡



![2023-11-24_15-47](录音和内部CODEC的播放说明文档.assets/03.png)

勾选dmic(外接数字mic)

![2023-11-24_16-02](录音和内部CODEC的播放说明文档.assets/04.png)

dmic 接口提供四个引脚，对应控制器8个通道，这里勾选使能dmic通道0,1，具体选那个通道，要看硬件连接情况而定。



本例使用PD_X2600E_VAST_V2.0开发板，相关硬件连接如下可以确定当前dmic使用的是通道0,1

![2023-11-24_16-07](录音和内部CODEC的播放说明文档.assets/05.png)



最后保存这些设置到对应的配置文件中。依次点击：file ---》 save ---》yes



![2023-11-24_16-08](录音和内部CODEC的播放说明文档.assets/06.png)



点击Yes进行配置保存。



### 2.2  编译和烧录

**注意：**因make x2600e_vast_v20_nand_5.10_factory_defconfig  会覆盖buildroot的后续配置，而全部使用buildroot的默认配置：buildroot_x2600_510_wifi_common_defconfig，所以此处应当先整体编译系统，后续再单独配置buildroot并重新编译。

```
cd build
make x2600e_vast_v20_nand_5.10_factory_defconfig
make
```

### 2.3  insmod 参数确认

```c
kenny@kenny-computer:~/source/source_darwin_linux_x2000/build$ cat ../buildroot/buildroot/output/target/module_driver/soc_dmic.sh 
insmod soc_dmic.ko dmic_int0=1 dmic_int1=0
```

参数：

 	dmic_int值：1--使能 0--失能
 	dmic_int0   #控制dmic通道0,1  
 	dmic_int1   #控制dmic通道2,3
 	dmic_int2   #控制dmic通道4,5
 	dmic_int3   #控制dmic通道6,7

由上可见，上图中iConfigTool所选择的音频通道0已经编译生效。



### 2.4  buildroot 中测试工具的配置

```
cd buildroot/buildroot
make menuconfig
```

![5](录音和内部CODEC的播放说明文档.assets/07.png)

最后保存退出就好啦。

确定当前buildroot的配置：

```c
kenny@kenny-computer:~/source/source_darwin_linux_x2000/build$ grep -nr "buildroot" configs/x2600e_vast_v20_nand_5.10_factory_defconfig
9:APP_br_dir=../buildroot/buildroot
10:APP_br_config_file=configs/buildroot/buildroot_x2600_510_wifi_common_defconfig
438:# APP_br_root_login_keep_buildroot is not set
```

所以当前的buildroot配置为:  buildroot_x2600_510_wifi_common_defconfig

上面保存以后，执行如下操作保存buildroot配置

```c
buildroot/buildroot$ cp .config ../../build/configs/buildroot_x2600_510_wifi_common_defconfig
```



### 2.5  编译buildroot

```
cd build
make buildroot
```

### 2.6  烧录build/output/rootfs.squashfs

烧录可以参考， [烧录方法](../01GettingStarted/05软件编译和烧录.md) 



## 三 录音、播放测试录音和播放的使用方法

### 3.1 列出设备

```
# 进⼊串⼝调试界⾯后，切换到/usr/data⽬录，执⾏⾳频录制命令
arecord -l
```

```shell
# arecord -l

**** List of CAPTURE Hardware Devices ****
card 0: dmicsoundcard [dmic-sound-card], device 0: x2600 DMIC snd-soc-dummy-dai-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 1: icodecsoundcard [icodec-sound-card], device 0: x2600 icodec pcm ingenic-icodec-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
# 
```

也可以用如下命令来看当前所有alsa设备列表：

```shell
# cat /proc/asound/cards 

 0 [dmicsoundcard  ]: dmic-sound-card - dmic-sound-card
                      dmic-sound-card
 1 [icodecsoundcard]: icodec-sound-ca - icodec-sound-card
                      icodec-sound-card
# 
```

​	可知，当前card0为所配置的dmic设备节点名，device0 可作为录⾳设备。本开发板默认留了 dmic接⼝座⼦，也接了dmic外设，所以可以⽤dmic录⾳的。 card1为当前内部icodec设备节点名，device0 可作为⾳频播放设备。当然了，本开发板icodec 默认不接amic，只有播放时的amic回采，所以card1不可以作为录⾳设备，但是可以做为播放设备。

### 3.2  使用dmic录音

```
arecord -Dhw:0,0 -d 10 -f S16_LE -r 16000 -c 2 -t wav /usr/data/test.wav
```

**参数解析**

-D 指定了录⾳设备0,0 是card 0 device 0的意思，本例中是DMIC

-d 指定录⾳的时⻓，单位时秒         # 录⾳时⻓:10s

-f 指定录⾳格式                   # ⾳频存储格式:S16_LE(有符号16位⼩端存储)

-r 指定了采样率，单位时Hz         # 采样频率:16000hz

-c 指定channel 个数             # 通道数:2

-t 指定⽣成的⽂件格式            # ⽂件格式:wav

### 3.3  使用aplay播放这些录音文件

```
aplay  -Dplughw:1,0  /usr/data/test.wav
```

此时如果板子接功放就会有声音输出。

### 3.4  出错时的定位

如果可以录音但是不能正常播放，则使⽤adb调试⼯具在本地环境中拉取刚刚⽣成的test.wav⽂件进⾏试听，检查硬件是否正常⼯作

```
adb pull /usr/data/test.wav ./
```

拉取成功后会如下显⽰：

```
/usr/data/test.wav: 1 file pulled. 5.0 MB/s (640044 bytes in 0.122s)
```

在电脑上建议使用audacity 工具播放看一下数据质量。

## 四 软件demo 接口程序

### 4.1  demo接口程序可以使用cmd_alsa 命令

cmd_alsa 是君正平台自主研发的用于调试音频的重要工具，客户可以参考其源码进行上层应用的开发。 具体位置如下：

```c
libhardware2/src/cmds/alsa_main.c
```

##### 4.1.1 DMIC 录音

```text
cmd_alsa record device=hw:0,0 rate=48000 channels=1 time=10 > /tmp/test.wav
```

**参数简单释义：**

record ：实现录⾳功能。

device=hw:0,0 ：使⽤card0 的device 0设备，DMIC。

rate=48000 : 采样率为48k。

channels=1：单通道。

time=10 ： 录⾳⽂件⻓度为10秒。 

/tmp/test.wav ： 指定最终⽣成的⾳频⽂件。 

将在⽂件系统中⽣成：/tmp/test.wav。

##### 4.1.2 ICODEC + SPK播放

```c
cmd_alsa play device=plughw:1,0 rate=48000 channels=1 file=/tmp/test.wav
```

**参数简单释义：**

play ：实现播放功能。

device=plughw:1,0 ：使⽤card1 的device 0设备，对应icodecsoundcard。

rate=48000 : 采样率为48k。

channels=1：单通道。

file=/tmp/test.wav ： 所要播放的⾳频⽂件。 
