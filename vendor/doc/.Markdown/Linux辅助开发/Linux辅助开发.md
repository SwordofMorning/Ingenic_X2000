# Linux辅助开发

## 1. 配置流程

![](img/1.png)

![](img/2.png)

![](img/3.png)

## 2. 接口/命令详解

### 2.1 GPIO

gpio支持的命令/接口有：

>   gpio shell命令

gpio shell命令详解：

```c
cmd_gpio get_func <GPIO>
功能：获取指定IO功能状态
参数：GPIO                      //IO的名字
example：
               cmd_gpio get_func PB28
```

```c
cmd_gpio get_value <GPIO>
功能：获取IO电平
参数：GPIO                      //IO的名字
example：
              cmd_gpio get _value PB28
```

```c
cmd_gpio set_func <GPIO> <FUNC>
功能：设定指定IO功能
参数：GPIO                           //IO口的名字
              FUNC                         //IO的功能
example：
              cmd_gpio set_func PB28 func0

```

### 2.2 SPI

spi支持的命令/接口有：

>   spi shell命令

spi shell命令详解：

```c
cmd_spi info <dev_path>
功能：获取spi设备的传输配置信息
参数：dev_path                     //spi设备节点的路径
example：
              cmd_spi info /dev/spidev0.0
```

```c
cmd_spi set <dev_path> <arg>[arg...]
功能：设置spi设备传输的配置信息
参数：dev_path                   //spi设备节点的路径
              arg                               /*配置参数（至少配置一项）
                                                        mode=（传输模式）
                                                        speed=（传输速度）
                                                        lsb=（数据低位发送顺序，0表示低位在后，1表示低位在前）
                                                        bits=（数据每次传输的位数）*/
  example：
                 cmd_spi set /dev/spidev0.0 mode=0x01 speed=5000000 lsb=0 bits=8
```

``` c
cmd_spi transfer <dev_path> <data0>[data...]
功能：SPI传输（收发）
参数：dev_path                          //spi设备节点路径
              <data0>[data...]            //数据（单位：16进制）
example：
               cmd_spi transfer /dev/spidev0.0 0x01 0x02 0x03 0x05 0x05
```

```c
cmd_spi write <dev_path> <data0>[data...]
功能：SPI传输（只发不收）
参数：dev_path                          //spi设备节点路径
              <data0>[data...]            //数据（单位：16进制）
example：
               cmd_spi write /dev/spidev0.0 0x01 0x02 0x03 0x05 0x05
```

```c
cmd_spi read <dev_path> <len>
功能：SPI传输（只收不发）
参数：dev_path                          //spi设备节点路径
              len                                      //要接收的长度
example：
              cmd_spi read /dev/spidev0.0 10
```

```c
cmd_spi add_dev <busnum> <cs_gpio>
功能：添加一个spi设备到spi总线上,生成/dev/spidev节点
参数：busnum                           //要挂载的spi总线号
              cs_gpio                           //该设备的片选引脚
example：
              cmd_spi add_dev 0 pc26
```

```c
cmd_spi del_dev <dev_path>
功能：删除已有的spi设备
参数：dev_path                          //spi设备节点路径
example：
              cmd_spi del_dev /dev/spidev0.0
```

### 2.3 I2C

I2C支持的接口/命令有：

>   I2C shell命令

i2c shell命令详解：

```c
cmd_i2c detect <busnum>
功能：探测i2c设备
参数：busnum                            //探测的i2c总线号
example：
             cmd_i2c detect 0
注意：探测前需要先给设备上电，确保设备iic工作正常
```

```c
cmd_i2c read <busnum> <dev_addr> <size>
功能：从指定的i2c总线下的设备接收数据
参数：busnum                            //指定的i2c总线号
              dev_addr                         //指定设备相对应的设备地址
              size                                     //接收的大小
example:
              cmd_i2c read 2 0x58 8
```

```c
cmd_i2c write <busnum> <dev_addr> <data0> [data...]
功能：往指定的i2c总线下的设备发送数据
参数：busnum                             //指定的i2c总线号
              dev_addr                         //指定设备对应的设备地址
              <data0>[data...]            //发送的数据（16进制）
example：
              cmd_i2c  write 2 0x58 0xaa 0xbb
```

