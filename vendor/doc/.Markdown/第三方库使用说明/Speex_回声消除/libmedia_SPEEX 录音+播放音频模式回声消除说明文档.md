# libmedia_SPEEX 录音+播放音频模式回声消除说明文档

本文档将说明如何使用第三方库 [speex](https://www.speex.org/) 在x2600_vast_v2.0上 播放音频文件并使用amic录音进行录音消回声(aec)，其他平台可供参考。



[TOC]

<div STYLE="page-break-after: always;"></div>

## 一、配置



### 1. 勾选第三方库

![第三方库勾选](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/%E7%AC%AC%E4%B8%89%E6%96%B9%E5%BA%93%E5%8B%BE%E9%80%89.png)

### 2. 勾选 libmedia 中相应的算法功能以及测试用例

![libmedia配置](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/libmedia%E9%85%8D%E7%BD%AE.png)



speex amic录音与音频文件的回声消除测试 对应代码在工程目录中的 `libmedia/test/main_aec_test.c`

功能是 打开播放设备与录音设备，将播放音频送入算法进行播放，同时算法录制音频并将播放数据与录制音频进行回声消除，输出回声消除的音频帧。

### 3. 播放与录音功能

声卡驱动

![内部声卡驱动](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/%E5%86%85%E9%83%A8%E5%A3%B0%E5%8D%A1%E9%A9%B1%E5%8A%A8.png)



创建声卡设备

![声卡设备](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/%E5%A3%B0%E5%8D%A1%E8%AE%BE%E5%A4%87.png)



使用amic采集外界音频

![amic录制的aec](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/%E6%B6%88%E5%9B%9E%E5%A3%B0%E9%85%8D%E7%BD%AE.png)



重点参数解释

开启偏值电压：外接amic时用于给麦克风供电 不同型号的麦克风需要不同的偏值电压值（设置偏值电压的寄存器数值）

mic信号的增益(MICL_GAIN)：外接amic时需要依据信号强弱设置改增益大小

手动设置ALCL增益(ALCL_GAIN)：alcl为codec的adc的数字增益，用于调节录制音频的增益大小。

声卡输出的模拟增益(HPOUTL)：用于调节播放音频的增益大小。



当成功配置icodec正常来说会出现以下节点

```
# ls /dev/snd/
controlC0   pcmC0D0c   pcmC0D0p   timer
# 
# 
# arecord -l
**** List of CAPTURE Hardware Devices ****
card 0: icodecsoundcard [icodec-sound-card], device 0: x2600 icodec pcm ingenic-icodec-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
# 
# 
# aplay -l
**** List of PLAYBACK Hardware Devices ****
card 1: icodecsoundcard [icodec-sound-card], device 0: x2600 icodec pcm ingenic-icodec-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
# 
```

可以看到pcmC0D0c对应amic设备，pcmC0D0p为播放设备

<div STYLE="page-break-after: always;"></div>

## 二、使用

```shell
aec_test        # 请结合代码查看，播放固定路径下的音频文件、开始录制以及开始算法处理，等待文件播放完成即可得到消除回声后的音频文件

ls /tmp/

aec.pcm          # 回声消除后的音频文件

```

回声消除效果

![aec_test效果](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/amic%E5%BD%95%E5%88%B6%E7%9A%84%E6%B6%88%E5%9B%9E%E5%A3%B0%E6%95%88%E6%9E%9C.png)

<div STYLE="page-break-after: always;"></div>

## 三、算法软硬件指南

算法需要满足

1. 给算法的音频帧的尽量保证在幅值的60%到80%之间。
2. 录制的、播放的音频均不能消顶。
3. 给算法的音频帧的时间轴需要尽可能的同步，且对于算法而言的播放数据始终先于录音数据。

软件参数以及硬件设计需要匹配，本节将介绍对于算法而言最佳的硬件设计以及如何设置软件参数，根据不同的使用场景可以按照本文档的设计思路进行调整。

### 调试手段

1. **查看输入给算法的音频数据**

使能宏 **DUMP_ECHO_CANCEL_DATA**

位置在工程目录的 `third_party/speexdsp/src/config.h`

```
#define DUMP_ECHO_CANCEL_DATA
```

指定音频数据文件保存的位置，`third_party/speexdsp/src/mdf.c` 如下所示，这里保存到tmp目录下

![dump音频](libmedia_SPEEX%20%E5%BD%95%E9%9F%B3+%E6%92%AD%E6%94%BE%E9%9F%B3%E9%A2%91%E6%A8%A1%E5%BC%8F%E5%9B%9E%E5%A3%B0%E6%B6%88%E9%99%A4%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.assets/dump%E9%9F%B3%E9%A2%91%E8%B0%83%E8%AF%95%E6%96%87%E4%BB%B6.png)

通过 adb 推送到电脑上，推荐通过 **[Audacity](https://www.audacityteam.org/)** 软件打开音频文件查看。

2. **修改算法相关参数**

```c
// 算法参数
struct speex_params {
    // 算法处理单元的样本数，也是算法输出帧的样本数，帧的其余参数与录制音频帧的参数一致
    int aec_samples;
    // 为播放与录音音频对齐，录音录过align_ms(与播放音频对齐)后才传入算法
    int capture_align_ms;
    // 算法对于播放与录音音频不对齐的帧数的容忍程度
    // 直接影响算法产生消除回声后第一帧的快慢
    // 单位为算法处理单元的样本数aec_samples
    int filter_count;

    // 在喇叭静音时，录制设备的音量大小
    int mic_volume_m;
    // 喇叭播放时，录音设备的音量大小（一般设置的比前者小，防止录到的喇叭声过大影响录制效果）
    int mic_volume_p;
    int spk_volume;
};
```

3. **修改声卡的其他增益**

内部声卡的示意图（依据平台不同会有不同，注意在pm手册中查看区别，以x2600为例）

![image-20231109102206373](libmedia_SPEEX%20%E5%BD%95%E9%9F%B3+%E6%92%AD%E6%94%BE%E9%9F%B3%E9%A2%91%E6%A8%A1%E5%BC%8F%E5%9B%9E%E5%A3%B0%E6%B6%88%E9%99%A4%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.assets/icodec%E5%A2%9E%E7%9B%8A%E7%A4%BA%E6%84%8F%E5%9B%BE.png)

可以看到可以调节的增益包括MICL ALCL HPOUTL   ADCL DACL

其中 MICPL MICNL 的最大输入电压为1.8v

其中 HPOUTPL HPOUTNL 的最大输出电压为1.8v



### 硬件设计标准

#### 对于播放而言，当软件增益为0db的时候，播放满幅的1khz正弦波，喇叭输出端不消顶且电压也接近codec的满幅值。



步骤一、准备满幅的1k正弦波音频文件

推荐使用Audacity进行生成 在Generate中选择Chirp，选择波形、频率、持续时间等基础设置后点击OK生成正弦波

![生成满幅波形](libmedia_SPEEX%20%E5%BD%95%E9%9F%B3+%E6%92%AD%E6%94%BE%E9%9F%B3%E9%A2%91%E6%A8%A1%E5%BC%8F%E5%9B%9E%E5%A3%B0%E6%B6%88%E9%99%A4%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.assets/%E7%94%9F%E6%88%90%E6%BB%A1%E5%B9%85%E6%B3%A2%E5%BD%A2.png)

放大后查看生成的波形如图：

![满幅波形](libmedia_SPEEX%20%E5%BD%95%E9%9F%B3+%E6%92%AD%E6%94%BE%E9%9F%B3%E9%A2%91%E6%A8%A1%E5%BC%8F%E5%9B%9E%E5%A3%B0%E6%B6%88%E9%99%A4%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.assets/%E6%BB%A1%E5%B9%85%E6%B3%A2%E5%BD%A2.png)

步骤二、获取codec输出端的最大音量且不消顶的软件配置

设置软件增益为0db：

依照内部声卡的示意图，可以设置的播放的增益有DACL与HPOUTL（DACL是数字增益，HPOUTL是模拟增益）

通常来讲，它们两者的0db可以通过pm手册的声卡部分获取，但配置后我们仍需要通过播放满幅音频来确定是否真的是0增益，具体方法如下：

1. 按照手册设置0增益后，播放满幅的1k正弦波音频，通过示波器测量HPOUTPL与HPOUTNL端（未经过功放电路的端点），如果出现消顶现象 或 底端值与顶端值不满1.8v，此时应优先确定手册是否匹配或是否有更新。

消顶：

![codec消顶](libmedia_SPEEX%20%E5%BD%95%E9%9F%B3+%E6%92%AD%E6%94%BE%E9%9F%B3%E9%A2%91%E6%A8%A1%E5%BC%8F%E5%9B%9E%E5%A3%B0%E6%B6%88%E9%99%A4%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.assets/codec%E6%B6%88%E9%A1%B6%E7%8E%B0%E8%B1%A1.png)

不满1.8V：

![不满1.8](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/RigolDS5.png)

2. 如遇上述情况可以通过减小（针对消顶现象）增大（针对不满现象）DACL或HPOUTL增益进行调整，优先推荐调整增益HPOUTL这个模拟增益，因为在x2600平台，DACL增益是作为声音控制的操作对象，用于控制播放音量的大小。

调整完后的波形下图：

![捕获正常](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/%E6%8D%95%E8%8E%B7%E6%AD%A3%E5%B8%B8.png)

**（注：波形图均为由差分探头测量出的波形，单端探头也可以进行测量，但请留意此时的电压范围会有所不同）**



步骤三、寻找功放的最大音量且音频不消顶的硬件配置

经过步骤二，已经能够获取到codec能输出的最大音频，在此前提下，调整功放电路的硬件增益来获取功放的最大音量且音频不消顶的硬件配置。

以 x2600 为例

**功放电路**

![功放电路](libmedia_SPEEX%20%E5%BD%95%E9%9F%B3+%E6%92%AD%E6%94%BE%E9%9F%B3%E9%A2%91%E6%A8%A1%E5%BC%8F%E5%9B%9E%E5%A3%B0%E6%B6%88%E9%99%A4%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.assets/%E5%8A%9F%E6%94%BE%E7%94%B5%E8%B7%AF.png)

功放电路的最大输出电压取决于输入的电压大小。可以看到输入电压为5V(VSYS)，所以对于这个功放电路，输出到SPK的峰峰值为10V，硬件电路的调整目标就是经过功放电路输出到SPK端的波形不消顶且峰峰值接近10V。

依据功放芯片TPA6211A1的说明，调节R76与R79就可以调节增益的大小
$$
Gain=\frac{R_F(内部电阻40Ω)}{R_I(R76与R79)}
$$
如上图目前的增益为4 V/V （四倍电压增益）

估算此时HPOUTP与HPOUTN端经过这个增益1.8V -> 7.2V是超过输入电压的，因此SPK端会消顶，如图：

![消顶现象](libmedia_SPEEX%20%E5%BD%95%E9%9F%B3+%E6%92%AD%E6%94%BE%E9%9F%B3%E9%A2%91%E6%A8%A1%E5%BC%8F%E5%9B%9E%E5%A3%B0%E6%B6%88%E9%99%A4%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.assets/%E6%B6%88%E9%A1%B6%E7%8E%B0%E8%B1%A1.png)

为了使SPK端不消顶，可以更换R76与R79电阻为15k，这样增益就变成了2.667 V/V

再次测量喇叭端：

![RigolDS3](libmedia_SPEEX%20%E5%BD%95%E9%9F%B3+%E6%92%AD%E6%94%BE%E9%9F%B3%E9%A2%91%E6%A8%A1%E5%BC%8F%E5%9B%9E%E5%A3%B0%E6%B6%88%E9%99%A4%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.assets/%E5%96%87%E5%8F%AD%E7%AB%AF.png)

可以看到已经不消顶，电压范围也接近预期值。



经过上述的硬件调整，我们已经获得到了不消顶情况下SPK端最大的输出电压，也就是最大音量，在实际播放音频时（一般不会是满幅音频），如下图

![正常音频](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/%E6%AD%A3%E5%B8%B8%E9%9F%B3%E9%A2%91.png)

如果觉得播放出来的声音不够大，可以适当增大软件增益，比如这里增大6db（使音频最大幅值满幅），实际播放的音频会是如下：

![放大音频](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/%E5%A2%9E%E7%9B%8A%E9%9F%B3%E9%A2%91.png)

会有小部分消顶，但也基本能满足算法要求，这时如果仍对播放的音量不满意，就需要通过更换喇叭、对功放电路的输入端进行升压来解决，再进行软件或硬件增益消顶现象就会严重影响算法效果了。



### 软件增益适配硬件增益

通过上述的硬件设计标准，我们可以获得一个最佳的满足算法的电路，但实际上我们也可以通过更改软件上的可调增益来适配硬件

**但这样做的信噪比可能会增大，如下述例子中 放大音频的增益设置，实际上是硬件分压较小，导致需要用软件进行放大，因此优先推荐修改电路，在单级的增益中达到我们需要的音频效果**

具体的情况以及增益设置：

#### SPK端的消顶现象：

![消顶现象](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/%E6%B6%88%E9%A1%B6%E7%8E%B0%E8%B1%A1.png)

可以通过软件设置XXX_volume减小3db (226->220)

```
cmd_alsa set_ctl card=hw:1 ctl="Master Playback Volume" value=220
```

效果：

![更改后的效果](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/%E9%81%BF%E5%85%8D%E6%B6%88%E9%A1%B6%E7%8E%B0%E8%B1%A1.png)





#### 采集到的音频幅值小

可以通过软件(XXX_volume)设置增益大小增加29.5db达到下面的音频效果。（196->254）

```
cmd_alsa set_ctl card=hw:1 ctl="Mic Volume" value=254
```

![软件音频放大](libmedia_SPEEX 录音+播放音频模式回声消除说明文档.assets/%E8%BD%AF%E4%BB%B6%E9%9F%B3%E9%A2%91%E6%94%BE%E5%A4%A7.png)

或更改驱动参数ALCL增益从6增加到25（增加28.5db）或 MICL 增益。



*推荐先更改XXX_volume确定需要调节的增益db，之后依据测量到需要调节的db设置MICL增益与ALCL增益*



### 软件调试

经过上述的硬件或软件增益适配，我们已经可以得到功放的最大播放效果，接下来就是对于算法而言的软件调试。

#### 目标一：音频帧同步

默认会计算出一个延迟值，如果在使用的过程默认值不能满足对齐要求，可以通过手动设置算法的参数capture_align_ms来对齐给算法的播放数据与录音数据。

查看算法dump出的音频可以看到：

![对齐](libmedia_SPEEX%20%E5%BD%95%E9%9F%B3+%E6%92%AD%E6%94%BE%E9%9F%B3%E9%A2%91%E6%A8%A1%E5%BC%8F%E5%9B%9E%E5%A3%B0%E6%B6%88%E9%99%A4%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.assets/%E5%AF%B9%E9%BD%90.png)

出现这样的原因是由于amic先开始录制，且录制为阻塞等待导致播放较晚，如果原本填写的参数为0，那么更改算法参数`.speex_params.capture_align_ms = 16,`即可



#### 目标二：使播放、录制的音频的幅值接近，且最大幅值在60%-80%之间

设置算法的参数

```
    // 在喇叭静音时，录制设备的音量大小
    int mic_volume_m;
    // 喇叭播放时，录音设备的音量大小（一般设置的比前者小，防止录到的喇叭声过大影响录制效果）
    int mic_volume_p;
    int spk_volume;
```

设置上述三个参数，使播放、回采、录制的音频的幅值接近，且最大幅值在60%-80%之间，算法效果最佳。

