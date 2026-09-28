# Darwin_X2000_双系统OTA升级

## 一  开发板介绍

开发板使用 Darwin_X2000_V2.0.

## 二  OTA 原理和介质介绍

### 1 OTA简介和实现方法

​        君正OTA简单来说分为三个步骤。

* 第一步：将需要更新的升级固件放到OTA升级服务器上。

* 第二步：通过网络将升级固件传输到设备上。

* 第三步：设备利用得到的OTA升级固件更新到自身的设备上，然后重启切换到更新好的系统上，从而完成OTA整个流程。

  君正基于Linux 系统，目前OTA 升级的策略是采用双系统备份。也就是一台设备上同时含有两套系统。每次只运行在其中一套系统上。其中共用一个uboot，有两份kernel和rootfs文件系统。这样不管OTA升级是否成功，都可以保证当前的系统是正常的。



### 2 OTA 存储介质

​		君正OTA目前支持的存储介质包含两种：mtd  和 mmc两种。其中mtd包含：nand 和 nor，本教程主要为mtd介质（nand）的OTA升级介绍，mmc介质OTA升级请参考`doc/FAE文档/X2000_ilock_V1.0/X2000_iLock_V1.0_mmc_开发板OTA使用说明文档`



## 三  ota 编译配置

### 1  uboot 添加支持 ota 功能

​		以Darwin_X2000_V2.0开发板编译配置为例，编译出的uboot固件需要支持ota功能：

首先需要确定你当前的编译配置使用的是那个uboot的编译配置文件，通过如下方法可以确定。使用工具 `tools/iconfigtool/IConfigToolApp$ ./IConfigTool` 

```c
选择Config.in文件：`build/Config.in`

选择Config文件： build/configs/x2000_darwin_factory_defconfig
```

![1](Darwin_X2000_双系统OTA升级.assets/1.png)

如上图，根据uboot相对路径和默认配置，可以确定uboot编译的配置文件为 `bootloader/uboot-x2000/boards.cfg` 的 `x2000_base_xImage_sfc_nand`,在文件 `boards.cfg`中搜索`x2000_base_xImage_sfc_nand`。看是否含有`SPL_OS_OTA_BOOT`的标志。有这个标志表示uboot支持ota的功能，没有就手动加上这个标志

//或选择使用预先写好的uboot默认配置`x2000_base_ota_xImage_sfc_nand`

```
x2000_base_xImage_sfc_nand	 mips        xburst2	x2000_base	 ingenic	x2000_v12	x2000_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_BOOT,SPL_OS_OTA_BOOT,RMEM_MB=16,SPL_PARAMS_FIXER
```



### 2  buildroot  ota编译配置注意事项

​	君正ota采用的是只读系统，所以便于用户开发需要可读可写的分区，我们设置了一个data分区（在IConfigTool中启动的自动挂载脚本），该分区是可读可写分区。这样可以便于用户存放可读可写的文件。

如需要ubifs格式的文件系统，需要buildroot支持可以打包该格式的文件系统配置。

方法如下,进入buildroot 执行:

`/buildroot/buildroot$ make menuconfig`  在`Filesystem images`目录下按照下图选择相应的配置即可。



![2](Darwin_X2000_双系统OTA升级.assets/2.png)



备注：buildroot的配置修改以后，需要执行拷贝到其主配置上去。比如该buildroot的配置为：

通过如下命令：

```c
build$ grep -nr "buildroot" configs/x2000_darwin_factory_defconfig
8:APP_br_dir=../buildroot/buildroot
9:APP_br_config_file=configs/buildroot/buildroot_x2000_darwin_debug
456:# APP_br_root_login_keep_buildroot is not set
```

可知当前使用buildroot的配置为：

`build/configs/buildroot/buildroot_x2000_darwin_debug`（对应IConfigTool中`工程/编译配置-buildroot配置文件`）

所以上面修改了配置以后，执行如下操作：

`buildroot/buildroot$ cp .config ../../build/configs/buildroot/buildroot_x2000_darwin_debug`

