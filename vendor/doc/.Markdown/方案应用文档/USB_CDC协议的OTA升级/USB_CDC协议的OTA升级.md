# USB's CDC协议的OTA升级

## 一 支持的软硬件平台

### 1.支持的硬件平台

​	支持x1000、x1600、x2000、x2600、ad100系列CPU，已在ilock(x2000e & mmc0)、Halley6(x1600e &nand)、x2600_evb_v1.0(x2600e & nor)开发板上验证。

### 2.支持的软件平台

​	ota支持升级linux系统和Freertos系统。

## 二 OTA原理和介质介绍

### 1 OTA简介和实现方法

君正USB's CDC协议的OTA简单来说分为三个步骤。

- 第一步： 准备好需要更新的升级固件。

- 第二步：通过USB的CDC协议将升级固件传输到设备上。

- 第三步：设备将得到的OTA升级固件更新到自身的存储介质中，然后重启切换到最新升级的系统上，从而完成整个OTA流程。

  ​	君正基于Freertos系统，OTA升级的策略是采用创建一个rtos_ota分区，分区中放置具有ota升级功能的Freertos系统代码。SPL引导代码通过判断ota分区中的内容是否为"ota:kernel2"， 来决定是否要进入具有ota升级功能的Freertos系统。也就是一台设备上具有ota升级功能的Freertos系统代码和用户的主系统，但是每次只运行其中的一套系统，两个系统共同使用一份SPL代码。




### 2 OTA升级支持的存储介质

​	君正USB's CDC协议的OTA目前支持的存储介质有mtd和mmc，mtd包含：nand和nor。



## 三 OTA功能使用流程

### 1 uboot添加支持rtos_ota功能

####  1.1 MTD存储介质

​		以配备nor flash 的PD_X2600_EVB_V1.0开发板，主系统为Freertos的编译配置为例，编译需要支持rtos ota功能的uboot固件的步骤如下：

#####     1.1.1 修改对应的板级配置文件

​		对应的SPL的编译配置为x2600e_base_rtos_sfc_nor，此配置对应的板级配置文件为x2600_base.h。

​		文件的路径如下：

```
[工程目录]/bootloader/uboot-x2000/include/configs/{board}.h

以x2600为例：[工程目录]/bootloader/uboot-x2000/include/configs/x2600_base.h
```

​		增加OTA的功能需要对x2600_base.h文件进行如下修改：

```c
#ifndef __X2600_BASE_H__
#define	__X2600_BASE_H__

#define CONFIG_ROOTFS_SQUASHFS
#define CONFIG_ROOTFS2_SQUASHFS
#define CONFIG_ARG_QUIET
#define CONFIG_SPL_SERIAL_SUPPORT

/* 添加以下宏定义 */
#define CONFIG_BOOT_RTOS_OTA

#include "x2600_base_common.h"

#endif /* __X2600_BASE_H__ */
```

#####   1.1.2 编译uboot

- ​	配置编译uboot需要的环境变量

​	通过以下命令，确定对应的芯片的uboot编译工具链：

```shell
#获取对应的芯片的uboot的编译工具链版本
fy@hlf:~/work/x2670/build$ grep -nr "uboot" configs/x2600e_nor_5.10_defconfig
3:APP_uboot_toolchain_dir=../tools/toolchains/mips-gcc720-glibc229
4:APP_uboot_dir=../bootloader/uboot-x2000
5:APP_uboot_config=x2600e_base_xImage_sfc_nor

#获取工具的绝对路径
fy@hlf:~/work/x2670/build$ cd ../tools/toolchains/mips-gcc720-glibc229
fy@hlf:~/work/x2670/tools/toolchains/mips-gcc720-glibc229$ pwd
/home/fy/work/x2670/tools/toolchains/mips-gcc720-glibc229

#回到对应的uboot代码的目录下
fy@hlf:~/work/x2670/tools/toolchains/mips-gcc720-glibc229$ cd ../../../bootloader/uboot-x2000/

#配置编译uboot需要的工具链的路径，注意路径要到bin文件夹
fy@hlf:~/work/x2670/bootloader/uboot-x2000$ export PATH=$PATH:/home/fy/work/x2670/tools/toolchains/mips-gcc720-glibc229/bin

```

- ​	编译对应的uboot配置


