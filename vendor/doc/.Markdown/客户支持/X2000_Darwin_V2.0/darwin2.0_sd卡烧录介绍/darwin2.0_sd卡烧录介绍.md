## 																		                                                                                                                                                                                             darwin2.0_sd卡烧录介绍



### 一  什么是sd卡烧录

这里sd卡烧录指的是利用pc上制作好的专用sd卡对板子上的nand/nor进行烧录。
一张sd卡可以重复使用,去烧录多台设备,以达到代替usb烧录的目的。优点是不需要pc机和烧录线这些，烧录更简便、灵活，减少对烧录设备的依赖。

### 二  sd卡烧录的原理
#### 1  烧录工具的相关烧录原理

君正的软件多数使用usb烧录工具进行烧录,导致烧录工具根据烧录工具的配置文件自动做了以下事情：

1. 1  烧录当前主控芯片的ddr类型到nand/nor的固定地址中,烧录时决定ddr类型。

1. 2  烧录nand/nor分区表到nand/nor的固定地址中,烧录时决定分区表。

#### 2  烧录工具的合并镜像原理

​	烧录工具有合并镜像的功能,可以把ddr参数和分区表写入到新生成的镜像中。sd卡烧录时可以直接此带有完整ddr参数和分区表的镜像写入nand/nor的固定地址，从而实现sd卡烧录。

利用**合并镜像**的功能我们可以得到一个带ddr类型参数和分区信息的完整的image.bin,从而为卡烧做好准备。

#### 3  卡烧的实现原理

本例使用两套代码，分别编译Darwin v2.0板子的sd卡启动镜像和 nand/nor 启动镜像。sd卡启动的镜像负责从sd卡启动基础系统, nand/nor的启动镜像则保存在sd卡的数据分区中。

sd卡启动的基础系统通过配置自带nand/nor驱动,从而有了操作nand/nor设备的可能。

通过烧录工具的合并镜像功能，我们可以得到一个带ddr类型参数和分区信息的nand/nor 启动的完整镜像image.bin。mmc启动后通过启动卡烧脚本来实现将此nand/nor 启动的完整镜像image.bin、xImage、rootfs.squashfs 通过mtd命令写入nand/nor中。从而实现卡烧录。

硬件上BOOT_SEL2/1/0   的状态**要能保证在nand/nor为空**的时候，主控可以进入卡启动模式。而在nand/nor不为空的时候，则可以从nand/nor启动。本例使用君正开发板Darwin_X2000_V2.0。

### 三 sd卡烧录spi nand的实现

#### 1 制作sd 卡升级镜像

这里我们以Darwin_X2000_V2.0为例子,根据硬件设计，sd卡在mmc2上。

**注意：**由于sd卡的镜像也需要我们编译,待
待升级的镜像也需要编译, 为了避免混淆,可以拷贝或者重新同步一份代码编译sd卡镜像,然后另一份编译nand镜像。

##### 1.1  选择mmc2配置, 使能sdcard_update升级脚本

打开IConfigTool工具选择configs/x2000_darwin_mmc2_defconfig进行修改 

<img src="darwin2.0_sd卡烧录介绍.assets/1.png" alt="1" style="zoom:150%;" />

打开 [ 开机启动升级/烧录程序 (一般是sd卡烧录 nand/nor) ] 

<img src="darwin2.0_sd卡烧录介绍.assets/2.png" alt="2" style="zoom:150%;" />

 如果需要自定义升级脚本,可以更改升级脚本的路径 (相对于
buildroot/buildroot_patch目录)

<img src="darwin2.0_sd卡烧录介绍.assets/3.png" alt="3" style="zoom:150%;" />

最后按 ctrl-s 保存上述配置，并编译

```
sxyzhang@T430:~/test2000_sz_linux/build$ make x2000_darwin_mmc2_defconfig
sxyzhang@T430:~/test2000_sz_linux/build$ make
```

##### 1.2  使能kernel sfc

###### 1.2.1 make menuconfig

在make x2000_darwin_mmc2_defconfig 之后可以进入kenrel目录对kernel进行配置

```
cd ../kernel/kernel-x2000/
make menuconfig
```

###### 1.2.2 选择nand配置

```
  -> Ingenic device-drivers Configurations     
     -> [SFC] (SPI Nand/Nor Flash) Drivers      
      -> Ingenic series SFC driver
```