这样你再整体编译的时候，buildroot的修改才会生效（当使用`make x2000_darwin_factory_defconfig`进行整体配置编译后，会覆盖buildroot_x2000_darwin_debug 配置，此时也需重新将配置拷贝到主配置中）。



## 四  制作OTA升级固件

​		以Darwin_X2000_V2.0开发板编译配置为例制作OTA升级固件。使用ota升级为联网升级，参照`doc/FAE文档/X2000H_Darwin_X2000_V2.0/x2000H_Darwin_v2.0_开发板快速上手说明.pdf`进行以太网与wifi设置。



### 1 ota 相关配置

 使用工具 `tools/iconfigtool/IConfigToolApp$ ./IConfigTool` ,选择配置文件： 

```c
 build/configs/x2000_darwin_factory_defconfig
```

配置如下：

![3](Darwin_X2000_双系统OTA升级.assets/3.png)



![2023-05-06_15-25](Darwin_X2000_双系统OTA升级.assets/4.png)



从上往下，每项宏的配置对应的功能如下所示：

```shell
1：Storage medium(存储介质)(APP_ota_updater_storage_medium):
//板子的存储介质，包含两种类型，nand和mmc，可以根据自己的设备存储介质选择不同的类型

2：当前系统的ota版本号(每次编译新的ota版本时都应该递增此值)(APP_ota_updater_version)
//当前ota的版本号，会写入文件系统的 /etc/ota_info 'ota_version=N'
该版本号是指打包升级包的版本号，如果你需要制作版本号为3,该处需要修改为3

3：ota 升级服务器的地址(App_ota_updater_site)
//ota的服务器地址路径，会写入文件系统的 /etc/ota_info 'ota_site=...'

4：ota 分包大小(App_ota_updater_block_size)
//ota分包大小，制作ota镜像时会将ota镜像按此大小分包

5：ota kernel 镜像路径(App_ota_updater_kernel_img_path)
//ota kernel镜像路径，制作ota镜像时需要

6：ota rootfs 镜像路径(App_ota_updater_rootfs_img_path)
//ota rootfs镜像路径，制作ota镜像时需要
```



### 2 编译 ota 升级包固件

先使用配置 `build/configs/x2000_darwin_factory_defconfig` 进行整体编译以后，然后进入build，执行如下指令：

```
build$ make ota_img
#注意： 在编译ota_img 之前必须得先保证编译好buildroot镜像文件与kernel镜像文件
```

在 "工程/build"目录下编译ota升级镜像，会在 "工程/build/output/ota" 目录下生成ota镜像文件：

```shell
cd output/ota/
ls

#保存 rootfs.squashfs.000*的md5sum 用于下载后设备校验
ota_md5_rootfs.squashfs.4a2698f41ef89f7f70589feb06f0924b

#保存xImage.000*的md5sum 用于下载后设备校验
ota_md5_xImage.344291f6e997c3e74f7febcd22b618fa

#镜像文件拆分的配置
ota_update.in

#"rootfs.squashfs.*"表示被拆分后的buildroot镜像文件
rootfs.squashfs.0000.4a2698f41ef89f7f70589feb06f0924b
rootfs.squashfs.0001.1934179f2128a1bb799926004c8e9e0a
rootfs.squashfs.0002.dd36a397d7cadcaf96b9d4bd97bad2e3
rootfs.squashfs.0003.6b3e7d13960f67cb80c95696c2daa76d
rootfs.squashfs.0004.b908e4b6485dcab8a62b70fde2bd7829
rootfs.squashfs.0005.2404c8bde90cdeaf7ef49bcdc7daabf0
rootfs.squashfs.0006.a0a82d04bd7114536a7487f436797baa
rootfs.squashfs.0007.cc4f40ef2c2e3b0607aebef1cc323552
rootfs.squashfs.0008.7847ab3545b3e3e2dbef74ec729b63de
rootfs.squashfs.0009.aa90d92745c114597411ecc923e345c0
rootfs.squashfs.0010.23fb1598fe32fda41935949c1f31f086
rootfs.squashfs.0011.a3022e2180bc612d8561cea3fe484e8d
rootfs.squashfs.0012.8baf38989fcbb787dc5880f9a4e1651c

#"xImage.*"表示被拆分后的kernel镜像文件
xImage.0000.344291f6e997c3e74f7febcd22b618fa
xImage.0001.f4986784927e99bf0fa15960220eb770
xImage.0002.48e976fc59a9849ff1ad35a31e4509f8
xImage.0003.dd8351d30492eb862444b08c9329e30d

```