```shell
#清除之前make生成的所有文件
[工程目录]/bootloader/uboot-x2000$ make distclean
#编译对应的板级的uboot配置，此处以x2600e的nor flash的Freertos系统为例
[工程目录]/bootloader/uboot-x2000$ make x2600e_base_rtos_sfc_nor
```

​	编译成功后会在[工程目录]/bootloader/uboot-x2000/spl目录下生成目标文件：

![](img/8.png)

#### 1.2 MMC存储介质

​	以配备emmc的x2000_iLock_v1.0开发板，主系统为Freertos的编译配置为例，编译需要支持rtos ota功能的uboot固件的步骤如下：

#####     1.2.1 修改对应的板级配置文件

​		对应的SPL的编译配置为x2000_base_rtos_mmc0，此配置对应的板级配置文件为x2000_base.h。

​		文件的路径如下：

```
[工程目录]/bootloader/uboot-x2000/include/configs/{board}.h

以x2000为例：[工程目录]/bootloader/uboot-x2000/include/configs/x2000_base.h
```

​		增加OTA的功能需要对x2000_base.h文件进行如下修改：

```c
#ifndef __X2000_BASE_H__
#define	__X2000_BASE_H__

#define CONFIG_ROOTFS_SQUASHFS
#define CONFIG_ROOTFS2_SQUASHFS
#define CONFIG_ARG_QUIET
#define CONFIG_SPL_SERIAL_SUPPORT

/* 添加以下宏定义 */
#define CONFIG_BOOT_RTOS_OTA

#include "x2000_base_common.h"

#endif /* __X2000_BASE_H__ */
```

 #####  1.2.2 修改分区表

​	分区表的路径如下：[工程目录]/bootloader/uboot-x2000/board/ingenic/芯片名_base/

​	 eg: /home/fy/work/x2670/bootloader/uboot-x2000/board/ingenic/x2000_base

-  主系统为Freertos系统的分区表：

  ```
  property:
      disk_size = 3728m
      gpt_header_lba = 512
      custom_signature = 0

  partition:
  	#name     =  start,   size, fstype
  	uboot     =     0m,     1m,
  	rtos_ota  =     1m,     1m, EMPTY
  	rtos      =     2m,     5m, EMPTY
  	ota       =     7m,     1m, EMPTY

  #fstype could be: LINUX_FS, FAT_FS, EMPTY
  ```

-  主系统为Linux系统的分区表：

  ```
  property:
      disk_size = 3728m
      gpt_header_lba = 512
      custom_signature = 0
  
  partition:
  	#name     =  start,   size, fstype
  	uboot     =     0m,     1m,
  	kernel    =     1m,     8m, EMPTY
  	rootfs	  =     9m,		50m, LINUX_FS
  	rtos_ota  =     59m,    1m, EMPTY
  	ota       =   	60m,    1m, EMPTY
  
  #fstype could be: LINUX_FS, FAT_FS, EMPTY
  ```

#####   1.2.3 编译uboot

- ​	配置编译uboot需要的环境变量

​	通过以下命令，确定对应的芯片的uboot编译工具链：

```shell
#获取对应的芯片的uboot的编译工具链版本
fy@hlf:~/work/x2670/build$ grep -nr "uboot" configs/x2000_mmc0_defconfig
2:APP_uboot_toolchain_dir=../tools/toolchains/mips-gcc720-glibc229
3:APP_uboot_dir=../bootloader/uboot-x2000
4:APP_uboot_config=x2000_base_xImage_mmc0

#获取工具的绝对路径
fy@hlf:~/work/x2670/build$ cd ../tools/toolchains/mips-gcc720-glibc229
fy@hlf:~/work/x2670/tools/toolchains/mips-gcc720-glibc229$ pwd
/home/fy/work/x2670/tools/toolchains/mips-gcc720-glibc229

#回到对应的uboot代码的目录下
fy@hlf:~/work/x2670/tools/toolchains/mips-gcc720-glibc229$ cd ../../../bootloader/uboot-x2000/

#配置编译uboot需要的工具链的路径，注意路径要到bin文件夹
fy@hlf:~/work/x2670/bootloader/uboot-x2000$ export PATH=$PATH:/home/fy/work/x2670/tools/toolchains/mips-gcc720-glibc229/bin

```

- ​	编译对应的uboot配置

```shell
#清除之前make生成的所有文件
[工程目录]/bootloader/uboot-x2000$ make distclean
#编译对应的板级的uboot配置，此处以x2000e的mmc flash的Freertos系统为例
[工程目录]/bootloader/uboot-x2000$ make x2000_base_rtos_mmc0
```

