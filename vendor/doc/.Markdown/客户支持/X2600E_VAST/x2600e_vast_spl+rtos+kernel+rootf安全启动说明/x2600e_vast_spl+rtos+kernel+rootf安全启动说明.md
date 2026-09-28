# spl + rtos + kernel + rootfs 安全启动实现

## 一 安全启动说明

- CPU 启动的时验证 SPL 的的签名，保证代码没有更改过。
  利⽤ RSA 的⾮对称加解特性, 私钥加密, 只有公钥可以解密. 把 SPL 的 HASH 值加密, 在启动的时候验通过公钥解密，检校 HASH 是否正确. 这样只有有私钥的才能使⽤, 防⽌代码发⽣变化.
- CPU启动时候使⽤ USERKEY 解密(可选)
  代码段可以通过 AES 加密. 启动过程中⽤ AES 解密, 保证⽤⼾的代码是加密的, 防⽌泄露. AES 的秘钥要与 CPU 内部烧录的秘钥相同.
- CPU 启动时候使⽤ CHIPKEY 解密(可选)
  代码是通过 CPU 的⾃⾝的 CHIPKEY 解密的, 每个CPU的 CHIPKEY 是不同的, ⽽通过这个 KEY 加密的 Firmware 也是不同的, 保证了 FLASH ⾥⾯的代码是⽆法 COPY 的

CPU 内部保存了3个 KEY:

- RSA的公钥签名,  保证公钥是和⽤⼾的私钥对应.
- USERKEY, 保证⽤⼾使⽤ OTA 时代码是加密的.
- CHIPKEY, 保证每个 CPU 的KEY是不同的.

这三个 KEY 是保存 CPU 内部 EFUSE 中, 在⾮安全核不可读取, 对外不开放. 在使⽤安全 BOOT 前需要烧录, 并打开security功能的BIT位.

具体加密流程如下：

![1](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/1.png)

> **对于 rootfs，采用只签名，不加密的方式**

## 二 软硬件环境说明

本文以 PD_X2600E_VAST_V2.0 开发板为例进行说明

### 2.1 硬件环境说明

X2600系列芯片不同于x1600e及x2000系列, efuse可以长供电，不需要外接io控制, 本开发板电路

![1](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/2.png)





### 2.2 软件环境说明

需修改 uboot 工程中的文件,本开发板使用配置x2600e_vast_v20_nand_5.10_factory_defconfig

```c
bhu@bhu-PC:~/work/build$ grep -nr  "uboot_config" configs/x2600e_vast_v20_nand_5.10_factory_defconfig
5:APP_uboot_config=x2600e_base_xImage_sfc_nand                     //uboot配置
```

查找/Linux工程目录/bootloader/uboot-x2000/boards.cfg内的x2600e_base_xImage_sfc_nand

```c
x2600e_base_xImage_sfc_nand		mips        xburst2     x2600_base            ingenic        x2600         x2600_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_BOOT,SPL_PARAMS_FIXER,X2600E_DDR,RMEM_MB=16
```

可以看到uboot使用的为x2600_base, 位于bootloader/uboot-x2000/include/configs下的x2600_base.h文件, 改动后内容如下：

```c
#ifndef __X2600_BASE_H__
#define	__X2600_BASE_H__

#define CONFIG_ROOTFS_SQUASHFS
#define CONFIG_ROOTFS2_SQUASHFS
#define CONFIG_ARG_QUIET
#define CONFIG_SPL_SERIAL_SUPPORT
/* 添加以下 4 个宏 */
#define CONFIG_JZ_SCBOOT 
#define CONFIG_JZ_SECURE_SUPPORT 
#define CONFIG_SPL_RTOS_LOAD_KERNEL 
#define CONFIG_JZ_SECURE_ROOTFS

#include "x2600_base_common.h"

#endif /* __X2600_BASE_H__ */
```

本文默认使用的是nand,nor也可以支持, 但是对于 nor 介质来说，rtos 不能挂载文件系统，因为 rtos 和 linux 的文件系统格式不一样，如果在 rtos 挂载文件系统，会破坏 linux 文件系统的分区（需在 rtos 的 iconfig 中取消掉文件系统勾选）

![2](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/3.png)

对于如何使用 spl 启动 rtos 去 load kernel，查看文档《提前初始化屏幕显示logo》

## 三 签名和加密

> **注：本文档描述的功能只适用于 cloner-2.5.40-ubuntu_alpha/windows_alpha 以上的烧录工具**
>
> **x1600、x2000、x2600使用的工具一样,都在securitytool/x2000目录下**

### 3.1 生成秘钥

生成秘钥的工具在烧录工具目录的 `securitytool/x2000/keytool/` 下：

![2](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/4.png) 

其中 genkey_32 对于 32 位 pc，genkey_64 对应 64 位 pc

运行：`./genkey_64`

![2](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/5.png)

