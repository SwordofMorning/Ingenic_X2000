
# 提前初始化屏幕显示logo

## 1 概述

SPL(core0) 加载引导 RTOS(core0), RTOS(core0)同时进行加载kernel镜像和显示logo 任务, 二者均完成后返回SPL(core0), SPL(core0) 引导 kernel(core0)

支持X1600、X2000、X2600系列CPU,已在Darwin(x2000),Halley6(x1600e),VAST(x2600e)开发板验证.


## 2 配置及编译

根据实际情况使用IConfigTool工具配置屏幕, **linux和rtos的屏幕相关配置必须保持一致 (lcd设备、背光设备、fb驱动)**

### 2.1 Linux(以x2000_nand_defconfig为例)

在对应 UBOOT 板极配置中添加配置 **"SPL_RTOS_LOAD_KERNEL"**

```c
bhu@bhu-PC:~/work$ grep -nr  "uboot_config" build/configs/x2000_nand_defconfig
5:APP_uboot_config=x2000_base_xImage_sfc_nand             // UBOOT板极配置
```

查找/Linux工程目录/bootloader/uboot-x2000/boards.cfg内的x2000_base_xImage_sfc_nand并添加

```c
x2000_base_xImage_sfc_nand	 mips        xburst2	x2000_base	 ingenic	x2000_v12	x2000_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_BOOT,RMEM_MB=16,SPL_PARAMS_FIXER,SPL_RTOS_LOAD_KERNEL
```

为避免rtos和Linux操作空间冲突导致系统运行出错, 可将其分开.本配置有配置RMEM,提供两种方式

1. 配置rtos时将CONFIG_OS_MEM_ADDR(固件加载的地址)配置在预留内存以外靠后的位置, 例如此处仅配置RMEM_MB=16, 总空间大小为128M, 给CONFIG_OS_MEM_SIZE(固件使用的内存大小)预留16M, 可将CONFIG_OS_MEM_ADDR配置为0x86000000 (96MB) ;

<img src="img/10.png" />



2. 对应 UBOOT 板极配置中添加配置 **",RTOS_SIZE_MB=16"** 为rtos运行预留内存, rtos内CONFIG_OS_MEM_ADDR需配置为0x87000000 (112MB)

```c
x2000_base_xImage_sfc_nand	 mips        xburst2	x2000_base	 ingenic	x2000_v12	x2000_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_BOOT,RMEM_MB=16,SPL_PARAMS_FIXER,SPL_RTOS_LOAD_KERNEL,RTOS_SIZE_MB=16
```

<img src="img/11.png" />

若配置无RMEM配置,rtos内CONFIG_OS_MEM_ADDR配置为0x87000000 (112MB)即可.

按需求可以调整RMEM_MB、RTOS_SIZE_MB及rtos固件加载的地址和使用的内存大小.

Linux需要在IConfigTool内勾选 **"rtos 中已初始化过 lcd (此时无需再初始化)"**

<img src="img/2.png" />

X1600与X2600 fb驱动配置大致相同

<img src="img/12.png" />

<img src="img/13.png" />

若使用了 **gpios regulator**, 需要在IConfigTool内勾选 **[ 开机时使能, 防止kernel开机时自动关闭 (如快起时,提前亮屏的场景) ]**

如下lcd供电使用到了 **gpios regulator**

<img src="img/14.png" />

设备电源管理

<img src="img/3.png" />


编译Linux系统:
```shell
/Linux工程目录/build$ make x2000_nand_defconfig
/Linux工程目录/build$ make
```

### 2.2 rtos(以x2000_boot_logo_example_defconfig为例)

**rtos和linux的屏幕相关配置必须保持一致(lcd设备、背光设备、fb驱动)**

rtos需要在IConfigTool内根据实际情况配置 **SFC驱动**, 勾选 **CMYK_TO_RGB** 和 **libjpeg**

<img src="img/4.png" />

<img src="img/5.png" />

<img src="img/6.png" />

根据Linux uboot配置是否添加RTOS_SIZE_MB来配置**固件加载的地址**.本文不添加,使用2.1章节有配置RMEM时第一种方式,固件加载地址为0x86000000(96MB) 

<img src="img/7.png" />

修改完成配置后保存

在/rtos工程目录/freertos/vendor/**vendor.c**内, 修改代码如下:

```C
#include <stdio.h>
#include "../example/driver/quick_boot_logo_and_load_kernel_example.c"

void vendor_init(void *data)
{
    printf("vendor init...\n");
    quick_boot_logo_and_load_kernel(arg);  
 //rtos加载完logo进入kernel例子,源码位于/rtos工程目录/freertos/example/driver/quick_boot_logo_and_load_kernel_example.c
}
```

> **例子默认为gpio背光, linux和rtos的背光设备需配置为gpio背光. 若需要pwm背光,可修改例子为pwm背光方式**

编译rtos系统:

```shell
/rtos工程目录/freertos$ source build/envsetup.sh
/rtos工程目录/freertos$ make x2000_boot_logo_example_defconfig
/rtos工程目录/freertos$ make
```


## 3 烧录

分区表配置如下, 在烧录工具分区内分配一块rtos分区:

<img src="img/8.png" style="zoom:80%;" />

rtos分区烧录/rtos工程目录/freertos/zero.bin文件, 烧录成功后板子上电显示logo后启动Linux。

<img src="img/9.png" />
