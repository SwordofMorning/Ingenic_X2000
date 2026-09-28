# USB 应用层节点与功能引脚介绍

## 1.引脚介绍

**OTG_ID**: OTG_ID引脚为高电平,设备初始状态为device。OTG_ID引脚为低电平，设备初始状态为host。

**USB_DETECT**：当设备作为device时，USB_DETECT引脚有效。USB_DETECT引脚为高电平，设备与主机正常连接。当USB_DETECT引脚为低电平，设备与主机断开连接。当设备作为host时，USB_DETECT引脚不使用。

**DRVVBUS**：当设备作为host时，DRVVBUS引脚输出5V电源供电。  当设备作为device时，DRVVBUS引脚不使用。

**WAKEUP_GPIO**：WAKEUP_GPIO需要具有可唤醒功能，并与usb数据引脚连接。当PC在休眠时，手动唤醒PC，主机自主发送Resume信号（数据线极性翻转保持20ms唤醒设备，并以EOP信号结尾），导致WAKEUP_GPIO引脚的电平翻转进行唤醒设备。

**SWITCH_GPIO**：SWITCH_GPIO引脚与usb switch芯片设置引脚连接。当SWITCH_GPIO引脚为输出低电平时，switch芯片数据线与usb控制器连通。当SWITCH_GPIO引脚为输出高电平时，switch芯片数据线与wakeup gpio连通。

## 2.usb设备节点介绍

**soft_connect** ：设备软件连接与断开（connect、disconnect）

```c
/* 断开设备 */
echo "disconnect" > /sys/class/udc/dwc2/soft_connect
/* 连接设备 */
echo "connect" > /sys/class/udc/dwc2/soft_connect
```

**srp** ：通过srp协议来唤醒主机

```c
/* 唤醒主机 */
echo 1 > /sys/class/udc/dwc2/srp
```

**current_speed**：当前设备运行速度

```c
high-speed、 full-speed、low-speed
```

**maximum_speed**：当前设备支持最大速度

**state** ：当前设备的状态（not attached、attached、powered、reconnecting、unauthenticated、default、addresssed、configured、suspended）

```c
/* 所有状态列表 */
not attached、attached、powered、reconnecting、unauthenticated、default、addresssed、configured 、suspended
```

![](2.jpg)

**role**：设备主从切换

```c
在kernel目录下make menuconfig进行配置role接口
Device Drivers  --->
     [*] USB support  --->
         [*]     External Id Pin
配置完成再编译烧录镜像后会产生dwc2-role-switch目录

/* 切换host模式 */
echo "host" > /sys/class/usb_role/dwc2-role-switch/role
/* 切换device模式 */
echo "device" > /sys/class/usb_role/dwc2-role-switch/role
```

## 3.keyboard作为usb休眠唤醒示例

### 3.1. menuconfig 配置流程

```c
Device Drivers  --->
     [*] USB support  --->
            <*>   Support for Host-side USB
            < >   DesignWare USB2 DRD Core Support
            <*>   DesignWare USB2 DRD Core Support (NEW)
                      DWC2 Mode Selection (Dual Role mode)  --->
            [*]     Force full speed mode
            [*]     External Vbus Detect
            [*]   USB Physical Layer drivers  --->
            <*>   Ingenic USB PHY controller Driver
            [*]     External vbus detect gpio  --->
                      [*]   External vbus detect gpio wakeup enable
                      (86)  gpio number
                      [ ]   gpio level invert (NEW)
            [*]     Vbus supply gpio support  --->
                       (57)  gpio number
                       [ ]   gpio level invert (NEW)
            [*]     Switch gpio support  --->
                      (89)  gpio number
                       [ ]   gpio level invert (NEW)
            [*]     Wakeup gpio support  --->
                       (15)  gpio number
                        [ ]   gpio level invert (NEW)
            <*>   USB Gadget Support  --->
                       <*>   USB Gadget Drivers (HID Gadget)  --->
```

### 3.2. IConfig按键配置流程

![](1.png)

### 3.3.休眠唤醒流程

1.编译keyboard设备与按键的应用

> cd doc/开发使用说明/USB使用说明文档/设备/USB_HID 目录
>
> ../../../../../buildroot/buildroot/output/host/bin/mips-linux-gnu-gcc gadget_hid_wakeup_keyboard.c -lhardware2 -lpthread  -o  gadget_hid_wakeup_keyboard

2.编译USB状态监听应用

> cd doc/开发使用说明/USB使用说明文档/设备/USB状态监听
>
> ../../../../buildroot/buildroot/output/host/bin/mips-linux-gnu-gcc  usb_state.c -o  usb_state

3.执行键盘的休眠唤醒脚本hid_keyboard_sleep_wakeup.sh。

> 将生成 gadget_hid_wakeup_keyboard、usb_state、和hid_keyboard_sleep_wakeup.sh 文件放入开发板的/tmp或者/esr/data目录
>
> Usage: $prg_name <device_path> <gadget_hid_wakeup_keyboard_app_path> <usb_state_app_path> <udc_path>
>
> /usr/data/hid_keyboard_sleep_wakeup.sh /dev/hidg0 /usr/data/gadget_hid_wakeup_keyboard
> /usr/data/usb_state /sys/class/udc/13500000.otg
>
> 注意:udc_path 路径不同平台会有差异,x1000平台udc_path路径为/sys/class/udc/dwc2/,x2000平台udc_path路径为/sys/class/udc/13500000.otg