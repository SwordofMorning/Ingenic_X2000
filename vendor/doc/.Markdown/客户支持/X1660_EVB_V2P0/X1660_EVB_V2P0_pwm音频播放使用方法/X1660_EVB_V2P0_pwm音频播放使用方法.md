### 						            X1660_EVB_V2P0_pwm音频播放使用方法



## 一 硬件环境

X1660_EVB_V2P0， spi nand， 应将j9座子的pin9～10跳冒短接连通。

硬件电路：

<img src="X1660_EVB_V2P0_pwm音频播放使用方法.assets/1.png" alt="1" style="zoom:150%;" />



![2](X1660_EVB_V2P0_pwm音频播放使用方法.assets/2.png)



采用pwm 由上述原理图可知，本硬件设计采用pwm4, 对应主控PC25引脚。功放默认供电，不需额外控制供电。

## 二 软件配置  

软件分为两种模式,一种模拟alsa声卡使用alsa接口播放声音。一种使用pwm接口播放配套demo代码
直接播放声音,可修改性更强,可用于调试。
底层音量调节接口没有实现,需要应用层自行处理。

### pwm 模拟alsa声卡

<img src="X1660_EVB_V2P0_pwm音频播放使用方法.assets/3.png" alt="3" style="zoom:150%;" />

<img src="X1660_EVB_V2P0_pwm音频播放使用方法.assets/4.png" alt="4" style="zoom:150%;" />

<img src="X1660_EVB_V2P0_pwm音频播放使用方法.assets/5.png" alt="5" style="zoom:150%;" />

最后保存配置并退出

<img src="X1660_EVB_V2P0_pwm音频播放使用方法.assets/6.png" alt="6" style="zoom:150%;" />

## 三  编译

```
sxyzhang@T430:~/wj_sz/build$  make x1660_nand_defconfig
sxyzhang@T430:~/wj_sz/build$  make
```

## 四  烧录配置

<img src="X1660_EVB_V2P0_pwm音频播放使用方法.assets/7.png" alt="7" style="zoom:150%;" />

## 五 测试方法

### 1. aplay播放

```
aplay -l
**** List of PLAYBACK Hardware Devices ****
card 0: pwmaudio [pwm_audio], device 0: pwm_audio_pcm [pwm_audio_pcm]
  Subdevices: 1/1
  Subdevice #0: subdevice #0
# 
```

列出当前音频播放设备详情。

```
# aplay -Dplughw:0,0 /usr/data/01_1_20s.wav 
Playing WAVE '/usr/data/01_1_20s.wav' : Signed 16 bit Little Endian, Rate 44100 Hz, Stereo
# 
```

播放音频文件。

###  2. 使用pwm接口直接播放音频

<img src="X1660_EVB_V2P0_pwm音频播放使用方法.assets/10.png" alt="10" style="zoom:150%;" />

勾选以上配置之后，保存并重新按照上述方法编译，就可以使用cmd_pwm_audio命令了。

具体使用命令播放pcm裸数据，在板子的串口或adb输入以下命令，可以听到正常的声音输出：

```
cmd_pwm_audio pc25 48000 /usr/data/mono_s16le_48000.pcm
```

第一个参数 pwm对应的gpio。

第二个参数 pwm的基础频率,基础频率必须是播放音频数据采样率的倍数。

第三个参数 音频数据文件。

### 3. cmd_pwm_audio 对应的源码

libhardware2/src/cmds/pwm_audio_main.c， 客户可以按照这个demo进行应用开发。

## 六  可能遇到的问题

### 1. 指定格式的裸数据如何获取

答：可在pc上输入以下命令生成对应不同的格式裸数据

 ffmpeg -i ~/1.mp3 -ar 8000 -ac 1 -f S16le mono_s16le_8000.pcm   

 ffmpeg -i ~/1.mp3 -ar 48000 -ac 1 -f S16le mono_s16le_48000.pcm

### 2. 加载驱动的时候异常打印

![11](X1660_EVB_V2P0_pwm音频播放使用方法.assets/11.png)

答：如果有这个异常,说明pwm的时钟源不满足audio的基频对齐,需要修改pwm时钟源对齐base freq。

要求pwm_audio的时钟源的频率必须是48k的整数倍。

![12](X1660_EVB_V2P0_pwm音频播放使用方法.assets/12.png)

时钟分频,仅供参考。

pwm驱动代码： module_driver/soc/x1600/pwm/pwm.c （此地址为4.4版本，5.10版本的在其他路径） ---》jz_pwm_init（）。

<img src="X1660_EVB_V2P0_pwm音频播放使用方法.assets/13.png" alt="13" style="zoom:150%;" />

上图说明pwm_audio的时钟源为mpll。

再看uboot中mpll是否为48k的整数倍，如果没有，可以修改时钟源（apll、mpll、epll均可，推荐使用epll）或修改时钟源的频率值从而保证是48k的整数倍。

### 3.播放时出现的爆破音

如果功放一直处于使能状态的话，在播放的开头和结尾都可能产生爆破音。究其原因是功放的电平急剧飙升引起的。

如果想要消除此处的爆破音，应该在播放真正的音频数据之前和之后各自插入一段空数据（全0），在播放此空数据的过程中使能和关闭功放，才能保证功放打开和关闭时没有波形变化，换言之，就没有爆破音形成。

因此，要在硬件上接出功放的使能引脚到gpio上，方便软件控制。本例中，TP23需要连接到gpio上。同时软件上要可调插入的空数据长度。

代码修改：

```
module_driver/drivers/pwm_audio/pwm_audio.c
```

<img src="X1660_EVB_V2P0_pwm音频播放使用方法.assets/14.png" alt="14" style="zoom:150%;" />