​	编译成功后会在[工程目录]/bootloader/uboot-x2000目录下生成目标文件：

![](img/26.png)

### 2 rtos_ota功能使用流程

<img src="img/1.jpg" style="zoom:25%;" />

### 3 具有ota升级功能的Freertos编译配置

#### 3.1 IConfigTool配置

以**x2600e_nor_defconfig**为例，打开IConfigTool工具，选择配置文件

![](img/31.png)

​	勾选以下配置：

- 选中**USB驱动**

![](img/18.png)

<img src="img/19.png" style="zoom:85%;" />

![](img/20.png)

- 选中**SFC驱动**,若为MMC介质不需要选中

![](img/21.png)

![](img/22.png)

- 选中**rtos的ota升级**

![](img/23.png)

![](img/24.png)

- 选中**watchdog（看门狗）**

![](img/9.png)

#### 3.2 增加ota升级代码

​	修改vendor/vendor.c文件：

```
#include <stdio.h>
#include <driver/ota.h>

void vendor_init(void *arg)
{
    ota_init();
    ota_work();
    ota_deinit();
}
```

#### 3.3 编译

​	1. 首先先要配置环境变量，再使用对应板级和存储介质的配置文件，如x2600的nor flash：

![](img/3.png)

​	2. 配置之后就可以make：

<img src="img/4.png" style="zoom:75%;" />

​	3.编译完成之后会在Freertos工程目录下生成zero.bin文件：

<img src="img/5.png" style="zoom:80%;" />



### 4 烧录

#### 		4.1 Freertos系统 作为主系统的烧录工具配置

##### 	4.1.1 MTD介质

​	此处以x2600e芯片，存储介质为nor flash为例：

<img src="img/25.png" style="zoom:85%;" />

<img src="img/12.png" style="zoom:85%;" />

<img src="img/6.png" style="zoom:85%;" />

<img src="img/7.png" style="zoom:85%;" />

- uboot 分区：放置支持rtos_ota功能的SPL镜像
- rtos_ota 分区：放置具有ota升级功能的Freertos系统镜像
- rtos 分区：此处用Freertos系统作为示例，放置用户的主系统
- ota 分区：存储ota升级相关的信息，用于SPL做是否进入具有ota升级功能的Freertos系统的判断条件。ota分区烧录的文件可以为一个空文件。

​	**设置完成之后可以将此配置另存为配置文件。**

注：分区的大小和分区的起始地址对齐flash的块大小。

##### 	4.1.2 MMC介质

​	此处以x2000e芯片，存储介质为mmc0为例：

<img src="img/27.png" style="zoom:85%;" />

<img src="img/28.png" style="zoom:85%;" />

<img src="img/29.png" style="zoom:85%;" />

- uboot 分区：放置支持rtos_ota功能的SPL镜像
- rtos_ota 分区：放置具有ota升级功能的Freertos系统镜像
- rtos 分区：此处用Freertos系统作为示例，放置用户的主系统
- ota 分区：存储ota升级相关的信息，用于SPL做是否进入具有ota升级功能的Freertos系统的判断条件。ota分区烧录的文件可以为一个空文件。

​	**设置完成之后可以将此配置另存为配置文件。**

注：分区的大小和分区的起始地址对齐flash的块大小。



#### 	4.2 Liunx 系统作为主系统的烧录工具配置

#####  4.2.1 MTD存储介质

​	此处以x2600e芯片，存储介质为nor flash为例：

<img src="img/25.png" style="zoom:85%;" />

<img src="img/12.png" style="zoom:85%;" />

<img src="img/10.png" style="zoom:85%;" />

<img src="img/11.png" style="zoom:85%;" />

- uboot 分区：放置支持rtos_ota功能的SPL镜像
- rtos_ota 分区：放置具有ota升级功能的Freertos系统镜像
- kernel 分区：用户的主系统， 此处为Linux系统，放置kernel镜像
- rootfs分区：用户的主系统， 此处为Linux系统，放置rootfs镜像
- ota 分区：存储ota升级相关的信息，用于SPL做是否进入具有ota升级功能的Freertos系统的判断条件。ota分区烧录的文件可以为一个空文件。

​	**设置完成之后可以将此配置另存为配置文件。**

注：建议将Linux系统的分区放置在rtos_ota之前，分区的大小和分区的起始地址对齐flash的块大小。



#####  4.2.2 MMC存储介质