```c
cmd_i2c read_reg <busnum> <dev_addr> <reg_addr> <size>
功能：从指定的i2c总线下的设备的寄存器地址读取数据（8位寄存器）
参数：busnum                             //指定的i2c总线号
             dev_addr                          //指定设备对应的设备地址
             reg_addr                           //指定设备的寄存器地址
             size                                      //读取的大小
example：
              read_reg 2 0x58 0x00 8
```

```c
cmd_i2c wrtie_reg <busnum> <dev_addr> <reg_addr> <data0> [data...]
功能：往指定的i2c总线下的设备的寄存器写入数据（8位寄存器）
参数：busnum                             //指定的i2c总线号
             dev_addr                          //指定设备对应的设备地址
             reg_addr                           //指定设备对应的寄存器地址
             <data0>[data...]             //写入的数据（16进制）
example：
             write_reg 2 0x58 0x00 0xaa 0xbb
```

```c
cmd_i2c read_reg_16 <busnum> <dev_addr> <reg_addr_16> <size>
功能：从指定的i2c总线下的设备的寄存器地址读取数据（16位寄存器）
参数：busnum                             //指定的i2c总线号
             dev_addr                          //指定设备对应的设备地址
             reg_addr_16                   //指定设备的寄存器地址
             size                                      //读取的大小
example：
             read_reg_16 0 0x10 0x3010 2
```

```c
cmd_i2c wrtie_reg_16 <busnum> <dev_addr> <reg_addr_16> <data0> [data...]
功能：往指定的i2c总线下的设备的寄存器写入数据（16位寄存器）
参数：busnum                             //指定的i2c总线号
             dev_addr                          //指定设备对应的设备地址
             reg_addr_16                   //指定设备对应的寄存器地址
             <data0>[data...]             //写入的数据（16进制）
example：
             write_reg_16 0 0x10 0x3010 0x55 0xaa
```

### 2.4 ADC

adc支持的命令/接口有：

>   adc shell命令

adc shell命令详解：

```c
cmd_adc get_value <channel>
功能：读取指定通道的adc值
参数：channel                            //将要读取adc通道
example:
             cmd_adc get_value 0

```

```c
cmd_adc get_voltage <channel>
功能：读取指定通道的电压值
参数：channel                            //将要读取adc通道
example:
             cmd_adc get_voltage 0
```

```c
cmd_adc get_vref
功能：获取adc基准电压值
参数：无
example：
             cmd_adc get_vref
```

```c
cmd_adc set_vref <ref_voltage>
功能：设置adc基准电压值
参数：ref_voltage                     //基准电压值
example：
             cmd_adc set_vref 1200
```

### 2.5 PWM

pwm支持的命令/接口有：

>   pwm shell命令

pwm shell命令详解：

```c
cmd_pwm config <gpio> <freq=value> <max_level=value> [active_level=value] [accuracy_priority=freq(levels)]
功能：请求pwm和配置
参数：gpio                                                //io口的名字
              freq=value                                  //频率
              max_level=value                      //PWM最大调制的级数
              active_level=value                  //活跃电平(active_level =1、 active_level=0)
              accuracy_priority=freq         // 频率优先
              accuracy_priority=levels     // 极数优先
example：
              cmd_pwm config pc11 freq=1000000 max_level=300 active_level=1 accuracy_priority=freq
```

```c
cmd_pwm set_level <gpio> <level>
功能：设置pwm级数
参数：pwm_id                       //io口的名字
              level                             //pwm 调制级数，即一个周期内非空闲电平长度
example:
               cmd_pwm set_level pc11 100
```

```c
cmd_pwm disable <gpio>
功能：失能pwm
参数：gpio                                //io口的名字
example：
              cmd_pwm disable PC25
```

### 2.6 EFUSE

efuse支持的命令/接口有：

>   efuse shell命令

efuse shell命令详解：