其中 ota_update.in文件

build$ cat output/ota/ota_update.in 

```shell
ota_version=3

img_type=kernel
img_name=xImage
img_size=3776576
img_md5=344291f6e997c3e74f7febcd22b618fa

img_type=rootfs
img_name=rootfs.squashfs
img_size=12853248
img_md5=4a2698f41ef89f7f70589feb06f0924b

#关键字 ota_version:做版本检查，避免忙中出错和实际版本不一致
#关键字 img_type:定义升级的内容类型：kernel rootfs
#关键字 img_name:升级的文件名，在升级包目录的文件名前缀
#关键字img_size:升级文件的总大小，因为升级文件会被拆分成多个文件，默认按1Mbyte拆分
#关键字img_md5:升级文件的md5sum值

```



## 五    OTA 烧录工具配置

### 存储介质为NAND 的烧录工具配置

以Darwin_X2000_V2.0开发板 OTA 烧录为例，进行烧录工具的说明。配置如下：

//板级信息也可选x2000h_sfc_nand_ota_lpddr3_linux.cfg

![5](Darwin_X2000_双系统OTA升级.assets/5.png)

![6](Darwin_X2000_双系统OTA升级.assets/6.png)

![7](Darwin_X2000_双系统OTA升级.assets/7.png)

![image-20230414115643066](Darwin_X2000_双系统OTA升级.assets/8.png)



**上面 kernel和kernel2 烧录的是同一个kernel固件， rootfs 和 rootfs2 烧录的是同一个rootfs固件。第一次烧录两份文件系统是为了后面ota升级备用使用的。每次板子都会只运行在正常的文件系统上面，保证板子能够正常运行。**

*填写label和选择ops后会自动补齐offset，均需要与sfc中的分区信息一致*

*ota 分区烧录的文件可以为一个空文件。*

*userdata为/usr/data下挂载的分区，根据需要设置大小与文件。*



## 六   OTA 测试服务器搭建方法（仅供参考）

君正不提供服务器，客户需自己解决，以下服务器搭建仅供测试使用。ota的升级文件在设备端，能被wget命令获取即可。

### 1  安装 apache2 服务器，用于测试ota升级

​       安装命令： `sudo apt-get install apache2`

### 2  启动 apache2 服务器

启动命令：  `service apache2 start `

 电脑端放置OTA升级固件的目录：`/var/www/html/ `。

注：如果设备端无法访问apache2服务器，有可能是防火墙的问题。

可以关掉电脑端的防火墙再进行测试，命令如下：

暂时关闭防火墙：
`systemctl stop firewalld`



## 七   OTA服务器文件部署

以`Darwin_X2000_V2.0`开发板为例，将版本升级至3，我的电脑的地址是10.4.3.5，所以地址：`http://10.4.3.5//ota/board_test/`用于存放ota相关的文件，对应于服务器本机的路径为：`/var/www/html/ota/board_test`，

此处与4.1章节ota配置保持一致.

该目录下的文件需要自己添加：

```shell
cd /var/www/html/ota/board_test/

ls -l
ota_config.in    #定义当前最新版本的ota版本
ota_v2           #版本为2对应的ota升级文件夹
ota_v3           #版本为3对应的ota升级文件夹
```

1 添加ota_config.in文件，该文件内容如下：

```shell
/*
关键字 current_version 用于定义当前最新的ota的版本
当`current_version=3`时,对应于目录`ota_v3/`存放ota的升级信息和文件
*/
current_version=3
```