注意需要将所有选项与下图一致. 如果客户项目中用到nand的保留空间,那么建议选上
"ingenic SN and MAC read write support."

<img src="darwin2.0_sd卡烧录介绍.assets/4.png" alt="4" style="zoom:150%;" />

###### 1.2.3 保存配置

make menuconfig 保存后, kernel的配置保存在kernel/kernel-x2000/.config 中。再次执行 make x2000_darwin_mmc2_defconfig 会覆盖掉这个当前配置文件。**建议保存到configs/x2000_darwin_mmc2_defconfig中指定的kernel配置文件中，以免误操作丢失kernel当前配置。**

本例中configs/x2000_darwin_mmc2_defconfig中指定的kernel配置为：x2000_darwin_module_base_linux_mmc2_defconfig， 因此执行如下命令来保存kernel上述设置：

```
cp .config arch/mips/configs/x2000_darwin_module_base_linux_mmc2_defconfig
```

##### 1.3 修改 kernel dts 文件

sfc 驱动不仅要使能sfc 配置,而且需要修改dts文件。

###### 1.3.1 找到当前kernel加载的dts文件

<img src="darwin2.0_sd卡烧录介绍.assets/5.png" alt="5" style="zoom:150%;" />

可知当前kernel加载的dts文件为：module_drivers/dts/x2000_darwin_module_base_mmc2.dts

###### 1.3.2 修改当前dts

直接将如下代码复制到 上述dts文件中

```
&sfc {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&sfc_pe>;
    ingenic,sfc-init-frequency = <200000000>;
    ingenic,sfc-max-frequency = <200000000>;
    ingenic,use_ofpart_info  = /bits/ 8 <1>;
    ingenic,spiflash_param_offset = <0>;
    norflash@0 {
        partitions {
            compatible = "fixed-partitions";
            #address-cells = <1>;
            #size-cells = <1>;
            /* spi nor flash partition */
            partition0 {
                label = "uboot";
                reg = <0x0000000 0x40000>;
                /*read-only;*/
            };
        };
    };
    nandflash@1 {
        partitions {
            compatible = "fixed-partitions";
            #address-cells = <1>;
            #size-cells = <1>;
            /* spi nand flash partition */
            partition0 {
                label = "uboot";
                reg = <0x0000000 0x100000>;
                /*read-only;*/
            };
        };
    };
};
```

##### 1.4  编译生成mmc2启动的镜像

```
cd ../../build/ # 从kernel/kernel-x2000 ⽬录返回
make x2000_darwin_mmc2_defconfig
make
```

至此，生成来mmc2启动的镜像文件

```
ls output
# rootfs.squashfs u-boot-with-spl-mbr-gpt.bin xImage
```

##### 1.5 使用烧录工具合并mmc2启动镜像

由于烧录工具合并镜像功能在之前的版本会有bug,所以请保证烧录工具版本不低于2.5.28。

如下链接可以下载最新版本的烧录工具：

http://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz

http://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windows.zip

如下图所示在烧录工具的配置界面点击 merge (合并镜像)按钮,将 uboot、 kernel、 rootfs 合并成一个镜像 image.bin，存放在mmc启动编译代码的build/output/下。

**注意：**info offset (参数偏移) 的位置要填 0x4400 (17K)
。

<img src="darwin2.0_sd卡烧录介绍.assets/6.png" alt="6" style="zoom:150%;" />



<img src="darwin2.0_sd卡烧录介绍.assets/7.png" alt="7" style="zoom:150%;" />



<img src="darwin2.0_sd卡烧录介绍.assets/8.png" alt="8" style="zoom:150%;" />

合并的产生的镜像是 image.bin,存放在mmc启动编译代码的build/output/下。

###### <img src="darwin2.0_sd卡烧录介绍.assets/9.png" alt="9" style="zoom:150%;" />

##### 1.6 将合并后的mmc2启动镜像写入sd卡

###### 1.6.1 确认sd卡设备⽂件路径  

使用读卡器将sd卡插入电脑,使用 ls /dev/sd* 命令查看所有的磁盘设备。

<img src="darwin2.0_sd卡烧录介绍.assets/10.png" alt="10" style="zoom:150%;" />

