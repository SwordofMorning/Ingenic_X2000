# TP触摸屏

## 一 软件配置

打开iconfigtool配置页面

以x2600e_vast_v20_nand_5.10_factory_defconfig为例子, 根据自己需要的配置选择

![1](x2600e_vast_TP触摸屏.assets/1.png)

TP设备驱动配置

![2](x2600e_vast_TP触摸屏.assets/2.png)

![3](x2600e_vast_TP触摸屏.assets/3.png)

tp供电通过电源管理器 "GPIO-POWER0" 控制

![4](x2600e_vast_TP触摸屏.assets/4.png)

tp i2c配置

![5](x2600e_vast_TP触摸屏.assets/5.png)

![6](x2600e_vast_TP触摸屏.assets/6.png)

## 二 编译和烧录

```c
build$ make x2600e_vast_v20_nand_5.10_factory_defconfig

build$ make clean_app_module_driver

build$ make app_module_driver

build$ make buildroot
```

把编译好的文件系统重新烧录到设备.烧录方法, 请参考: [烧录方法](../01GettingStarted/05软件编译和烧录.md) 

## 三 测试TP触摸屏

触摸屏幕, 串口会有相应的触摸信息打印, 如下：

![7](x2600e_vast_TP触摸屏.assets/7.png)