```c
cmd_efuse read_size <segment_name>
功能：读efuse某一段的大小
参数：segment_name                        //段的名字
example：
              cmd_efuse read_size CHIP_ID
```

```c
cmd_efuse read <segment_name>
功能：读efuse
参数：segment_name                         //段的名字
example：
              cmd_efuse read CHIP_ID
```

```c
cmd_efuse write <segment_name> <start> <size> <data0>[data...]
功能：写efuse
参数：segment_name                         //段的名字
             start                                                //写入的起始位
             size                                                  //写入的大小
             <data0>[data...]                         //写入的数据
example：
             cmd_efuse write CHIP_ID 0 2 0x10 0x11
```

```c
cmd_efuse print_segment_info
功能：打印efuse每个段的信息
参数：无
example：
             cmd_efuse print_segment_info

```

### 2.7 WATCHDOG

watchdog支持的命令/接口有：

>   watchdog shell命令

watchdog shell命令详解：

```c
cmd_watchdog start <ms>
功能：启动看门狗，同时设置最迟喂狗时间
参数：ms                                                //最迟喂狗时间
example：
             cmd_watchdog start 1000
```

```c
cmd_watchdog free
功能：喂狗
参数：无
example：
            cmd_watchdog free
```

```c
cmd_watchdog stop
功能：停止看门狗计数
参数：无
example：
             cmd_watchdog stop
```

```c
cmd_watchdog reset
功能：cpu复位重启
参数：无
example：
             cmd_watchdog reset
```

### 2.8 DTRNG

dtrng支持的命令/接口有：

>   dtrng shell命令

dtrng shell命令详解

```c
cmd_dtrng get_random_number
功能：获取 DTRNG 生成的随机数
参数：无
example：
             cmd_dtrng get_random_number
```

### 2.9 FrameBuffer

framebuffer支持的命令/接口有:

>   framebuffer shell命令
>
>   俄罗斯方块程序

framebuffer shell命令详解：

```c
./cmd_fb enable  <fb_dev_path>
功能：使能fb设备，上电并初始化
参数：fb_dev_path                //要操作的设备驱动路径
example：
                    ./cmd_fb enable /dev/fb0
```

```c
./cmd_fb disable <fb_dev_path>
功能：关闭fb设备，掉电
参数：fb_dev_path                //要操作的设备驱动路径
example：
                    ./cmd_fb disable /dev/fb0
```

```c
./cmd_fb info <fb_dev_path>
功能：显示fb的相关信息
参数：fb_dev_path                //要操作的设备驱动路径
example：
                    ./cmd_fb info /dev/fb0
```

```c
./cmd_fb clear  <fb_dev_path> [color] [frame_index]
功能：清理某一帧的数据
参数：fb_dev_path                //要操作的设备驱动路径
              color                             //清理成的数据（默认为0x000000）数据格式为：ARGB（32位）
              frame_index              //要清理的帧（默认为第0帧）
example：
                    ./cmd_fb clear /dev/fb0 color=0x00ff00 frame_index=0
```

```c
./cmd_fb draw_rect <fb_dev_path> [color] [frame_index]
                                          [x] [y] [width] [height]
功能：绘制某一帧的数据
参数：fb_dev_path                //要操作的设备驱动路径
              color                             //要绘制的颜色数据      数据格式为：ARGB（32位）
              frame_index              //要绘制的帧（默认为第0帧）
               x                                     //开始绘制的水平位置
               y                                     //开始绘制的垂直位置
               width                           //绘制的宽度（默认为屏幕的宽度）
               height                          //绘制的长度（默认为屏幕的长度）
example：
                    ./cmd_fb /dev/fb0/ color=0xff0000 frame_index=0 x=0 y=0
```

```c
./cmd_fb display <fb_dev_path> [frame_index]
功能：将某一帧缓冲的数据刷新到屏幕上
参数：fb_dev_path                //要操作的设备驱动路径
             frame_index                 //要刷新的帧（默认为第0帧）
example：
                    ./cmd_fb display /dev/fb0/ frame_index=1
```

俄罗斯方块程序使用详解：

