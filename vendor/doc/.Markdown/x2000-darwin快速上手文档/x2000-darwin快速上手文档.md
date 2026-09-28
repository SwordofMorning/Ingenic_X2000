# Darwin-x2000 快速上手文档

## 1.配置、编译及烧录步骤

### 1.1.选择板级

进入到图形界面配置文件夹下，打开 IConfigTool

```c
cd linux/tools/iconfigtool/         #进入配置工具目录下
tar xvf IConfigToolApp.tar.gz       #解压工具
cd IConfigToolApp/                  #退回工具目录
./IConfigTool                       #运行可视化图形配置工具
# cd lib/                           #若打开配置工具后出现闪退现象
# rm libQtCore.so.4 libQtGui.so.4   #将lib文件夹内这两个文件删除即可
```

工具打开后，在弹出的界面中选中 'Browse'，选择对应的配置文件，如下图所示即可
![2](img/2.png)

配置文件打开之后，选中"外设测试脚本"，修改该界面下的 WiFi 及 mac
![31](img/31.png)

![32](img/32.png)
修改完成后，按 'Ctrl' + 's' 保存配置

### 1.2.编译

**1.2.1进入到 build 目录下，编译板极配置文件，成功之后全局编译**

```c
cd linux/build
make clean               #清除其他板极配置，若一直使用同一个配置文件可不执行此步骤
make x2000_darwin_factory_defconfig     #编译板极配置文件
make                                    #全局编译，时间较长
```

注意：若 make 失败，可先 make clean 再重新开始本步骤，如若依旧出错，请百度或咨询技术人员。

![3](img/3.png)

编译成功生成的目标文件为 build 目录下的 output 文件夹内的 rootfs.squashfs、xImage 和 u-boot-spl-pad.bin 文件，烧录时需要用到这3个文件

![4](img/4.png)

**1.2.2.进入烧录工具文件夹下，打开 cloner ，选中弹出界面上的配置选项**

这里使用的是 Ubuntu 下的烧录工具，若要使用 Windows 下的烧录工具需要安装驱动，请参考 *linux/tools/burntools/Doc_Chinese/USBCloner烧录工具说明文档.pdf* 文件内的第二节"烧录工具驱动的安装"以及相关使用说明
![6](img/6.png)

![7](img/7.png)

**1.2.3.根据板子型号选择 INFO 下的平台及板极**

![8](img/8.png)

**1.2.4.查看并设置基本信息及分区信息**

![10](img/10.png)
`文件系统擦除块大小为默认值即可`

![11](img/11.png)
`分区名称(Partition name)不能改动`

**1.2.5.选择要烧录的文件并点击保存**

![12](img/12.png)

**1.2.6.进行烧录**

![13](img/13.png)
代码成功烧录时 boot、uboot、kernel 和 rootfs 4个分区依次成功擦除并烧写代码段，最后稳定如下图所示，若出现其他情况请根据 *linux/tools/burntools/Doc_Chinese/USBCloner烧录工具说明文档.pdf* 文件内第八节"常见问题"查找对应问题解决方法

![14](img/14.png)

## 2.命令使用

**若未使用过 minicom ，先安装 minicom，安装完成之后配置串口以及波特率等**

```c
sudo apt-get install minicom    #安装
sudo minicom -s                 #进入配置界面，此时串口线应已连接
```

![15](img/15.png)
**`注意：波特率为3000000 即 3M`**
![16](img/16.png)

**在已烧录的情况下按下 RST 可以看到 minicom 界面开始打印信息**
![33](img/33.png)
直接按回车进入命令行，输入 "lsmod" 查看当前已安装的驱动
```c
lsmod                   #查看当前已安装的驱动
#insmod [驱动文件]       #安装驱动
#rmmod  [驱动文件]       #卸载驱动
```
这里不需要额外安装驱动，故只查看已安装驱动即可
![5](img/5.png)
已安装上图所示驱动，故可使用adc按键、codec、以太网、bt/wifi、LCD、camera等外设

### 2.1.ADC 按键

```c
#模板：cmd_keyboard_test [key_code] <timeout=[time_value]>
#测试adc按键是否正常 key_code对应表如下 time_value为等待时间 单位为s
#"[]"内为必填项，"<>"内为选填项
#该代码表示 key_code 必填，"timeout="选填，但若填了"timeout="必需填time_value
cmd_keyboard_test 102 103 108 105 106 139 timeout=15
```

![9](img/9.png)
按键及其对应键值查询可使用编辑器查看 *linux/factory_test/tools/key_to_code.sh* 文件。

