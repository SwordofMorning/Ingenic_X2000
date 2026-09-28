# ALSA音频驱动添加流程

## 1.配置aic(i2s)驱动

> 使用IConfigTool对x1000的 aic(i2s)驱动进行配置

<img src="img/5.png"  />

<img src="img/6.png" style="zoom: 67%;" />

<img src="img/9.png" style="zoom:67%;" />



## 2. 添加codec驱动

> 本例使用的是x1000的内部codec(icodec)，所以直接使用IConfigTool对x1000的 icodec 驱动进行配置即可。如下图所示：

<img src="img/7.png" style="zoom:67%;" />

<img src="img/8.png" style="zoom:67%;" />

## 3. 添加一个ALSA声卡设备

#### 3.1.添加声卡设备源文件

> 进入目录devices/alsa-sound-card，添加声卡设备源文件(.c、.h等)以及Makefile文件。这里以使用x1000内部codec(icodec)创建一个声卡设备 x1000_icodec_sound_card 为例，如下图所示：

<img src="img/1.png"  />

#### 3.2.创建编译规则和添加Iconfig配置

> 进入目录package/devices/alsa-sound-card/，为x1000_icodec_sound_card创建编译规则。如下图所示：（"package/"目录下主要主要存放这编译规则文件（.mk）以及Iconfig的配置文件（.in），详见*模块驱动添加流程* 说明文档）

<img src="img/2.png"  />

> 在package/devices/alsa-sound-card/alsa-sound-card.mk文件末尾加入

```
package-$(MD_x1000_ICODEC_SOUND_CARD) += package/devices/alsa-sound-card/x1000_icodec_sound_card/x1000_icodec_sound_card.mk
```

> MD_x1000_ICODEC_SOUND_CARD：可以用y代替。用宏可以使用Iconfig对其进行配置，从而决定加不加入编译，Iconfig的配置选项添加详见 *IConfig配置选项添加说明文档*。

<img src="img/3.png"  />

#### 3.3.配置snd_soc_dai_link和snd_soc_card结构体

> snd_soc_dai_link、snd_soc_card结构体说明

```c
static struct snd_soc_dai_link icodec_board_dais[] = {
    [0] = {
        /* 仅表示声卡信息，无实质意义 */
        .name = "x1000 icodec",

        /* 仅表示声卡信息，无实质意义 */
        .stream_name = "x1000 icodec pcm",

        /* cpu_dai_name表示选择对应的dai接口，这里是君正平台的aic(i2s)接口 */
        .cpu_dai_name = "ingenic-aic",

        /* platform_name表示选择对应的DMA接口，这里直接整合在cpu_dai接口中 */
        .platform_name = "ingenic-aic",

        /*codec_dai_name表示选择codec的哪一个接口 */
        .codec_dai_name = "internal-codec",

        /* codec_name表示选择对应的codec，这里选择君正平台的内部codec(icodec)*/
        .codec_name = "ingenic-icodec",

        /*设置dai_fmt为i2s模式，设置 codec clk和FRM都作为master，具体参数设置参考include/sound/soc-dai.h*/
        .dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBM_CFM,

        /* SoC audio ops，提供了hw_params，hw_free等函数，参考include/sound/soc.h */
        .ops = &i2s_ops,
    },
    [1] = {
       /* 对于有多个声卡的情况依次添加即可 */
    },
};

/* SoC card */
static struct snd_soc_card icodec_snd_card = {
    .name = "icodec-sound-card",
    .owner = THIS_MODULE,
    .dai_link = icodec_board_dais, /* 上文填充好的snd_soc_dai_link结构体*/
    .num_links = ARRAY_SIZE(icodec_board_dais),
};
```

#### 3.4.调用snd_soc_dai_set_sysclk函数设置声卡参数

```c
/**
 * snd_soc_dai_set_sysclk
 * @dai: DAI
 * @clk_id: 选择codec类型，可选SELECT_INNER_CODEC、SELECT_EXT_CODEC
 * @freq: 设置时钟频率，0表示自动选择一个合适的时钟频率
 * @dir: 选择时钟方向，0表示输入，1表示输出
 */
int snd_soc_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id,unsigned int freq, int dir);
```

> 本例选择的codec类型为内部codec，由系统自动选择合适的时钟频率，时钟方向为输入，如下图所示：

<img src="img/4.png"  />

#### 3.5.调用snd_soc_register_card注册声卡

```c
static int icodec_board_probe(struct platform_device *pdev)
{
    int ret = 0;

    icodec_snd_card.dev = &pdev->dev;

    ret = snd_soc_register_card(&icodec_snd_card);/* 上文填充好的snd_soc_card结构体 */
    if (ret)
        panic("icodec-sound-card: ASOC register card failed !\n");

    return ret;
}
```

> 至此，一个alsa声卡设备添加完成。
