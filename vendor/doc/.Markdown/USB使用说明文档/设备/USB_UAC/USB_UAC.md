# USB_UAC

> 注意：使用uac工具时，需要关闭USB的DMA功能
>
> 使用x2000 nand或nor为存储介质时文件路径（arch/mips/boot/dts/ingenic/x2000_module_base.dts）
>
> 使用x2000 mmc为存储介质时文件路径（arch/mips/boot/dts/ingenic/x2000_module_base_mmc0.dts）

![](img/3.png)

## 1. menuconfig 配置流程

```c
Device Drivers  --->
     [*] USB support  --->
            <*>   USB Gadget Support  --->
                  <*>   USB Gadget Drivers
                           (USB functions configurable through configfs)  --->
                   [*]       Audio Class 1.0
```

## 2. uac设备使用流程

在终端执行uac设备配置脚本（文件路径：doc/开发使用说明/usb使用说明文档/设备/USB_UAC/uac_device_config.sh）。

> 命令：./uac_device_config.sh start

### 2.1. 设备端播放，主机端录音

主机端接入USB设备后，打开电脑设备管理器 -> 声音,视频控制器，查询是否产生新插入的音频设备，若存在则点击电脑桌面喇叭图案右键，设置录音设备为插入的USB音频设备。

![](img/1.png)

设备端执行aplay命令进行播放

```c
使用aplay命令查看声卡设备
# aplay -l
**** List of PLAYBACK Hardware Devices ****
card 1: icodecsoundcard [icodec-sound-card], device 0: x2000 icodec pcm internal-codec-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 2: UAC1Gadget [UAC1_Gadget], device 0: UAC1_PCM [UAC1_PCM]
  Subdevices: 1/1
  Subdevice #0: subdevice #0

选择USB声卡设备card 2，进行播放test.wav音频文件
# aplay -Dhw:2,0 test.wav
```

### 2.2. 设备端录音，主机端播放

pc端接入USB设备后，打开电脑设备管理器 -> 声音,视频控制器，查询是否产生新插入的音频设备，若存在则点击电脑桌面喇叭图案右键，设置播放设备为插入的USB音频设备。

![](img/2.png)

linux 端执行arecord命令进行录音

```c
使用arecord命令查看声卡设备
# arecord -l
**** List of CAPTURE Hardware Devices ****
card 0: DMICPCM [DMIC_PCM], device 0: DMIC PCM [DMIC PCM]
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 1: icodecsoundcard [icodec-sound-card], device 0: x2000 icodec pcm internal-codec-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 2: UAC1Gadget [UAC1_Gadget], device 0: UAC1_PCM [UAC1_PCM]
  Subdevices: 1/1
  Subdevice #0: subdevice #0

选择USB声卡设备card 2, 进行录制test.wav音频文件
# time arecord -Dhw:2,0 -d 10 -f S16_LE -r 48000 -c 2 -t wav test.wav

```