一般来说最后插入的磁盘的设备字母序号排在最后,所以我的电脑是 /dev/sdb。 然后把读卡器拔掉,再次 ls /dev/sd* 命令查看所有的磁盘设备

<img src="darwin2.0_sd卡烧录介绍.assets/11.png" alt="11" style="zoom:150%;" />

如上图所示 /dev/sdb 没有了,再次插入读卡器,再次 ls /dev/sd*

<img src="darwin2.0_sd卡烧录介绍.assets/10.png" alt="10" style="zoom:150%;" />

这样反复确认之后,我的电脑当前sd卡设备路径是 /dev/sdb。

**此处一定要反复确认，以免误操作将PC机的磁盘格式化掉，后果可能非常严重。**

 **即使同一台电脑也不要掉以轻心,下次插入的时候可能设备号就变了,同样需要反复确认。**

###### 1.6.2 dd命令写⼊sd卡镜像

如上面所说我们要自行找到sd卡设备路径,比如/dev/sdb。 为了防止复制导致的灾难,
这里我们用/dev/sdX 代替,请自行替换.

将1.5章节中合并生成的image.bin通过如下命令写入sd卡中。

```
sudo dd if=image.bin of=/dev/sdX bs=64k
sync
```

###### 1.6.3 创建 ext4 ⽂件系统

dd命令写入sd卡之后,将读卡器拔掉,再插入,这个时候会重新识别sd卡上uboot自带的
分区表

<img src="darwin2.0_sd卡烧录介绍.assets/12.png" alt="12" style="zoom:150%;" />

此时我们可以通过 parted 命令查看对应的分区表（ 如果pc没有parted 命令,可以安装sudo apt-get install parted）

<img src="darwin2.0_sd卡烧录介绍.assets/13.png" alt="13" style="zoom:150%;" />

所以我们知道userdata分区对应的是序号是6,对应于/dev/sdb6。 **再次强调**,这是我
的电脑此时的场景,一定要反复确认设备号。

这里开始将第6个分区创建为ext4文件系统

```
# 如果pc没有mkfs.ext4 命令,可以安装
#  sudo apt-get install e2fsprogs
mkdir userdata
sudo mkfs.ext4 /dev/sdX6
sudo mount /dev/sdX6 userdata/
sudo chmod -R 777 userdata/
sudo umount userdata/
sync
```

至此，烧录卡sd卡已经制作完成，稍后我们再拷贝要升级的镜像到ext4 文件系统中即可。

#### 2 准备nand启动的镜像 

##### 2.1 编译生成nand启动的镜像

另外准备一份代码，编译待烧录的nand启动镜像

```
sxyzhang@T430:~/x2000_sz/build$ make x2000_darwin_factory_defconfig
sxyzhang@T430:~/x2000_sz/build$ make
```

对应生成如下文件：

```
sxyzhang@T430:~/my/work/linux/x2000_sz/build$ ls output/
rootfs.squashfs  u-boot-spl-pad.bin  xImage
```

##### 2.2 使用烧录工具合成带有ddr参数和分区信息的nand启动uboot镜像

前面有说过烧录工具在烧录的时候会将ddr,分区表等信息烧录到uboot分区中，所以这里需要用到烧录工具的合并镜像功能合并出一个与usb烧录之后无异的uboot.bin。

首先，需要在配置中选择与板子上对应的cpu型号。

<img src="darwin2.0_sd卡烧录介绍.assets/14.png" alt="14" style="zoom:150%;" />

其次，nand需要注意分区表中rootfs分区的类型,必须
修改为MTD_MODE,因为不是ubi文件系统。如下图所示。

<img src="darwin2.0_sd卡烧录介绍.assets/15.png" alt="15" style="zoom:150%;" />

最后，需要选择待合并的nand启动的uboot镜像文件及设置info offset (参数偏移), x2000和
x1600的info offset (参数偏移)都是0x5800。此处指定的0x5800是指要在nand启动的u-boot-spl-pad.bin的0x5800地址处写入烧录工具当前设置的cpu型号对应的ddr参数及“SFC--》分区信息”中指定的分区信息。因此，此处烧录工具的“INFO--》板级”和“SFC--》分区信息”一定要设置对。

<img src="darwin2.0_sd卡烧录介绍.assets/16.png" alt="16" style="zoom:150%;" />