2 ota_v'N'/目录 （'N'是数字，这里添加ota_v3)，需要自己手动添加，并将编译好的ota升级镜像文件复制到该文件夹

> **之前已编译过ota版本为2的系统,这里升级为3版本, 需要重新编译ota版本为3的系统, 即iconfig需要修改ota版本号**

![image-20230414115643066](Darwin_X2000_双系统OTA升级.assets/10.png)

```shell
 cp output/ota/* /var/www/html/ota/board_test/ota_v3/ -arf    #复制ota镜像文件到服务器
```

3 手动创建 ota_v3.ok 到 /var/www/html/ota/board_test/ota_v3/ 文件夹下，最终确定 ota_v3版本发布

```shell
ota_v3.ok (3是版本号 ota_v'N'.ok 且该文件的内容为空即可)
此文件是做 ota 的最后一步确认，制作 ota镜像的时候不会有这个文件产生，这个文件需要客户自己手动创建生成，以确定此版本的ota验证是成功可行的，可以提供给外部的设备端进行下载更新。设备端如果检测到此文件才会启动ota升级流程
```

所以`/var/www/html/ota/board_test/ota_v3`文件夹的内容如下：

```shell
lih@lih:/var/www/html/ota/board_test/ota_v3$ ls -lh
total 16M
-rw-rw-r-- 1 lih lih  429  3月 14 20:01 ota_md5_rootfs.squashfs.636e0689347b182159baa27cf6c8ffbf
-rw-rw-r-- 1 lih lih  132  3月 14 20:01 ota_md5_xImage.affe7e73ecb13d69956e401d0421ec62
-rw-rw-r-- 1 lih lih  207  3月 15 11:23 ota_update.in
-rw-rw-r-- 1 lih lih    0  3月  9 13:51 ota_v3.ok
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0000.636e0689347b182159baa27cf6c8ffbf
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0001.25bb3b3b105107c5a74deff47ce4ad61
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0002.dd36a397d7cadcaf96b9d4bd97bad2e3
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0003.b8bb466d63b88d9849f329dce72bbfc5
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0004.b151bd8c51aeaf5c8ba9be2dff76b81c
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0005.a96cbf15bcb726fce02e2a94bee26b33
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0006.cb0abb95236650f20dcbf163dafd34f2
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0007.29b6a0ccb21613589d47c6e740bcc20f
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0008.39fd8413de1e58f97bd98ff3b3e6a0bf
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0009.20957b97e277628df62481149a27bed6
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0010.6088eff61c31178313f45a3e6c5e882d
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 rootfs.squashfs.0011.ad96a6588963c064ddd9805ea3e6b5b9
-rw-rw-r-- 1 lih lih 272K  3月 14 20:01 rootfs.squashfs.0012.bcd4cce1abf582ede1c4d38b1a0eaf46
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 xImage.0000.affe7e73ecb13d69956e401d0421ec62
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 xImage.0001.2102e4f4c28a9925f012eb4f3e0c8c9f
-rw-rw-r-- 1 lih lih 1.0M  3月 14 20:01 xImage.0002.48e976fc59a9849ff1ad35a31e4509f8
-rw-rw-r-- 1 lih lih 617K  3月 14 20:01 xImage.0003.1ef4ff4a1efec9eb35af6639d8d7d0e6
```

## 八   OTA  升级的流程

### 1  ota 升级脚本

调用ota升级脚本，ota的升级脚本在 "/etc/ota_bin/" 目录下，ota的配置在文件 "/etc/ota_info" 中，ota_info的内容如下：

```shell
ota_version=2    #表示设备端当前的版本号
ota_site=http://10.4.3.5/ota/board_test    #表示ota升级的服务器地址
```

ota_info 配置文件是由前面 "make ota_img" 的命令所生成的：

ota的升级脚本文件：

