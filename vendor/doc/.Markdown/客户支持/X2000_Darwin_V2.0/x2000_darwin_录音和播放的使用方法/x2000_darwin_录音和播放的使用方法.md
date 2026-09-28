# x2000 darwin 录音和播放的使用方法

***写在前面的说明：***

​	1. 本文硬件平台：Darwin_X2000_V2.0



## **一 原理介绍**

### 	1. 录音有两种方式

​	第一，采用dmic硬件采样音频模拟信号并转成数字信号，软件直接读dmic输出的数字信号并编码成指定的文件格式，从而录音产生相应格式的音频文件，比如wav格式。

**注意：**darwin的开发板上留有dmic的设备接口，但是应该是没有默认接这个设备。

​	第二，采用内部codec（简称icodec） +  amic（采样音频模拟信号的传感器），从而，接上amic之后，硬件上可以实现录音。

### 	2. 播放

​	播放统一采用内部codec（简称icodec）这个设备实现。所以，icodec也是要软件配置的。

## 二 具体实现

### 1. iConfigTool中 dmic和icodec的配置流程

![12](x2000_darwin_录音和播放的使用方法.assets/12.png)



![9](x2000_darwin_录音和播放的使用方法.assets/9.png)



![1](x2000_darwin_录音和播放的使用方法.assets/1.png)



![2](x2000_darwin_录音和播放的使用方法.assets/2.png)



![3](x2000_darwin_录音和播放的使用方法.assets/3.png)

本例使用Darwin_X2000_V2.0开发板，相关硬件连接如下可以确定当前dmic使用的是通道2,3

![1662532382618](x2000_darwin_录音和播放的使用方法.assets/1662532382618.png)

最后保存这些设置到对应的配置文件中。依次点击：file ---》 save ---》yes

![4](x2000_darwin_录音和播放的使用方法.assets/4.png)

### 2. 编译系统

**注意：**因make x2000_darwin_v20_nand_defconfig会覆盖buildroot的后续配置，而全部使用buildroot的默认配置：buildroot_x2000_wifi_common_defconfig，所以此处应当先整体编译系统，后续再单独配置buildroot并重新编译。

```
cd build
make x2000_darwin_v20_nand_defconfig
make
```

#### 3. insmod 参数确认

![7](x2000_darwin_录音和播放的使用方法.assets/7.png)

参数：

 	dmic_int值：1--使能 0--失能
    dmic_int0   #控制dmic通道0,1
    dmic_int1   #控制dmic通道2,3
    dmic_int2   #控制dmic通道4,5
    dmic_int3   #控制dmic通道6,7

由上可见，上图3中iConfigTool所选择的音频通道2已经编译生效。

### 4. buildroot 中测试工具的配置

```
cd buildroot/buildroot
make menuconfig
```

![5](x2000_darwin_录音和播放的使用方法.assets/5.png)

最后保存退出就好啦。

### 5. 编译buildroot

```
cd build
 make buildroot
```

### 6. 烧录build/output/rootfs.squashfs。

## 三 录音、播放测试录音和播放的使用方法

### 1. 列出设备

```
# 进⼊串⼝调试界⾯后，切换到/usr/data⽬录，执⾏⾳频录制命令
arecord -l
```

![8](x2000_darwin_录音和播放的使用方法.assets/8.png)

也可以用如下命令来看当前所有alsa设备列表：

![10](x2000_darwin_录音和播放的使用方法.assets/10.png)

​	可知，当前card0为所配置的dmic通道1设备节点名，device0.  可作为录音设备。darwin板默认留了dmic接口座子，但是没有接dmic外设，所以dmic节点是不能工作的，默认是无法用dmic录音的。

​	card1为当前icodec设备节点名，device0. 可作为音频播放设备。当然了，darwin v2.0开发板icodec默认接了amic，所以card1也是可以作为录音设备的。

### 2. 成功获取⾳频输⼊设备列表后进⼀步使用amic + icodec进⾏录⾳操作

```
arecord -Dhw:1,0 -d 10 -f S16_LE -r 16000 -c 2 -t wav /usr/data/test.wav
```

### 3. 使用dmic录音：

```
arecord -Dhw:0,0 -d 10 -f S16_LE -r 16000 -c 2 -t wav /usr/data/test.wav
```

**参数解析**
-D 指定了录⾳设备0,0 是card 0 device 0的意思，本例中是DMIC_PCM
-d 指定录⾳的时⻓，单位时秒         # 录⾳时⻓:10s
-f 指定录⾳格式                   # ⾳频存储格式:S16_LE(有符号16位⼩端存储)
-r 指定了采样率，单位时Hz         # 采样频率:16000hz
-c 指定channel 个数             # 通道数:2
-t 指定⽣成的⽂件格式            # ⽂件格式:wav

### 4. 使用aplay播放这些录音文件

```
aplay -Dplughw:1,0  /usr/data/test.wav
```

此时如果板子接功放就会有声音输出。

### 5.  出错时的定位

 5.1 如果可以录音但是不能正常播放，则使⽤adb调试⼯具在本地环境中拉取刚刚⽣成的test.wav⽂件进⾏试听，检查硬件是否正常⼯作

```
adb pull /usr/data/test.wav ./
```

拉取成功后会如下显⽰：

```
/usr/data/test.wav: 1 file pulled. 5.0 MB/s (640044 bytes in 0.122s)
```

在电脑上建议使用audacity 工具播放看一下数据质量。

## 四 软件demo 接口程序

### 1.  demo接口程序可以使用cmd_alsa 命令，源文件在：

```
sxyzhang@T430:~/my/work/linux/x2000_sz$ vim libhardware2/libhardware2_cmds.mk 
```

![11](x2000_darwin_录音和播放的使用方法.assets/11.png)

### 2. 使用示例

![10](x2000_darwin_录音和播放的使用方法.assets/10.png)

​	可知，当前card0为所配置的dmic通道1设备节点名，device0.  可作为录音设备。darwin板默认留了dmic接口座子，但是没有接dmic外设，所以dmic节点是不能工作的，默认是无法用dmic录音的。

​	card1为当前icodec设备节点名，device0. 可作为音频播放设备。当然了，darwin v2.0开发板icodec默认接了amic，所以card1也是可以作为录音设备的。

​	**使用icodec aimc录音**

```
cmd_alsa record device=hw:1,0 rate=48000 channels=1 time=10 > /tmp/test.wav
```

**参数简单释义：**

record ：实现录音功能。

device=hw:1,0 ：使用card1 的device 0设备，对应上图的：icodecsoundcard。

rate=48000 : 采样率为48k。

channels=1：单通道。

 time=10 ： 录音文件长度为10秒。 

/tmp/test.wav ： 指定最终生成的音频文件。	

将在文件系统中生成：/tmp/test.wav。

![13](x2000_darwin_录音和播放的使用方法.assets/13.png)

**使用icodec + amic进行音频播放**

```
cmd_alsa play device=plughw:1,0 rate=48000 channels=1 file=/tmp/test.wav
```

**参数简单释义：**

play ：实现播放功能。

device=plughw:1,0 ：使用card1 的device 0设备，对应上图的：icodecsoundcard。

rate=48000 : 采样率为48k。

channels=1：单通道。

file=/tmp/test.wav ： 所要播放的音频文件。	

**更多cmd_alsa参数详解可以执行cmd_alsa -h 来查看**

![14](x2000_darwin_录音和播放的使用方法.assets/14.png)



















