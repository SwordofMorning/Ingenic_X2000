

## 		log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例

## 一 修改初衷

经常会有客户需要修改代码的log输出uart口，主要有两个原因。 

1. 是客户拿到的开发板与代码的编译选项不匹配，造成代码初始化的uart并不是真正硬件连接的log输出口。

2. 客户根据自己硬件设计改动了默认的log输出口。

无论是哪种情况，客户都需要了解代码中uart的代码流程，从而掌握如何根据自己的现有硬件条件来配置代码，从而实现log正常输出。

## 二 修改方法

按照 doc/FAE文档/X2000H_Darwin_X2000_V2.0/x2000H_Darwin_v2.0_开发板快速上手说明.pdf**整体编译烧录后**，如果log信息无法正常输出，需要做如下检查．

### 	1．确定当前uboot的编译路径和配置文件

```
sxyzhang@T430:~/x2000_sz/build$ cat ./configs/x2000_darwin_v20_debug_nand_defconfig | grep uboot
APP_uboot_toolchain_dir=../tools/toolchains/mips-gcc720-glibc229
APP_uboot_dir=../bootloader/uboot-x2000
APP_uboot_config=x2000_base_xImage_sfc_nand
```

可知，当前uboot编译目录为：../bootloader/uboot-x2000

所用的uboot配置文件为：x2000_base_xImage_sfc_nand

### 2．确定当前的配置文件

```
sxyzhang@T430:~/x2000_sz/build$ cd ../bootloader/uboot-x2000
sxyzhang@T430:~/x2000_sz/bootloader/uboot-x2000$ vim include/config.h
```

![9](log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例.assets/9.png)

```
sxyzhang@T430:~/x2000_sz/bootloader/uboot-x2000$ vim include/configs/x2000_base.h
```

![10](log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例.assets/10.png)

可见当前代码真正的板级配置文件为：include/configs/x2000_base_common.h

```
sxyzhang@T430:~/x2000_sz/bootloader/uboot-x2000$ vim include/configs/x2000_base_common.h
```

![1](log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例.assets/1.png)

**如果更换log输出口或波特率则修改上述两项即可。一般我们代码只支持两个波特率：115200和3000000。**

上图默认log输出的uart为：uart2，波特率：115200n8。

### ３．重新编译并烧录

```
sxyzhang@T430:~/x2000_sz/build$ make uboot
```

**注意：**此处要确保代码已经整体编译过一遍了，保证uboot已经有默认配置了．

### ４．重新烧录生成的build/output/u-boot-spl-pad.bin

![7](log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例.assets/7.png)

此处如果设置正确，再次烧录时minicom串口终端会有烧录的log输出．例如：

![8](log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例.assets/8.png)

## 三　出现问题后的代码梳理

### 	１．确定ｕｂｏｏｔ引导第一阶段代码入口

```
sxyzhang@T430:~/x2000_sz/bootloader/uboot-x2000$ vim include/config.h
```

![2](log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例.assets/2.png)

### ２．跟踪代码

```
sxyzhang@T430:~/x2000_sz/bootloader/uboot-x2000$ vim arch/mips/cpu/xburst2/x2000_v12/soc.c 
```

![3](log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例.assets/3.png)

```
sxyzhang@T430:~/x2000_sz/bootloader/uboot-x2000$ vim drivers/gpio/jz_gpio_common.c 
```

![4](log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例.assets/4.png)

![5](log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例.assets/5.png)

### ３． 检查log输出的uart口默认配置

```
sxyzhang@T430:~/x2000_sz/bootloader/uboot-x2000$ vim drivers/gpio/jz_gpio/x2000_v12_gpio.c
```

![6](log输出端口uartX的修改方法---以Darwin_X2000_v2.0板为例.assets/6.png)

**此处一定要确保include/configs/x2000_base_common.h中设置的CONFIG_SYS_UART_INDEX与上图中默认的uartX引脚对应．**

## 四　常见问题解答

### １．系统log没有输出

​	1.1 因为软件默认设置的CONFIG_SYS_UART_INDEX与硬件板上的uart gpio不对应

​	例如，本例中#define CONFIG_SYS_UART_INDEX           2　，那么结合上图软件上默认设置的就是pd组uart2．　对应的主控gpio就是：pd30-31,　默认为复用gpio为FUNC_０| GPIO_PULL,　即：将主控pd30-31设置为带内部上拉的uart　功能．

​	大部分的主控芯片都存在同一个uart号对应不同的gpio引脚的问题．例如上图中uart4有两个通道可选：PA2-3 和 PC11-12．

​	1.2 请参照上述＂出现问题后的代码梳理＂部分详细介绍．

### ２．出现乱码

​	2.1 检查minicom设置的波特率与include/configs/x2000_base_common.h　设置的波特率是否一致．

   **一般我们代码只支持两个波特率：115200和3000000。**

​	2.2 检查硬件RX   TX 是否接反．

​	2.3 检查接地GND是否接触良好，

​	2.4 检查硬件是否有其他干扰． 

### ３．烧录没有log输出．

检查烧录工具：　配置－－－＞INFO　－－－＞其他－－－＞串口　＆　波特率　设置是否与硬件板对应．