启动前需要先加载adc驱动

```c
cmd_rector_play
功能：俄罗斯方块小游戏
参数：无
example：
             cmd_rector_play
```

### 2.10 MSCALER

mscaler 支持的命令/接口有：

>   mscaler shell命令

mscaler shell命令详解：

```c
cmd_mscaler convert src [src_arg...] dst [dst_arg...] convert frame
功能：缩放图像
参数：src_arg                     /*
                                                      要进行缩放图像的信息
                                                      file=（图像的路径，默认为stdin，可配合cmd_camera get_frame | 使用）
                                                      size=（图像的大小，默认为0，自动计算）
                                                      fmt=（图像的格式，只支持NV12，NV21）
                                                      width=（图像的宽度 mm）
                                                      height=（图像的高度 mm）
                                                      strige=（图像一行的大小，默认为0，自动计算）
                                                  */
               dst_arg                     /*
                                                        输出图像的信息
                                                        file=（输出图像的路径，默认为stdout）
                                                        fmt=（输出图像的格式）
                                                        width=（输出图像的宽度）
                                                        height=（输出图像的高度）
                                                        strige=（输出图像一行对齐后的大小，默认为0自动计算）
                                                    */
example：
            cmd_mscaler convert src file=/img_in fmt=NV12 width=1920 heigth=1080 dst file=/img_out fmt=BGRA8888 width=480 heigth=854

            cmd_camera get_frame |  cmd_mscaler convert src file=stdin fmt=NV12 width=1920 heigth=1080 dst file=/img_out fmt=BGRA8888 width=480 heigth=854
```

```c
cmd_mscaler show_format
功能：打印输入与输出图像支持的格式
参数：无
example：
             cmd_mscaler show_format
```

### 2.11 KEYBOARD

keyboard 支持的命令/接口有:

>   keyboard shell 命令

 keyboard shell 命令详解：

```c
cmd_keyboard  [timeout]
功能：监测键盘输入
参数：timeout                             //阻塞时间，默认为-1，一直阻塞。为0直接退出
example：
             cmd_keyboard timeout=3000
```

```c
cmd_keyboard wait_key_press <key_code> [timeout]
功能：等待指定按键按下
参数:  key_code                            //按键值
            timeout                               //阻塞时间，默认为-1，一直阻塞。为0直接退出
example：
            cmd_keyboard wait_key_press 102
```

```c
cmd_keyboard wait_key_release <key_code> [timeout]
功能：等待指定按键松开
参数:  key_code                            //按键值
            timeout                               //阻塞时间，默认为-1，一直阻塞。为0直接退出
example：
            cmd_keyboard wait_key_release 102 timeout=3000
```

### 2.12 CAMERA

camera支持的命令/接口有:

>   camera shell 命令
>
>   camera nv12预览命令

camera shell命令详解：

```c
cmd_camera power_on device_path
功能：使能camera设备,上电
参数：无
example：
	cmd_camera power_on /dev/camera
```

```c
cmd_camera power_off device_path
功能：关闭camera设备,掉电
参数：无
example：
	cmd_camera power_off /dev/camera
```

```c
cmd_camera stream_on device_path
功能：开始camera图像录制
参数：无
example：
	cmd_camera stream_on /dev/camera
```

```c
cmd_camera stream off device_path
功能：结束camera图像录制
参数：无
example：
	cmd_camera stream_off /dev/camera
```

```c
cmd_camera info device_path
功能：获取camera设备信息
参数：无
example：
	cmd_camera info /dev/camera
```

```c
cmd_camera get_frame device_path
功能：获取一帧的数据到到标准输出
参数：无
example：
	cmd_camera get_frame /dev/camera
```

```c
cmd_camera drop_all_frames device_path
功能：丢弃已录制的图像数据帧
参数：无
example：
	cmd_camera drop_all_frames /dev/camera
```

camera nv12预览命令详解：

```c
cmd_camera_nv12_preview  device_path
功能：使用屏幕预览camera录制的图像（nv12格式)
参数：无
example：
             cmd_camera_nv12_preview device_path
```