点击 Generate Key 后就会在当前路径下生成秘钥，也可以点击 Path 选择生成的路径。

如下会生成 key.bin，pri_key.pem ，user_key.bin

![image-20231114163232774](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/6.png) 

将秘钥拷贝到烧录工具的对应位置

- 将 key.bin 拷贝到烧录工具 `security/x2000/`下：

    ![image-20231114163443801](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/7.png) 

- 将 pri_key.pem 和 user_key.bin 拷贝到烧录工具的 `securitytool/x2000/sigtool/security_key`下：

    ![image-20231114163609562](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/8.png) 

    ![image-20231114163709839](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/9.png) 

### 3.2 签名和加密的工具

签名工具在烧录工具目录的 `securitytool/x2000/sigtool/` 下：

![image-20231114163903973](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/10.png) 

其中 sigtool-32、sigtool-64、sigtool.exe 为对应 pc 下使用的工具

创建文件**settings.ini** , 写入内容

```shell
[debug]
value=1
```

运行：`./sigtool-64`

![image-20231114164220755](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/11.png) 

其中 AES 选上为启用加密功能，不选为只签名。

### 3.3 签名和加密

> **在对镜像进⾏加密之前, 先烧录⼀次未加密过的镜像, 待spl能加载kernel, kernel挂载⽂件系统, ⽂件系统起来后，确定镜像可⽤即可进⾏签名镜像操作**

#### 3.3.1 burn bin

操作如图：

![image-20231114164957025](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/12.png) 

选择好后点击 sign，就会在烧录工具目录下的 `firmwares/x2600/`目录下生成spl_sec.bin 和 uboot_sec.bin

#### 3.3.2 spl bin

操作如图：

![image-20231114165414660](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/13.png) 

选择好后点击 sign，就会在对应的目录下生成加密后的文件，对于上面的文件就会生成 `u-boot-spl-pad-dst.bin`，等会烧录 spl 时要选择这个文件

#### 3.3.3 kernel bin

操作如图：

![image-20231114165846882](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/14.png) 

选择好后点击 sign，就会在对应的目录下生成加密后的文件，对于上面的文件就会生成 `xImage-dst.bin`，等会烧录 kernel 时要选择这个文件。

#### 3.3.4 rootfs

**对于 rootfs ，采用不加密，只签名. 具体操作如下：**

![image-20231114170228186](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/15.png) 

选择好后点击 sign，就会在对应的目录下生成加密后的文件，对于上面的文件就会生成 `rootfs-dst.squashfs`，然后**使用 dd 命令分离出签名信息**（等会需要将该签名信息烧录到 flash 中）：

```shell
dd if=rootfs-dst.squashfs of=signature bs=2048 count=1
```

如图，signature 就是签名信息，等会需要烧录这个文件：

![image-20231114170605692](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/16.png)  

#### 3.3.5 rtos bin

操作如下：

![image-20231114170921810](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/17.png) 

选择好后点击 sign，就会在对应的目录下生成加密后的文件，对于上面的文件就会生成 `zero-dst.bin`，等会烧录 rtos 时要选择这个文件

## 四 烧录

本文档使用 cloner-2.5.40-ubuntu_alpha 烧录工具：

### 4.1 选择配置文件

![image-20231114180814239](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/18.png)

### 4.2 分区配置

![image-20231114171413317](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/19.png) 

### 4.3 启用安全烧录

![](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/20.png) 

### 4.4 选择烧录文件

![image-20231114180936015](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/21.png) 

## 五 问题汇总

**1. 烧录到boot 40%变红** 

可能是cpu烧坏了, 需要重新更换cpu.

efuse烧录有严格的供电要求, 写⼊efuse时, 给efuse的供电时间不能超过200ms, ⼀旦超过200ms就会烧坏efuse, 导致整个芯⽚不能正常使⽤, 此时就需要更换主控芯⽚了

**2 烧录到boot 90% SEND KU ERR 变红**

直接表现是烧录⼯具烧录之前解码固件失败, 根本原因应该是efuse因为某些原因没有写⼊成功. 

此时应当检查efuse的供电及使能电平, 确保硬件设计和烧录⼯具设置的⼀致. 排除此处原因后, 重新烧录可以成功的.



**3 换其他烧录⼯具烧录后系统⽆法启动** 

由于efuse中数据位跳变是不可逆的, 所以⼀定要保证⽤此加密烧录后的系统再次更新还是使⽤同样的烧录⼯具和密钥来完成的. 如果使⽤新的烧录⼯具烧录了其他的密钥, 那么再⽤原先的烧录⼯具也⽆法再次烧录正确的密钥了, 引起系统可能再也启动不了了. 到此步, 只能更换主控芯⽚才能再次使⽤了. 

## 六 系统解密流程

![22](x2600e_vast_spl+rtos+kernel+rootf安全启动说明.assets/22.png)