点击“合并镜像后”会在nand启动的uboot镜像的同目录生成合并后的文件：image.bin

<img src="darwin2.0_sd卡烧录介绍.assets/17.png" alt="17" style="zoom:150%;" />

<img src="darwin2.0_sd卡烧录介绍.assets/18.png" alt="18" style="zoom:150%;" />

#### 3 将nand 启动镜像拷贝到sd卡

依次执行如下命令，实现将nand启动的镜像拷贝到sd卡中，完成制作sd烧录卡的最后一步。

```
将sd卡插入pc
cd build                                               # 进入nand启动代码的编译目录
mkdir userdata
sudo mount /dev/sdX6 userdata/
sudo chmod -R 777 userdata/
mkdir -p userdata/sdcard_update/
touch userdata/sdcard_update/enable_update
cp output/image.bin userdata/sdcard_update/uboot.bin   # 一定要将合并后的uboot镜像拷贝并更名为uboot.bin
cp output/xImage userdata/sdcard_update/
cp output/rootfs.squashfs userdata/sdcard_update/
sync
sudo umount userdata/
```

此时拔掉sd卡, 插入开发板就可以进行sd卡烧录了,升级打印如下：

```
detect mtd partitions
20+0 records in
20+0 records out
24556+0 records in
24556+0 records out
mtd erase uboot /dev/mtd0
mtd write uboot /dev/mtd0 /tmp/uboot.bin.2
[    2.565058] flash: 88284000
[    2.565258] ingenic-sfc 13440000.sfc: Use burner params.
[    2.565746] ingenic-sfc 13440000.sfc: magic is 0x726f6e  version 
is 0x2
[    2.566183] num_partition_info: 3
[    2.566410] Creating 2 MTD partitions on "sfc_mtd":
[    2.566736] 0x000000040000-0x000000340000 : "kernel"
[    2.589109] 0x000000340000-0x000001000000 : "rootfs"
mtd erase kernel /dev/mtd1
mtd write kernel /dev/mtd1 /usr/data/sdcard_update/xImage
mtd erase rootfs /dev/mtd2
mtd write rootfs /dev/mtd2 /usr/data/sdcard_update/rootfs.squashfs
mtd erase uboot /dev/mtd0
mtd write uboot /dev/mtd0 /usr/data/sdcard_update/uboot.bin
mtd write image ok!
```

若升级成功，弹出sd卡后重启系统，则会从nand启动，从串口打印就能确认。至此，sd卡烧录spi nand的流程全部完成。

### 四 sd卡烧录spi nor 的实现

#### 1 制作sd 卡升级镜像

这里我们以Darwin_X2000_V2.0为例子,将默认贴的spi nand换成spi nor   GD25Q127CSIG。根据硬件设计，sd卡在mmc2上。

**注意**：由于sd卡的镜像也需要我们编译,待 待升级的镜像也需要编译, 为了避免混淆,
可以拷贝或者重新同步一份代码编译sd卡镜像,然后另一份编译spi nor启动镜像。

##### 1.1  选择mmc2配置, 使能sdcard_update升级脚本

打开IConfigTool工具选择configs/x2000_darwin_mmc2_defconfig进行修改

<img src="darwin2.0_sd卡烧录介绍.assets/1.png" alt="1" style="zoom:150%;" />

打开 [ 开机启动升级/烧录程序 (一般是sd卡烧录 nand/nor) ]

<img src="darwin2.0_sd卡烧录介绍.assets/2.png" alt="2" style="zoom:150%;" />

如果需要自定义升级脚本,可以更改升级脚本的路径 (相对于
buildroot/buildroot_patch目录)

<img src="darwin2.0_sd卡烧录介绍.assets/3.png" alt="3" style="zoom:150%;" />

最后按 ctrl-s 保存上述配置，并编译

```
sxyzhang@T430:~/test2000_sz_linux/build$ make x2000_darwin_mmc2_defconfig
sxyzhang@T430:~/test2000_sz_linux/build$ make
```

##### 1.2  使能kernel sfc

###### 1.2.1 make menuconfig  

在make x2000_darwin_mmc2_defconfig 之后可以进入kenrel目录对kernel进
行配置

```
cd ../kernel/kernel-x2000/
make menuconfig
```

###### 1.2.2 选择nor配置