![19](img/19.png)
`若不设置timeout，默认时间为10s，按键对应键值不要输错，程序根据键值判断按键`

### 2.2.Codec

一般情况下 /usr/data 目录下是没有音频文件的，需要提前使用 adb 推到文件系统内

```c
cd linux/factory_test/
adb push audio_pcm.aiff /usr/data/      #将音频文件推给文件系统
cd /usr/data                            #进入用户可操作的目录下
ls                                      #查看是否存在音频文件
```

这一步使用的前提是板子已挂载文件系统，且可以使用 adb
![21](img/21.png)

![22](img/22.png)

```c
ls /dev/snd/pcmC*p      #查找设备节点文件 此时喇叭已接好
                        #若设备节点文件为 /dev/snd/pcmC1D0p ，则播放音频代码为下
aplay -Dplughw:1,0 -f S16_LE -r 16000 audio_pcm.aiff #其中"-Dplughw:1,0"对应"C1D0"
```

![23](img/23.png)
按照上述步骤运行会听到一段清晰的从1念至15的喜气洋洋的音频

### 2.3.以太网

在测试以太网之前，确保板子网线已连接

```c
#ifconfig eth0 [ip地址] netmask [子网掩码] up   #尝试启动网络
                                #ip地址为板子所接网线的地址 掩码一般为255.255.255.0
ifconfig eth0 194.169.3.202 netmask 255.255.255.0 up
#route add default gw [网关]                   #将ip地址最后一个数字改为1即可
route add default gw 194.169.3.1
ping -I eth0 14.215.177.39      #尝试是否可以与14.215.177.39(百度服务器)连通
ifconfig eth0 down                            #断开网络
```

![24](img/24.png)

### 2.4. BT/WiFi

**2.4.1. BT**

```c
#bt_enable_rtk.sh       #配置蓝牙波特率及使用的串口等 不可重复配置
                        #板子复位时自行运行了该文件 故这里不需要运行
hciconfig -a            #查看现有的蓝牙名称
hciconfig hci0 up       #启动蓝牙
hcitool -i hci0 scan    #搜索附近蓝牙
hciconfig hci0 down     #关闭蓝牙
```

![25](img/25.png)

**2.4.2. WiFi**

使用 WiFi 需要设置想要连接 WiFi 的名称及密码，将配置写入文件，启动WiFi时运行配置文件即可

```c
cd /usr/data                            #进入用户目录
vi /usr/data/wpa_supplicant.conf        #编辑文档
```

使用 vi 编辑器打开文件，将里面的内容更改为下述代码段，输入内容前先按下 'i' 进入编辑模式，退出保存时先按下 'Esc' 退出编辑模式，而后同时按下 'Shift' + ':' 进入命令行模式，依次输入 'w' 'q' 即可保存退出。

```c
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1
network={
    ssid="Guest"
    psk="ingenic_guest"
}
```

![26](img/26.png)

```c
 wifi_up.sh /usr/data/wpa_supplicant.conf            #启动WiFi
 ping -I wlan0 www.baidu.com -c 10                   #连通百度服务器
 wifi_down.sh                                        #关闭网络
```

![27](img/27.png)

### 2.5. LCD

使用 LCD 之前需要开启屏幕背光，本配置已默认开启背光

```c
cmd_fb enable /dev/fb0                      #使能屏幕
cmd_fb clear /dev/fb0 color=0xff00ff00      #清屏为绿色
cmd_fb display /dev/fb0                     #显色
cmd_fb disable /dev/fb0                     #失能屏幕
```

![28](img/28.png)

### 2.6. Camera

```c
ls /dev/vic0                                #查找设备节点
cmd_camera power_on /dev/vic0               #摄像头上电
cmd_camera stream_on /dev/vic0              #开始采集
cmd_camera get_frame /dev/vic0 > /tmp/frame #保存图像数据
cmd_camera stream_off /dev/vic0             #停止采集
cmd_camera power_off /dev/vic0              #断电
```

![29](img/29.png)

只要在 /tmp 文件夹内存有 frame 文件，我们就认为摄像头可用

```c
ls /tmp/frame                               #命令结果为 /tmp/frame
```

若想要查看采集到的图像，使用adb pull 将 frame 文件拉到电脑上

```c
adb pull /tmp/frame ./
```

![34](img/34.png)

启动 7yuv 软件(若没有自行安装)，按下 'Ctrl' + 'o' 选择您的 frame 文件(或选中 'File' 栏的 'Open')，具体配置如下图，显示的图像颜色不对是由于 7yuv 没有 frame 文件的图像格式
![35](img/35.png)

下图为君正logo图片示例
![36](img/36.png)