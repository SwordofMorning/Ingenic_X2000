

# 		x2600e_vast_log输出端口uartX的修改方法

## 一 修改初衷

经常会有客户需要修改代码的log输出uart口，主要有两个原因. 

1. 是客户拿到的开发板与代码的编译选项不匹配，造成代码初始化的uart并不是真正硬件连接的log输出口.

2. 客户根据自己硬件设计改动了默认的log输出口.

无论是哪种情况，客户都需要了解代码中uart的代码流程，从而掌握如何根据自己的现有硬件条件来配置代码，从而实现log正常输出.

## 二 修改方法

按照 doc/FAE文档/X2600E_VAST/x2600e_vast快速上手文档.pdf整体编译烧录后, 如果log信息无法正常输出, 需要做如下检查． 

### 	2.1 确定当前uboot的编译路径和配置文件

```c
bhu@bhu-PC:~/work/build$ cat ./configs/x2600e_vast_v20_nand_5.10_factory_defconfig  | grep uboot
APP_uboot_toolchain_dir=../tools/toolchains/mips-gcc720-glibc229
APP_uboot_dir=../bootloader/uboot-x2000
APP_uboot_config=x2600e_base_xImage_sfc_nand
```

可知，当前uboot编译目录为：../bootloader/uboot-x2000

所用的uboot配置文件为：x2600e_base_xImage_sfc_nand

### 2.2 确定当前的配置文件

```
bhu@bhu-PC:~/work/build$ cd ../bootloader/uboot-x2000
bhu@bhu-PC:~/work/bootloader/uboot-x2000$ vim include/config.h
```

![9](x2600e_vast_log输出端口uartX的修改方法.assets/9.png)

```
bhu@bhu-PC:~/work/bootloader/uboot-x2000$ vim include/configs/x2600_base.h
```

![10](x2600e_vast_log输出端口uartX的修改方法.assets/10.png)

可见当前代码真正的板级配置文件为：include/configs/x2600_base_common.h

```
bhu@bhu-PC:~/work/bootloader/uboot-x2000$ vim include/configs/x2600_base_common.h
```

![1](x2600e_vast_log输出端口uartX的修改方法.assets/1.png)

**如果更换log输出口或波特率则修改上述两项即可。一般我们代码只支持两个波特率：115200和3000000。**

上图默认log输出的uart为：uart2，波特率：3000000

### 2.3 重新编译并烧录

```c
bhu@bhu-PC:~/work/build$ make uboot
```

**注意：**此处要确保代码已经整体编译过一遍了，保证uboot已经有默认配置了．

### 2.4 重新烧录生成的build/output/u-boot-spl-pad.bin

![7](x2600e_vast_log输出端口uartX的修改方法.assets/7.png)

此处如果设置正确，再次烧录时minicom串口终端会有烧录的log输出．例如：

![8](x2600e_vast_log输出端口uartX的修改方法.assets/8.png)

## 三 出现问题后的代码梳理

### 	3.1 确定uboot引导第一阶段代码入口

```
bhu@bhu-PC:~/work/bootloader/uboot-x2000$ vim include/config.h
```

![2](x2600e_vast_log输出端口uartX的修改方法.assets/2.png)

### 3.2 跟踪代码

```
bhu@bhu-PC:~/work/bootloader/uboot-x2000$ vim arch/mips/cpu/xburst2/x2600/soc.c 
```

![3](x2600e_vast_log输出端口uartX的修改方法.assets/3.png)

```
bhu@bhu-PC:~/work/bootloader/uboot-x2000$ vim drivers/gpio/jz_gpio_common.c 
```

![4](x2600e_vast_log输出端口uartX的修改方法.assets/4.png)

![5](x2600e_vast_log输出端口uartX的修改方法.assets/5.png)

### 3.3 检查log输出的uart口默认配置

```
bhu@bhu-PC:~/work/bootloader/uboot-x2000$ vim drivers/gpio/jz_gpio/x2600_gpio.c
```

![6](x2600e_vast_log输出端口uartX的修改方法.assets/6.png)

**此处一定要确保include/configs/x2600_base_common.h中设置的CONFIG_SYS_UART_INDEX与上图中默认的uartX引脚对应．**

## 四 常见问题解答

### 4.1 系统log没有输出

​	1. 因为软件默认设置的CONFIG_SYS_UART_INDEX与硬件板上的uart gpio不对应

​	例如，本例中#define CONFIG_SYS_UART_INDEX   2,  那么结合软硬件设置的就是pd组uart2. 对应的主控gpio就是：pd12-13,　默认为复用gpio为FUNC_0| GPIO_PULL,　即：将主控pd12-13设置为带内部上拉的uart功能．

### 4.2 出现乱码

​	1  检查minicom设置的波特率与include/configs/x2600_base_common.h设置的波特率是否一致．

   **一般我们代码只支持两个波特率：115200和3000000**

​	2 检查硬件RX   TX 是否接反．

​	3 检查接地GND是否接触良好，

​	4 检查硬件是否有其他干扰． 

### 4.3 烧录没有log输出

检查烧录工具：　配置－－－＞INFO　－－－＞其他－－－＞串口＆波特率设置是否与硬件板对应．