```
   -> Ingenic device-drivers Configurations     
     -> [SFC] (SPI Nand/Nor Flash) Drivers      
      -> SFC driver for Ingenic series SoCs
```

注意需要将所有选项与下二图一致

<img src="darwin2.0_sd卡烧录介绍.assets/19.png" alt="19" style="zoom:150%;" />

<img src="darwin2.0_sd卡烧录介绍.assets/19-1.png" alt="19-1" style="zoom:150%;" />

###### 1.2.3 添加对应的nor选项及参数

如果上图中有对应的nor具体型号选项，则跳过此1.2.3节，直接进入1.2.4节。

上图中牵涉到当前nor具体型号的选择。如果此处没有当前硬件nor具体型号选项，则需要在kernel中添加。具体步骤如下。

```
sxyzhang@T430:~/test2000_sz_linux/kernel/kernel-x2000$ vim module_drivers/drivers/mtd/devices/Kconfig
```

<img src="darwin2.0_sd卡烧录介绍.assets/19-2.png" alt="19-2" style="zoom:150%;" />

参照如上图nor列表的方式添加自己的当前nor 所使用的宏，此例中为：INGENIC_YOUR_NOR_NAME。

```
sxyzhang@T430:~/test2000_sz_linux/kernel/kernel-x2000$ vim module_drivers/drivers/mtd/devices/ingenic_sfc_v2/nor_device/nor_device.c
```

在结构体nor_params_node中加入对应的nor 参数，注意用上面Kconfig中添加的宏来隔开。此处就不举例了。

<img src="darwin2.0_sd卡烧录介绍.assets/19-3.png" alt="19-3" style="zoom:150%;" />

再在get_cdt_params（）中添加chip id和name索引的case接口，实现如下：

<img src="darwin2.0_sd卡烧录介绍.assets/19-4.png" alt="19-4" style="zoom:150%;" />

至此，请跳至“**1.2  使能kernel sfc**”章节继续执行。

###### 1.2.4 保存配置

make menuconfig 保存后, kernel的配置保存在kernel/kernel-x2000/.config 中。再次执行 make x2000_darwin_mmc2_defconfig 会覆盖掉这个当前配置文件。

建议保存到configs/x2000_darwin_mmc2_defconfig中指定的kernel配置文件中，以免
误操作丢失kernel当前配置。

本例中configs/x2000_darwin_mmc2_defconfig中指定的kernel配置为：
x2000_darwin_module_base_linux_mmc2_defconfig， 因此执行如下命令来保存
kernel上述设置：

```
cp .config 
arch/mips/configs/x2000_darwin_module_base_linux_mmc2_defconfig
```

##### 1.3 修改 kernel dts 文件

sfc 驱动不仅要使能sfc 配置,而且需要修改dts文件。

###### 1.3.1 找到当前kernel加载的dts⽂件

<img src="darwin2.0_sd卡烧录介绍.assets/5.png" alt="5" style="zoom:150%;" />

可知当前kernel加载的dts文件为：
module_drivers/dts/x2000_darwin_module_base_mmc2.dts

###### 1.3.2 修改当前dts

直接将如下代码复制到 上述dts文件中

```
&sfc {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&sfc_pe>;
    ingenic,sfc-init-frequency = <200000000>;
    ingenic,sfc-max-frequency = <200000000>;
    ingenic,use_ofpart_info  = /bits/ 8 <1>;
    ingenic,spiflash_param_offset = <0>;
    norflash@0 {
        partitions {
            compatible = "fixed-partitions";
            #address-cells = <1>;
            #size-cells = <1>;
            /* spi nor flash partition */
            partition0 {                label = "uboot";
                reg = <0x0000000 0x40000>;
                /*read-only;*/
            };
        };
    };
    nandflash@1 {
        partitions {
            compatible = "fixed-partitions";
            #address-cells = <1>;
            #size-cells = <1>;
            /* spi nand flash partition */
            partition0 {
                label = "uboot";
                reg = <0x0000000 0x100000>;
                /*read-only;*/
            };
        };
    };
};
```

##### 1.4 修改 nor 驱动

本例中具体驱动代码为：

kernel/kernel-x2000/module_drivers/drivers/mtd/devices/ingenic_sfc_v2/nor_device/nor_device.c。 

 一定要耐心对照烧录工具和驱动代码，确保驱动代码中的相关参数和烧录工具保持一致。这里用的是GD25Q127CSIG，打开烧录工具的norinfo列表,找到GD25Q127CSIG这一项。

