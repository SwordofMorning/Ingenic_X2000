# USB设备休眠唤醒

判断系统何时需要休眠，主要为了满足usb协议规定的2.5mA的挂起电流。

**usb的状态 参考  doc/开发使用说明/USB使用说明文档《USB状态监听》文档。**

系统休眠指令   echo mem > /sys/power/state





## pc 通过 usb 唤醒设备需要两大块支持

usb模拟的设备枚举信息里面声明支持休眠唤醒，部分pc对于不支持唤醒的设备，在休眠时会对设备断电处理。

usb phy支持作为唤醒源（x2000 x1600）。usb phy只支持数据线唤醒，如果需要vbus或id唤醒，也需要而外接io处理。

如果usb phy不支持作为唤醒源，需要外部电路去支持唤醒。



### usb phy支持被唤醒功能内核配置修改

5.10内核    -》 修改设备树文件module_drivers/dts/x2000_module_base.dts

```
&otg {
	g-use-dma;
	dr_mode = "otg";
	status = "okay";
	snps,need-phy-for-wake;         /* 设置usb驱动需要phy作为唤醒源 */
};
```

```
&otg_phy {
	status = "okay";
	enable-usb-wakeup;				/* 设置usb phy支持唤醒功能 */
};
```



### usb phy不支持被唤醒功能内核配置修改

```
&otg {							/* 不需要usb驱动设置唤醒源 */
	g-use-dma;
	dr_mode = "otg";
	status = "okay";
};
```

```
&otg_phy {
	status = "okay";
	enable-usb-wakeup;				/* 设置usb phy支持唤醒功能 */
	ingenic,usb-switch-gpio = <&gpe 22 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;  /* 设置唤醒电路的切换引脚,注意实际电路的电平  */
	ingenic,usb-wakeup-gpio = <&gpd 17 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;	/* 设置唤醒电路的唤醒引脚 */
};
```



### 休眠唤醒的应用demo

需要配置gpio按键到输入子系统并支持唤醒

根据自己实际情况配置，主要是用于gadget_hid_wakeup_keyboard应用的数据输入。

![1](1.png)

**参考 doc/开发使用说明/USB使用说明文档/设备《USB_HID.pdf》 文档**

**参考  doc/开发使用说明/USB使用说明文档《USB应用层节点与功能引脚介绍.pdf》 文档**

```
文件说明：
hid_device_config.sh     把usb枚举成键盘
hid_keyboard_sleep_wakeup.sh	休眠唤醒的demo脚本
gadget_hid_wakeup_keyboard		 把input子系统数据转成usb键盘数据上报给pc
usb_state						usb状态检测
```

```
/etc/init.d/S90adb stop   /* 如果默认启动adb，先把adb关闭 */

/usr/data/hid_device_config.sh start keyboard   /* 把usb设备枚举成键盘 */

usr/data/hid_keyboard_sleep_wakeup.sh /dev/hidg0 /usr/data/gadget_hid_wakeup_keyboard
 /usr/data/usb_state /sys/class/udc/13500000.otg   /* 启动usb休眠demo脚本 */
 
usb接入pc电脑，按下按键pc会对于收到键盘数据，pc休眠开发板也会跟随休眠，pc唤醒开发板也会跟随唤醒。

pc休眠开发板跟随休眠，按下按键开发板唤醒，脚本会自动唤醒pc电脑。

```

