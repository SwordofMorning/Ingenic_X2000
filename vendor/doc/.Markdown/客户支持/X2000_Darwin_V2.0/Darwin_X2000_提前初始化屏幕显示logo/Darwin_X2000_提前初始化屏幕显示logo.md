
# 提前初始化屏幕显示logo

## 1 概述

SPL(core0) 加载引导 RTOS(core0), RTOS(core0)同时进行加载kernel镜像和显示logo 任务, 二者均完成后返回SPL(core0), SPL(core0) 引导 kernel(core0)

本文以开发板 Darwin_X2000_V2.0为例进行测试


## 2 配置及编译

根据实际情况使用IConfigTool工具配置屏幕, **linux和rtos的屏幕相关配置必须保持一致 (lcd设备、背光设备、fb驱动)**

### 2.1 Linux(以 x2000_darwin_factory_defconfig为例)

在对应 UBOOT 板极配置中添加配置 **"SPL_RTOS_LOAD_KERNEL"**

```c
bhu@bhu-PC:~/work$ grep -nr  "uboot_config" build/configs/x2000_darwin_factory_defconfig
5:APP_uboot_config=x2000_base_xImage_sfc_nand             // UBOOT板极配置
```

查找/Linux工程目录/bootloader/uboot-x2000/boards.cfg内的x2000_base_xImage_sfc_nand并添加

```c
x2000_base_xImage_sfc_nand	 mips        xburst2	x2000_base	 ingenic	x2000_v12	x2000_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_BOOT,RMEM_MB=16,SPL_PARAMS_FIXER,SPL_RTOS_LOAD_KERNEL
```

为避免rtos和Linux操作空间冲突导致系统运行出错, 可将其分开.本配置有配置RMEM,提供两种方式

1. 配置rtos时将CONFIG_OS_MEM_ADDR(固件加载的地址)配置在预留内存以外靠后的位置, 例如此处仅配置RMEM_MB=16, 总空间大小为128M, 给CONFIG_OS_MEM_SIZE(固件使用的内存大小)预留16M, 可将CONFIG_OS_MEM_ADDR配置为0x86000000 (96MB) ;

![10](Darwin_X2000_提前初始化屏幕显示logo.assets/1.png)



2. 对应 UBOOT 板极配置中添加配置 **",RTOS_SIZE_MB=16"** 为rtos运行预留内存, rtos内CONFIG_OS_MEM_ADDR需配置为0x87000000 (112MB)

```c
x2000_base_xImage_sfc_nand	 mips        xburst2	x2000_base	 ingenic	x2000_v12	x2000_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_BOOT,RMEM_MB=16,SPL_PARAMS_FIXER,SPL_RTOS_LOAD_KERNEL,RTOS_SIZE_MB=16
```

![11](Darwin_X2000_提前初始化屏幕显示logo.assets/10.png)

若配置无RMEM配置,rtos内CONFIG_OS_MEM_ADDR配置为0x87000000 (112MB)即可.

按需求可以调整RMEM_MB、RTOS_SIZE_MB及rtos固件加载的地址和使用的内存大小.

Linux需要在IConfigTool内勾选 **"rtos 中已初始化过 lcd (此时无需再初始化)"**

![2](Darwin_X2000_提前初始化屏幕显示logo.assets/2.png)

若使用了 **gpios regulator**, 需要在IConfigTool内勾选 **[ 开机时使能, 防止kernel开机时自动关闭 (如快起时,提前亮屏的场景) ]**

如下lcd供电使用到了 **gpios regulator**

![14](Darwin_X2000_提前初始化屏幕显示logo.assets/11.png)

设备电源管理

![3](Darwin_X2000_提前初始化屏幕显示logo.assets/3.png)


编译Linux系统:
```shell
/Linux工程目录/build$ make x2000_darwin_factory_defconfig
/Linux工程目录/build$ make
```

### 2.2 rtos(以x2000_boot_logo_example_defconfig为例)

**rtos和linux的屏幕相关配置必须保持一致(lcd设备、背光设备、fb驱动)**

rtos需要在IConfigTool内根据实际情况配置 **SFC驱动**, 勾选 **CMYK_TO_RGB** 和 **libjpeg**

![4](Darwin_X2000_提前初始化屏幕显示logo.assets/4.png)

![5](Darwin_X2000_提前初始化屏幕显示logo.assets/5.png)

![6](Darwin_X2000_提前初始化屏幕显示logo.assets/6.png)

根据Linux uboot配置是否添加RTOS_SIZE_MB来配置**固件加载的地址**.本文不添加,使用2.1章节有配置RMEM时第一种方式,固件加载地址为0x86000000(96MB) 

![7](Darwin_X2000_提前初始化屏幕显示logo.assets/7.png)

修改完成配置后保存

在/rtos工程目录/freertos/vendor/**vendor.c**内, 修改代码如下:

```C
#include <stdio.h>
#include "../example/driver/quick_boot_logo_and_load_kernel_example.c"

void vendor_init(void *data)
{
    printf("vendor init...\n");
    quick_boot_logo_and_load_kernel(arg);  
}
```

 rtos加载完logo进入kernel例子,源码位于/rtos工程目录/freertos/example/driver/quick_boot_logo_and_load_kernel_example.c

logo图片位于/rtos工程目录/freertos/example/resource/test.jpeg, 可替换，格式仅限jpeg

```c
/******************************boot_logo******************************/
#include <cmyk_to_rgb.h>
#include "jpeglib.h"
#include "jerror.h"

#include <driver/fb.h>
#include <driver/backlight.h>
#include <driver/irq.h>

#include <include_bin.h>
INCBIN(jpeg_display, "example/resource/test.jpeg");  //示例中存放开机logo图片位置，可替换为自己开机logo图片, 格式为jpeg

static struct fb_info info;
static struct fb_handle *fb0_handle;
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

![8](Darwin_X2000_提前初始化屏幕显示logo.assets/8.png)

rtos分区烧录/rtos工程目录/freertos/zero.bin文件, 烧录成功后板子上电显示logo后启动Linux。

![9](Darwin_X2000_提前初始化屏幕显示logo.assets/9.png)