<img src="darwin2.0_sd卡烧录介绍.assets/20.png" alt="20" style="zoom:150%;" />

###### 1.4.1  修改驱动代码中的nor_params_node 结构体

<img src="darwin2.0_sd卡烧录介绍.assets/21.png" alt="21" style="zoom:150%;" />

选中具体型号的nor flash信息,点击 “修改”按钮来查看信息。根据烧录工具中此处nor参数信息，修改kernel中nor驱动参数。

<img src="darwin2.0_sd卡烧录介绍.assets/22.png" alt="22" style="zoom: 200%;" />

根据上图烧录工具中GD25Q127CSIG  nor详细参数，重点修改本文件中的 struct nor_params_node nor_params_node 结构体如下。

注：**一定要保证代码中的以下参数与上图烧录工具中参数一致**。

```
static struct nor_params_node nor_params_node[] = {
    /*GD25Q128C*/
    {
	    .nor_device_info.name = "GD25Q127C",
        .nor_device_info.id = 0xc84018,

        .nor_device_info.tCHSH = 5,
        .nor_device_info.tSLCH = 5,
        .nor_device_info.tSHSL_RD = 20,
        .nor_device_info.tSHSL_WR = 20,

        .nor_device_info.chip_size = 16777216,
        .nor_device_info.page_size = 256,
        .nor_device_info.erase_size = 32768,

        .nor_device_info.quad_ops_mode = 1,
        .nor_device_info.addr_ops_mode = 0,
    },
```

###### 1.4.2 修改驱动代码中的get_cdt_params（）函数

<img src="darwin2.0_sd卡烧录介绍.assets/23.png" alt="23" style="zoom:200%;" />

根据烧录工具上图“命令信息”选项的左侧各命令的时序，在驱动代码 struct spi_nor_info *get_cdt_params 函数中实现如下部分：

```
static struct spi_nor_info *get_cdt_params(struct sfc_flash *flash, struct spi_nor_info *nor_device_info)
{
    switch(nor_device_info->id) {
        case 0xc84018:
        if(!(strcmp(nor_device_info->name, "GD25Q127C"))) {
            CDT_PARAMS_INIT_COMMON(nor_device_info, 3);
            /* quad set     0x31 1 1 1 1 0 */
            ST_INFO(nor_device_info->quad_set, SPINOR_OP_WRSR_1, 1, 1, 1, 1, TM_STD_SPI);
            /* quad get     0x35 1 1 1 1 0 */
            ST_INFO(nor_device_info->quad_get, SPINOR_OP_RDSR_1, 1, 1, 1, 1, TM_STD_SPI);
            return nor_device_info;
            }
        break;
```

上面 CDT_PARAMS_INIT_COMMON(nor_device_info, 3)的位置也可以是CDT_PARAMS_INIT_SPECIAL(nor_device_info)。

当nor的容量超过16M Byte时，CDT_PARAMS_INIT_COMMON的addr_byte填4,否则填3。 CDT_PARAMS_INIT_SPECIAL 的 addr_byte默认是4，不可修改。

具体使用CDT_PARAMS_INIT_COMMON 还是CDT_PARAMS_INIT_SPECIAL，以及CDT_PARAMS_INIT_COMMON(nor_device_info, 3)中第二个参数到底是几，都是根据本节中烧录工具此款nor 的各命令时序确定的。

注：**要确保驱动代码中的各命令时序与烧录工具中的各命令时序参数一致。**

其中，CDT_PARAMS_INIT_COMMON 和 CDT_PARAMS_INIT_SPECIAL 的定义在
module_drivers/drivers/mtd/devices/ingenic_sfc_v2/nor_device/nor_device.h
中，如下图。

<img src="darwin2.0_sd卡烧录介绍.assets/24.png" alt="24" style="zoom:150%;" />

##### 1.5  编译生成mmc2启动的镜像

```
cd ../../build/                     # 从kernel/kernel-x2000 ⽬录返回
make x2000_darwin_mmc2_defconfig
make
```

至此，生成来mmc2启动的镜像文件

```
ls output
# rootfs.squashfs u-boot-with-spl-mbr-gpt.bin xImage
```