```shell
# ls /etc/ota_bin/ -l
total 45
-rwxrwxr-x    1 root     root         14075 Mar 15  2023 local_ota_update.sh
-rwxrwxr-x    1 root     root         14090 Mar 15  2023 network_ota_update.sh
-rwxrwxr-x    1 root     root          2596 Mar 15  2023 ota_img_data_provider.sh
-rw-rw-r--    1 root     root          3245 Mar 15  2023 ota_local_method.sh
-rwxrwxr-x    1 root     root          1077 Mar 15  2023 ota_update_kernel.sh
-rwxrwxr-x    1 root     root          1077 Mar 15  2023 ota_update_rootfs_squashfs.sh
-rwxrwxr-x    1 root     root          1053 Mar 15  2023 ota_update_rtos_bin.sh
-rw-rw-r--    1 root     root          5798 Mar 15  2023 ota_utils.sh

```

执行`sh /etc/ota_bin/network_ota_update.sh`

升级最后的结果见下图：

![9](Darwin_X2000_双系统OTA升级.assets/9.png)



"network_ota_update.sh" 脚本的升级过程如下：

```shell
 1  检查是否设置过下次启动项(检查文件是否存在/tmp/ota_boot_set),如未设置过,则继续
 2  下载测试服务器的 ota_config.in文件，解析当前ota版本号
 3  获取/etc/ota_info 中的版本号并与服务器ota版本号做对比，如果服务器更高，则继续
 4  下载测试服务器 ota_v3/ota_v3.ok 文件，如果无法下载，说明当前v3版本还没有准备好，则退出ota升级流程
 5  下载测试服务器 ota_v3/ota_update.in 文件
 6  获取 ota_version=字段，如果和当前版本不相等，则退出ota升级流程
 7  获取img_type img_name img_size img_md5 字段，并且验证和保存
    ota_kernel ota_kernel_name ota_kernel_size ota_kernel_md5
    ota_rootfs ota_rootfs_name ota_rootfs_size ota_rootfs_md5
 8  获取需要升级的kernel rootfs分区，检查kernel分区大小，检查rootfs 分区大小是否满足升级包要求
 9  开始升级kernel
10  启动升级脚本 /etc/ota_bin/ota_update_kernel.sh
11  调用download_ota_img 函数下载 xImage 相关文件，校验 md5值
12  去掉文件 md5 后缀，生成 xImage.000N，再生成 xImage.000N.ok 通知升级脚本读取 xImage.000N数据
13  继续下载新的 xImage.000N+1.md5文件，等待xImage.000N.done产生，并删除 xImage.000N
14  继续11的步骤，直到xImage 相关文件下载完成
15  等待xImage.ok 生成，表示升级脚本成功，或者 xImage.failed 表示升级脚本失败
16  按前面的步骤升级rootfs
17  如果升级成功，则设置启动分区为刚才升级的分区，返回值0
18  如果什么都没有升级，也没有出错，则返回值2,中间任意步骤出错，返回1

```



### 2 ota 启动分区

设置ota启动分区

若升级成功会自动设置启动分区为升级的分区，若想设置回来可以设置ota分区的内容，uboot会检查ota分区的内容，以决定启动哪个分区：



```shell
#擦除ota分区的一块数据

flash_erase /dev/mtd5 0 1

#写入启动分区名 ota:kernel2 或者 ota:kernel

printf "%-256s" "ota:kernel2" | nandwrite -s 0 -p /dev/mtd5 -

#查看当前的启动分区名

nanddump -s 0 -l 256 /dev/mtd5 -a
#注意:/dev/mtd5 对应ota分区,可用"cat /proc/mtd"命令查看
```



## 九   ota 补充说明

### 1  ota_bin 目录