​	此处以x2000e芯片，存储介质为mmc0为例：

<img src="img/27.png" style="zoom:85%;" />

<img src="img/28.png" style="zoom:85%;" />

<img src="img/30.png" style="zoom:85%;" />

- uboot 分区：放置支持rtos_ota功能的SPL镜像
- rtos_ota 分区：放置具有ota升级功能的Freertos系统镜像
- kernel 分区：用户的主系统， 此处为Linux系统，放置kernel镜像
- rootfs分区：用户的主系统， 此处为Linux系统，放置rootfs镜像
- ota 分区：存储ota升级相关的信息，用于SPL做是否进入具有ota升级功能的Freertos系统的判断条件。ota分区烧录的文件可以为一个空文件。

​	**设置完成之后可以将此配置另存为配置文件。**

注：建议将Linux系统的分区放置在rtos_ota之前，分区的大小和分区的起始地址对齐flash的块大小。



### 5 OTA升级的流程

#### 		5.1 进入具有ota升级功能的Freertos系统

​	修改ota分区中的内容为"ota:kernel2"

##### 	5.1.1 主系统为Freertos系统

​		 此处以x2600 nor flash 的Freertos系统作为主系统为例：

```shell
# mtd存储介质可用以下命令
$ nor_write 0x700000 0x6f 0x74 0x61 0x3a 0x6b 0x65 0x72 0x6e 0x65 0x6c 0x32
$ reset
$ nor_read 0x700000 0x20
```

##### 	5.1.2 主系统为Linux系统

```shell
$ cd /tmp
$ echo "ota:kernel2" > ota
#of对应的路径为ota分区所对应的设备名，此处为mmc设备
$ dd if=./ota of=/dev/mmcblk0p4 count=512 bs=1
```

​		做完以上操作，重启即可进入具有ota升级功能的Freertos系统。



#### 		5.2 ota的升级流程

<img src="img/2.jpg" style="zoom:25%;" />

#### 	5.3 ota升级过程中文件分包策略

​	在通过usb的cdc协议传输镜像的过程中，数据包大小是以flash的页大小为单位进行分包传输。传输过程中的每个包和包头都进行crc32校验，并将结果存储在包头中，设备端和上位机对包进行接收时，会对包内容和包头进行校验。



#### 	5.4 ota分区策略

​	更新分区中的镜像时，由上位机告知设备端对哪个分区进行升级，升级结束也由上位机告知设备端将ota分区中的信息改写。

​	不建议对uboot分区进行升级，一旦升级失败机器将无法启动。

​	目前采取的更新策略是由rtos_ota分区中的镜像对各个分区进行升级，因此若对rtos_ota分区进行升级，升级失败可能会导致rtos_ota分区中的镜像无法启动，无法使用ota升级功能。



#### 	5.5 退出具有ota升级功能的Freertos系统

​	修改ota分区中的内容不为"ota:kernel2"之后重启即可。



### 6 上位机的使用

​	上位机的代码位于Freertos工程下，目录为：[Freertos工程目录]/freertos/example/driver/ota

具体文件如下图：

![](img/13.png)

​	tty_send为编译生成的可执行文件。

在上述目录下，编译和使用上位机的代码：

```shell
#用gcc编译链编译脚本
$ gcc crc32.c pkt_api.c pkt.c tty_send_main.c usb_pkt.c -lpthread -o tty_send
#**********************************具体使用**********************************
#对指定分区进行ota升级
#./tty_send dev=[ota通讯的串口] file=[升级的文件名] partition=[升级的分区名]
$./tty_send dev=/dev/ttyACM0 file=zero.bin partition=rtos
#修改ota分区的信息，不为ota:kernel2, 这里以ota：kenrel为例。
#进行此操作之后，复位即可退出具有ota升级功能的Freertos系统。
$ echo "ota:kernel" > ota
$ ./tty_send dev=/dev/ttyACM0 file=ota partition=ota
# 复位芯片
$  ./tty_send dev=/dev/ttyACM0 reset

# 若ota后续还有其他需执行的代码，则可使用以下命令退出ota流程（执行后面的代码）
# 使用此命令之后，会退出ota流程，注销掉cdc的设备节点，上位机ota命令不可用
$  ./tty_send dev=/dev/ttyACM0 exit
```



### 7 ota升级过程打印信息

上位机：

<img src="img/14.png" style="zoom:75%;" />

设备端：

![](img/16.png)