##### 1.6 使用烧录工具合并mmc2启动镜像

由于烧录工具合并镜像功能在之前的版本会有bug,所以请保证烧录工具版本不低于
2.5.28。

如下链接可以下载最新版本的烧录工具：

http://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubunt
u.tar.gz

http://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windo
ws.zip

如下图所示在烧录工具的配置界面点击 merge (合并镜像)按钮,将 uboot、 kernel、
rootfs 合并成一个镜像 image.bin，存放在mmc启动编译代码的build/output/下。

**注意：**info offset (参数偏移) 的位置要填 0x4400 (17K) 。

<img src="darwin2.0_sd卡烧录介绍.assets/6.png" alt="6" style="zoom:150%;" />



<img src="darwin2.0_sd卡烧录介绍.assets/7.png" alt="7" style="zoom:150%;" />



<img src="darwin2.0_sd卡烧录介绍.assets/8.png" alt="8" style="zoom:150%;" />

合并的产生的镜像是 image.bin,存放在mmc启动编译代码的build/output/下。

<img src="darwin2.0_sd卡烧录介绍.assets/9.png" alt="9" style="zoom:150%;" />

##### 1.7 将合并后的mmc2启动镜像写入sd卡

###### 1.7.1 确认sd卡设备⽂件路径  

使用读卡器将sd卡插入电脑,使用 ls /dev/sd* 命令查看所有的磁盘设备。

<img src="darwin2.0_sd卡烧录介绍.assets/10.png" alt="10" style="zoom:150%;" />

一般来说最后插入的磁盘的设备字母序号排在最后,所以我的电脑是 /dev/sdb。 然后把读卡器拔掉,再次 ls /dev/sd* 命令查看所有的磁盘设备

<img src="darwin2.0_sd卡烧录介绍.assets/11.png" alt="11" style="zoom:150%;" />

如上图所示 /dev/sdb 没有了,再次插入读卡器,再次 ls /dev/sd*

<img src="darwin2.0_sd卡烧录介绍.assets/10.png" alt="10" style="zoom:150%;" />

这样反复确认之后,我的电脑当前sd卡设备路径是 /dev/sdb。

**此处一定要反复确认，以免误操作将PC机的磁盘格式化掉，后果可能非常严重。**

 **即使同一台电脑也不要掉以轻心,下次插入的时候可能设备号就变了,同样需要反复确认。**

###### 1.7.2  dd命令写⼊sd卡镜像

如上面所说我们要自行找到sd卡设备路径,比如/dev/sdb。 为了防止复制导致的灾难,
这里我们用/dev/sdX 代替,请自行替换.

将1.5章节中合并生成的image.bin通过如下命令写入sd卡中。

```
sudo dd if=image.bin of=/dev/sdX bs=64k
sync
```

###### 1.7.3 创建 ext4 ⽂件系统

dd命令写入sd卡之后,将读卡器拔掉,再插入,这个时候会重新识别sd卡上uboot自带的
分区表

<img src="darwin2.0_sd卡烧录介绍.assets/12.png" alt="12" style="zoom:150%;" />

此时我们可以通过 parted 命令查看对应的分区表（ 如果pc没有parted 命令,可以安装sudo apt-get install parted）

<img src="darwin2.0_sd卡烧录介绍.assets/13.png" alt="13" style="zoom:150%;" />

所以我们知道userdata分区对应的是序号是6,对应于/dev/sdb6。 **再次强调**,这是我
的电脑此时的场景,一定要反复确认设备号。

这里开始将第6个分区创建为ext4文件系统

```
# 如果pc没有mkfs.ext4 命令,可以安装
#  sudo apt-get install e2fsprogs
mkdir userdata
sudo mkfs.ext4 /dev/sdX6
sudo mount /dev/sdX6 userdata/
sudo chmod -R 777 userdata/
sudo umount userdata/
sync
```

至此，烧录卡sd卡已经制作完成，稍后我们再拷贝要升级的镜像到ext4 文件系统中即可。

#### 2 准备spi nor启动的镜像 

##### 2.1 编译生成spi nor启动的镜像

另外准备一份代码，编译待烧录的spi nor启动镜像

```
sxyzhang@T430:~/x2000_sz/build$ make make x2000_nor_defconfig
sxyzhang@T430:~/x2000_sz/build$ make
```

对应生成如下文件：

