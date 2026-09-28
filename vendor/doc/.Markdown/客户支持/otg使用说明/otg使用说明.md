																	otg使用说明

一  otg 的模式切换有两种方式

1，根据硬件连接的ID pin状态来自动识别当前的工作模式。OTG_ID引脚为高电平 , 设备初始状态为 device 。 OTG_ID 引脚为低电平,设备初始状态为 host。

2，如果硬件没有接ID pin的话，可以通过命令行来手动切换模式。

以上两种方法的控制，需要配置CONFIG_USB_DWC2_NEW_EXT_ID_PIN。

当CONFIG_USB_DWC2_NEW_EXT_ID_PIN = N时，是第一种方式。

当CONFIG_USB_DWC2_NEW_EXT_ID_PIN = Y时，是第二种方式。

**注意：**当硬件板子没有连接ID pin时，应当配置：CONFIG_USB_DWC2_NEW_EXT_ID_PIN = Y，从而实现手动切换模式。反之，如果硬件板子有连接ID pin时，应当配置：CONFIG_USB_DWC2_NEW_EXT_ID_PIN = N ，从而实现代码自动识别otg状态 。

二  配置方法

CONFIG_USB_DWC2_NEW_EXT_ID_PIN 的配置位置如下：

<img src="otg使用说明.assets/Otg的ID_pin控制方式.png" alt="Otg的ID_pin控制方式" style="zoom:150%;" />

三 供电方法

如果硬件板otg有接vbus供电，那么需配置：USB_DWC2_NEW_EXT_VBUS_DETECT = N，从而代码自动识别供电。 

如果硬件板otg没有接vbus供电，那么需配置：USB_DWC2_NEW_EXT_VBUS_DETECT = Y，从而实现外部供电，usb代码不会去自动检测。

四  供电配置

<img src="otg使用说明.assets/Usb_vbus_detect_control.png" alt="Usb_vbus_detect_control" style="zoom:150%;" />

五  编译方法

因为此处只涉及到kernel的配置，所以配置之后执行：

```
sxyzhang@T430:~/my/work/linux/x2000_sz/build$ make kernel
```

**注意：**编译之后烧录对应out/xImage  即可。 如果测试没有问题，需要将当前配置保存在kernel的默认配置xx_defconfig中，以免在build目录下整体配置执行make yy_defconfig时覆盖当前kernel配置。

```
sxyzhang@T430:~/my/work/linux/x2000_sz/kernel/kernel-x2000$ cp .config arch/mips/configs/xx_defconfig
```

六  问题列表：

1，Otg一直打印：otg new:overcurrent change detected 的处理方法

解决方法：

有可能是otg硬件没有接VBUS检测脚，但是kernel却配置：USB_DWC2_NEW_EXT_VBUS_DETECT = N，则会一直报错如下：

![Otg_vbus_detect影响](otg使用说明.assets/Otg_vbus_detect影响.png)

此时，只需kernel配置：USB_DWC2_NEW_EXT_VBUS_DETECT = Y ，然后重新编译内核即可。