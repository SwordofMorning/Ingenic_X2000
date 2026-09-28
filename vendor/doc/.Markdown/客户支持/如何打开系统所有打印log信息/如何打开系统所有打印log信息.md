# 					如何打开系统所有打印log信息

深圳君正的linux代码默认系统启动输出log是简洁模式，目的是提高启动速度，减少系统打印。 这种模式不利于开发者的开发调试，特别是客户拿到样机之后烧录不断重启的时候，无法从简洁模式的log输出中快速定位问题，此时需要关闭简洁模式，打印kernel的所有输出log。 

本文以x2600 vast v2.0 开发板的编译配置：x2600e_vast_v20_nand_5.10_factory_defconfig 为例，展示具体的关闭简介模式的方法。客户可根据自己当前的编译配置，用同样的方法来关闭简介模式。具体方法如下：

## 一 确定当前uboot的编译路径和配置文件

```c
sxyzhang@T430:~/x2670_sz/build$ make x2600e_vast_v20_nand_5.10_factory_defconfig
sxyzhang@T430:~/x2670_sz/build$ make   // 至少编译一次后才可以执行后面的操作
sxyzhang@T430:~/x2670_sz/build$ cat configs/x2600e_vast_v20_nand_5.10_factory_defconfig | grep uboot
APP_uboot_toolchain_dir=../tools/toolchains/mips-gcc720-glibc229
APP_uboot_dir=../bootloader/uboot-x2000
APP_uboot_config=x2600e_base_xImage_sfc_nand
```

可见，当前uboot的编译路径是：../bootloader/uboot-x2000

当前uboot的编译配置是：x2600e_base_xImage_sfc_nand

## 二 确定当前uboot的板级文件位置

```c
sxyzhang@T430:~/x2670_sz$ cd bootloader/uboot-x2000/
sxyzhang@T430:~/x2670_sz/bootloader/uboot-x2000$ cat include/config.h 
/* Automatically generated - do not edit */
#define CONFIG_SPL_SFC_NAND	1
#define CONFIG_MTD_SFCNAND	1
#define CONFIG_SPL_OS_BOOT	1
#define CONFIG_SPL_PARAMS_FIXER	1
#define CONFIG_X2600E_DDR	1
#define CONFIG_RMEM_MB	16
#define CONFIG_SYS_ARCH  "mips"
#define CONFIG_SYS_CPU   "xburst2"
#define CONFIG_SYS_BOARD "x2600_base"
#define CONFIG_SYS_VENDOR "ingenic"
#define CONFIG_SYS_SOC    "x2600"
#define CONFIG_BOARDDIR board/ingenic/x2600_base
#include <config_cmd_defaults.h>
#include <config_defaults.h>
#include <configs/x2600_base.h>     // 当前板级配置文件
#include <asm/config.h>
#include <config_fallbacks.h>
#include <config_uncmd_spl.h>
```

```c
sxyzhang@T430:~/x2670_sz/bootloader/uboot-x2000$ cat include/configs/x2600_base.h
#ifndef __X2600_BASE_H__
#define	__X2600_BASE_H__

#define CONFIG_ROOTFS_SQUASHFS
#define CONFIG_ROOTFS2_SQUASHFS
#define CONFIG_ARG_QUIET      // 删除此行则关闭简洁模式，默认打开简洁模式
#define CONFIG_SPL_SERIAL_SUPPORT

#include "x2600_base_common.h"  // 当前代码真正的板级配置文件

#endif /* __X2600_BASE_H__ */
```

由上可见，删除#define CONFIG_ARG_QUIET 即关闭简洁模式。默认是存在的，所以默认是打开简洁模式。

## 三 修改生效

```c
sxyzhang@T430:~/x2670_sz/build$ make uboot
```

将生成的如下文件重新烧录即可：

```c
sxyzhang@T430:~/x2670_sz/build$ ll output/u-boot-spl-pad.bin 
-rw-rw-r-- 1 sxyzhang sxyzhang 24576 11月  9 19:49 output/u-boot-spl-pad.bin
```

烧录后应该就有系统启动的所有log信息。 

**注意：**君正平台下的编译配置文件很多，关闭简洁模式的方法是一样的，客户可以根据自己所使用的编译配置文件来打开或者关闭log简洁模式。