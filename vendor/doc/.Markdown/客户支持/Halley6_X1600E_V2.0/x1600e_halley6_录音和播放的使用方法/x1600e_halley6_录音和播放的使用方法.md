## 						x1600e_halley6录音和播放的使用方法

****

***写在前面的说明：***

​	1. 本文硬件平台：HALLEY6_BASEBOARD_V2.0

## **一 原理介绍**

​	x1600和x1660系列因为cpu内部没有封装codec功能，所以录音和播放实现方式与x2000系列不同。本开发板硬件设计上I2S和以太网MAC是复用的主控PB22-28,通过跳冒CON6  Analog Switch来切换具体的功能属性。当CON6悬空，则主控PB22-28为MAC功能。当CON6连接，则主控PB22-28为I2S功能。

**硬件设计详细说明见**：Halley6开发套件硬件手册_V2.1.pdf

**硬件完整参考设计见**：halley6_baseboard_v2.0.pdf    halley6_coreboard_v2.0.pdf

### 	1. 录音

​	采用line-in输入模拟信号给ADC模块，经过ADC转成数字信号后给到主控I2S来编码成指定的文件格式，从而录音产生相应格式的音频文件，比如wav格式。

### 	2. 播放有两种方式

​	2.1  采用I2S 输出数字信号给DAC模块，经过DAC转成模拟信号后给到功放SPK。

​	2.2 采用主控pwm 载波播放PCM文件。

​	本硬件板halley6硬件设计上采用第一种方法，第二种就需要飞线，或者采用x1660_EVB_V2p0开发板也可以实现第二种方法的pwm播放PCM文件。

## 二 具体实现

### 1. iConfigTool中alsa及aic的配置流程

<img src="x1600e_halley6_录音和播放的使用方法.assets/1.png" alt="1" style="zoom:150%;" />

<img src="x1600e_halley6_录音和播放的使用方法.assets/2.png" alt="2" style="zoom:150%;" />

<img src="x1600e_halley6_录音和播放的使用方法.assets/3.png" alt="3" style="zoom:150%;" />

<img src="x1600e_halley6_录音和播放的使用方法.assets/4.png" alt="4" style="zoom:150%;" />

<img src="x1600e_halley6_录音和播放的使用方法.assets/5.png" alt="5" style="zoom:150%;" />

最后保存这些设置到对应的配置文件中。依次点击：file ---》 save ---》yes

<img src="x1600e_halley6_录音和播放的使用方法.assets/6.png" alt="6" style="zoom:150%;" />

### 2. 编译系统

**注意：**因make x1600e_halley6_nand_defconfig会覆盖buildroot的后续配置，而全部使用buildroot的默认配置：buildroot_wifi_common_defconfig，所以此处应当先整体编译系统，后续再单独配置buildroot并重新编译。

```
cd build
make x1600e_halley6_nand_defconfig
make
```

### 3. buildroot 中测试工具的配置

```
cd buildroot/buildroot
make menuconfig
```

<img src="x1600e_halley6_录音和播放的使用方法.assets/7.png" alt="7" style="zoom:150%;" />

最后保存退出就好啦。

### 4. 编译buildroot

```
cd build
 make buildroot
```

### 5. 烧录build/output/rootfs.squashfs。

## 三 录音、播放测试

### 1. 列出设备

```
# 进⼊串⼝调试界⾯后，切换到/usr/data⽬录，执⾏⾳频录制命令
arecord -l
```

<img src="x1600e_halley6_录音和播放的使用方法.assets/8.png" alt="8" style="zoom:150%;" />

​	可知，当前card0为所配置的虚拟声卡设备节点名，device0.  可作为录音和播放设备。line-in输入音频模拟信号就可以实现录音了。

### 2. 使用arecord进⾏录⾳操作

```
arecord -Dhw:0,0 -f S16_LE -r 44100 -c 2 -t wav -d 10 /tmp/test.wav
```

**注意**，此时应将line-in线连接pc和halley6开发板，pc上播放对应相同格式、采样率、通道数的音频文件。

**此处使用的命令注解为**：在pc上播放S16_LE数据格式，采样率为44100,双通道的wav格式文件，并且将录到的音频数据保存在/tmp/test.wav文件。

### 3. 使用aplay播放

```
aplay -Dplughw:0,0 -f S16_LE -r 44100 /tmp/test.wav
```

此时halley6开发板应该就会有对应的声音输出。

**参数解析**：

​	 -D 指定了录⾳设备0,0 是card 0 device 0的意思，本例中是虚拟声卡

​	-d 指定录⾳的时⻓，单位时秒         # 录⾳时⻓:10s

​	-f 指定录⾳格式                   # ⾳频存储格式:S16_LE(有符号16位⼩端存储)

​	-r 指定了采样率，单位时Hz         # 采样频率:16000hz

​	-c 指定channel 个数             # 通道数:2

​	-t 指定⽣成的⽂件格式            # ⽂件格式:wav

### 4.  出错时的定位

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
sxyzhang@T430:~/my/work/linux/wj_sz$ vim libhardware2/libhardware2_cmds.mk 
```

<img src="x1600e_halley6_录音和播放的使用方法.assets/9.png" alt="9" style="zoom:150%;" />

### 2. 使用示例

​	**使用cmd_alsa录音**

```
cmd_alsa record device=hw:0,0 rate=44100 channels=2 time=10 > /tmp/test.wav
```

**参数简单释义：**

record ：实现录音功能。

device=hw:0,0 ：使用card0 的device 0设备，对应虚拟声卡。

rate=44100 : 采样率。

channels=2：双通道。

 time=10 ： 录音文件长度为10秒。 

/tmp/test.wav ： 指定最终生成的音频文件。	

将在文件系统中生成：/tmp/test.wav。

<img src="x1600e_halley6_录音和播放的使用方法.assets/10.png" alt="10" style="zoom:150%;" />

**使用cmd_alsa进行音频播放**

```
cmd_alsa play device=plughw:0,0 rate=44100 channels=2 file=/tmp/test.wav
```

**参数简单释义：**

play ：实现播放功能。

device=plughw:0,0 ：使用card0 的device 0设备，对应虚拟声卡。

rate=44100 : 采样率为44100。

channels=2：双通道。

file=/tmp/test.wav ： 所要播放的音频文件。	

**更多cmd_alsa参数详解可以执行cmd_alsa -h 来查看**

<img src="x1600e_halley6_录音和播放的使用方法.assets/11.png" alt="11" style="zoom:150%;" />