```
sxyzhang@T430:~/x2000_sz/build$ ls output/
rootfs.squashfs  u-boot-spl-pad.bin  xImage
```

##### 2.2 使用烧录工具合成带有ddr参数和分区信息的nand启动uboot镜像

前面有说过烧录工具在烧录的时候会将ddr,分区表等信息烧录到uboot分区中，所以
这里需要用到烧录工具的合并镜像功能合并出一个与usb烧录之后无异的uboot.bin。

首先，需要在配置中选择与板子上对应的cpu型号。

<img src="darwin2.0_sd卡烧录介绍.assets/25.png" alt="25" style="zoom:150%;" />

其次，设置烧录工具的合并信息

<img src="darwin2.0_sd卡烧录介绍.assets/26.png" alt="26" style="zoom:150%;" />

最后，需要根据使
用的nor flash 设置对应的 nor id。 GD25Q127CSIG 的ID是 0xc84018。 需要设置info offset
(参数偏移),x2000和x1600都是0x5800。此处指定的0x5800是指要在
nor启动的u-boot-spl-pad.bin的0x5800地址处写入烧录工具当前设置的cpu型号对应
的ddr参数及“SFC--》分区信息”中指定的分区信息。因此，此处烧录工具的“INFO--》
板级”和“SFC--》分区信息”一定要设置对。

<img src="darwin2.0_sd卡烧录介绍.assets/27.png" alt="27" style="zoom:150%;" />

点击“合并镜像”后会在spi nor启动的uboot镜像的同目录生成合并后的文件：
image.bin

<img src="darwin2.0_sd卡烧录介绍.assets/18.png" alt="18" style="zoom:150%;" />

#### 3 将spi nor 启动镜像拷贝到sd卡

依次执行如下命令，实现将spi nor启动的镜像拷贝到sd卡中，完成制作sd烧录卡的最
后一步。

```
将sd卡插入pc
cd build                                    # 进入nor启动代码的编译目录
mkdir userdata
sudo mount /dev/sdX6 userdata/
sudo chmod -R 777 userdata/
mkdir -p userdata/sdcard_update/
touch userdata/sdcard_update/enable_update
cp output/image.bin userdata/sdcard_update/uboot.bin   # 一定要将合并后的uboot镜像拷贝并更名为uboot.bin
cp output/xImage userdata/sdcard_update/
cp output/rootfs.squashfs userdata/sdcard_update/
sync
sudo umount userdata/
```

此时拔掉sd卡, 插入开发板上电后就可以进行sd卡烧录了,升级打印如下：

```
detect mtd partitions
20+0 records in
20+0 records out
24556+0 records in
24556+0 records out
mtd erase uboot /dev/mtd0
mtd write uboot /dev/mtd0 /tmp/uboot.bin.2
[    2.565058] flash: 88284000
[    2.565258] ingenic-sfc 13440000.sfc: Use burner params.
[    2.565746] ingenic-sfc 13440000.sfc: magic is 0x726f6e  version 
is 0x2
[    2.566183] num_partition_info: 3
[    2.566410] Creating 2 MTD partitions on "sfc_mtd":
[    2.566736] 0x000000040000-0x000000340000 : "kernel"[    2.589109] 0x000000340000-0x000001000000 : "rootfs"
mtd erase kernel /dev/mtd1
mtd write kernel /dev/mtd1 /usr/data/sdcard_update/xImage
mtd erase rootfs /dev/mtd2
mtd write rootfs /dev/mtd2 /usr/data/sdcard_update/rootfs.squashfs
mtd erase uboot /dev/mtd0
mtd write uboot /dev/mtd0 /usr/data/sdcard_update/uboot.bin
mtd write image ok!
```

以上完成后，弹出开发板上的SD卡，重新给开发板上电，看串口log有正常输出，并且在终端查看cmdline，可以看到确实是从nor启动。

![28](darwin2.0_sd卡烧录介绍.assets/28.png)

至此，SD卡烧录nor完全结束。

### 五 sd卡烧录的常见问题

1  mmc2启动的时候，没有找到sfc分区，导致写sfc nor失败。

检查本文第四章1.2节，大概率是因为nor参数添加不对，或者没有添加，导致mmc2启动时加载sfc驱动失败，所以没有sfc 分区，从而无法继续烧录nor。