```shell
# ls /etc/ota_bin/

#ota升级过程均使用脚本编写，都有中文注释

# 升级主程序 供开发者调用

local_ota_update.sh
ota 本地升级主程序，探测是否需要 ota 升级，以及本地读取 ota 镜像执行 ota 升级流程

network_ota_update.sh
ota 网络升级主程序，探测是否需要 ota 升级，以及下载 ota 镜像执行 ota 升级流程

network_main_os_update_recovery.sh
ota recovery方式进行ota升级的主程序，主系统 中用来探测是否需要 ota 升级，下载 recovery 镜像以及升级 recovery 系统

network_recovery_update_main_os.sh
ota recovery方式进行ota升级的主程序，recovery系统 中用来下载 主系统 镜像以及升级 主系统

# 功能性程序与函数

ota_local_method.sh
ota 升级程序的设备端方法和升级流程回调的实现

ota_update_kernel.sh
ota kernel的升级方法

ota_update_rootfs_squashfs.sh
ota rootfs.squashfs 的升级方法

ota_update_rtos_bin.sh
ota rtos.bin 的升级方法

ota_utils.sh
ota 用到的实用函数集

ota_img_data_provider.sh
ota 流式升级方式的数据提供者
被ota_update_kernel.sh ota_update_rootfs_squashfs.sh 调用

main_os_info_check.sh
ota 检查 主系统 的升级信息可用性
被network_main_os_update_recovery.sh 调用

recovery_version_utils.sh
ota 用于检测recovery版本信息的工具脚本
被network_main_os_update_recovery.sh 调用
```

### 2  ota 文件组织策略

编译ota升级镜像时，kernel和buildroot的镜像文件被拆分成多份，分成多个文件，依次追加md5值，并且保存md5值用做文件校验，这样的好处是可以确保下载的文件不会因为服务器缓存的问题而导致的包之间不一致的问题。即使文件名碰巧重合，设备端也可以通过后缀的md5值进行校验

1 以kernel(xImage)为例子，将 xImage(大小 3776576)拆分成4个文件：

```shell
xImage.0000.affe7e73ecb13d69956e401d0421ec62
xImage.0001.2102e4f4c28a9925f012eb4f3e0c8c9f
xImage.0002.48e976fc59a9849ff1ad35a31e4509f8
xImage.0003.1ef4ff4a1efec9eb35af6639d8d7d0e6
```

2 对xImage 以及 xImage.000* 求 md5sum,关键字 img_md5 保存 xImage.md5sum,ota_md5_xImage 保存 xImage.000*的md5sum 用于下载后设备校验

3 所有的xImage 相关文件被依次追加前一个xImage包文件的md5值作为后缀：

```shell
1 ota_md5_xImage -> ota_md5_xImage."xImage.md5"
2 xImage.0000 -> xImage.0000."xImage.md5"
3 xImage.0001 -> xImage.0001."xImage.0000.md5"
4 xImage.0002 -> xImage.0002."xImage.0001.md5"
5 xImage.0003 -> xImage.0003."xImage.0002.md5"
```

### 3 ota 分区策略

更新设备分区的时候必须保证当前被更新的分区不被占用，所以rootfs分区的程序不能更新rootfs分区的内容，只能去更新另外一个rootfs分区。

从安全的角度上也是一样，如果当前能更新程序所在的分区，比如 rootfs 打包在kernel中的情况，但是遇到掉电/死机/重启等不正常的时候，再一次启动分区的内容不是完整的，所以设备就会无法启动。

所以必须为ota升级程序划分专门的kernel分区和rootfs分区，另外uboot不能进行升级，一旦升级失败机器将无法启动。

目前采用的策略是双分区备份策略，即rootfs的升级程序升级kernel2,rootfs2,反之rootfs2升级kernel，rootfs。升级完成之后改变当前的启动分区为被升级的分区并且重启，启动分区的信息保存在“ota”分区中，分区列表的结构如下,针对 128M nand ，分区大小可以根据实际情况进行调整。

```shell
1  uboot            1M
2  kernel		    4M
3  rootfs           48M
4  kernel2          4M
5  rootfs2          48M
6  ota              1M
7  userdata         剩余大小
```



## 十   ota 常见问题汇总

1  ota升级完成无法进入到新的系统？

​    可以检查设备端的固件uboot是否支持ota功能。具体可以参考上面 4.1  uboot 添加支持 ota 功能。

2 设备端无法通过wget获取到升级服务器上的文件？

   可以检查电脑端的防火墙是否关闭。如果是局域网，要检查你设备端的网络和你的升级服务器上的网络是否在同一个网段。

