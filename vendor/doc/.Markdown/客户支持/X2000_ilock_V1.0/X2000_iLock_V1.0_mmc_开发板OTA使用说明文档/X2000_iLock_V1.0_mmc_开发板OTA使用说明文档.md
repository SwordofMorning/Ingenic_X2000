# X2000_iLock_V1.0_mmc_开发板OTA使用说明文档



## 一  开发板介绍

### 快速上手

快速上手介绍见：

`doc/FAE文档/X2000_ilock_V1.0/X2000_iLock_V1.0开发板快速上手说明文档.pdf`



## 二  OTA 原理和介质介绍

### 1 OTA简介和实现方法

​        君正OTA简单来说分为三个步骤。

* 第一步：将需要更新的升级固件放到OTA升级服务器上。

* 第二步：通过网络将升级固件传输到设备上。

* 第三步：设备利用得到的OTA升级固件更新到自身的设备上，然后重启切换到更新好的系统上，从而完成OTA整个流程。

  君正基于Linux 系统，目前OTA 升级的策略是采用双系统备份。也就是一台设备上同时含有两套系统。每次只运行在其中一套系统上。其中共用一个uboot，有两份kernel和rootfs文件系统。这样不管OTA升级是否成功，都可以保证当前的系统是正常的。



### 2 OTA 存储介质

​		君正OTA目前支持的存储介质包含两种：mtd  和 mmc两种。其中mtd包含：nand 和 nor，本教程主要为mmc介质的OTA升级介绍，mtd介质OTA升级请参考`doc/FAE文档/X2000H_Darwin_X2000_V2.0/x2000H_Darwin_v2.0_开发板OTA使用说明文档`



## 三  ota 编译配置

### 1  uboot 添加支持 ota 功能

​		以X2000_iLock_V1.0开发板编译配置为例，编译出的uboot固件需要支持ota功能：

首先需要确定你当前的编译配置使用的是那个uboot的编译配置文件，通过如下方法可以确定。使用工具 `tools/iconfigtool/IConfigToolApp$ ./IConfigTool` 

选择Config.in文件：`build/Config.in`

选择Config文件： `build/configs/x2000_ilock_mmc0_factory_test_defconfig`

![1](X2000_iLock_V1.0_mmc_开发板OTA使用说明文档.assets/1.png)

如上图，根据uboot相对路径和默认配置，可以确定uboot编译的配置文件为 `bootloader/uboot-x2000/boards.cfg` 的 `x2000_base_ilock_xImage_mmc0`,在文件 `boards.cfg`中搜索`x2000_base_ilock_xImage_mmc0`。看是否含有`SPL_OS_OTA_BOOT`的标志。有这个标志表示uboot支持ota的功能，没有就手动加上这个标志

```
x2000_base_ilock_xImage_mmc0	 mips        xburst2	x2000_base	 ingenic	x2000_v12	x2000_base:SPL_JZMMC_SUPPORT,ENV_IS_IN_MMC,GPT_CREATOR,JZ_MMC_MSC0,SPL_OS_BOOT,SPL_OS_OTA_BOOT,SPL_PARAMS_FIXER,GPT_TABLE_PATH=$
```



### 2  buildroot  ota编译配置注意事项

​	君正ota采用的是只读系统，同时我们设置了一个data分区，该分区是可读可写分区，在烧录时在分区表中声明此分区，并在IConfigTool中勾选自动挂载脚本，便会自动挂载到`/usr/data`下。

​	在IConfigTool中做如下设置：

![2](X2000_iLock_V1.0_mmc_开发板OTA使用说明文档.assets/2.png)

​	如需设置文件系统为可读可写系统，对于mmc介质一般采用ext4格式的文件系统，在buildroot的menuconfig中以及kernel的文件系统中进行勾选、烧录对应的文件即可，读者可自行尝试。



## 四  制作OTA升级固件

​        以`X2000_iLock_V1.0`开发板为例制作OTA升级固件。参考`doc/FAE文档/X2000_ilock_V1.0/X2000_iLock_V1.0开发板快速上手说明文档.pdf` ，让板子先联上网络。而后重新配置ota相关配置，生成ota升级固件。



### 1 ota相关配置

 使用工具 `tools/iconfigtool/IConfigToolApp$ ./IConfigTool` ,选择配置文件：  `build/configs/x2000_ilock_mmc0_factory_test_defconfig`

配置如下：

![3](X2000_iLock_V1.0_mmc_开发板OTA使用说明文档.assets/3.png)



![4](X2000_iLock_V1.0_mmc_开发板OTA使用说明文档.assets/4.png)

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



### 2 编译ota 升级包固件

先使用配置 `build/configs/x2000_ilock_mmc0_factory_test_defconfig` 进行整体编译以后，然后进入build，执行如下指令：

```
build$ make ota_img
#注意： 在编译ota_img 之前必须得先保证编译好buildroot镜像文件与kernel镜像文件
```

在 "工程/build"目录下编译ota升级镜像，会在 "工程/build/output/ota" 目录下生成ota镜像文件：

```shell
cd output/ota/
ls

#保存 rootfs.squashfs.000*的md5sum 用于下载后设备校验
ota_md5_rootfs.squashfs.c881e4f2a9d36f0a6370f794477901a8

#保存xImage.000*的md5sum 用于下载后设备校验
ota_md5_xImage.a5afac90177d681490e21e41c52fc1d7

#镜像文件拆分的配置
ota_update.in

#"rootfs.squashfs.*"表示被拆分后的buildroot镜像文件
rootfs.squashfs.0000.c881e4f2a9d36f0a6370f794477901a8
rootfs.squashfs.0001.1db8def2d65892915a6509674d8ce95f
rootfs.squashfs.0002.beb0de3ac92df0f7888ff05f245778f1
rootfs.squashfs.0003.19b80f11614f78c029c90c8f69d4345b
rootfs.squashfs.0004.a80f41c4accf2900ef74bce707c8c7b6
rootfs.squashfs.0005.08f0dd2af5f6fd069247af3458ccea8b
rootfs.squashfs.0006.7ec8f4a9ede786672acfaea77f55cf1a
rootfs.squashfs.0007.6c9e5b86b52082f3308c6e83e092082d
rootfs.squashfs.0008.a274e869cf48516768a5d7d173633062
rootfs.squashfs.0009.f9f6eb8c75ee6865855d998607a5f72a
rootfs.squashfs.0010.734fe09b6907e495b321b69df8ca70fe
rootfs.squashfs.0011.9206ddb788bb3b0bee648e27ee061d8a
rootfs.squashfs.0012.00c685e8137adc8d3937e26816cc76a9
rootfs.squashfs.0013.ab6f9f7e16c61b068843a333d04077da
rootfs.squashfs.0014.64a7e9ae34caef9cdc054ffd5ecc31e3
rootfs.squashfs.0015.80e096f334fd4989b8ebf53ceb1561dd
rootfs.squashfs.0016.ef46aed2109409a3804f1f8e454805e8
rootfs.squashfs.0017.48f795b1342e635b0125ad22b50b49b9
rootfs.squashfs.0018.6be28de7869509e5f3934c0b9ec940ea

#"xImage.*"表示被拆分后的kernel镜像文件
xImage.0000.a5afac90177d681490e21e41c52fc1d7
xImage.0001.f94ee3b2b50955fa8ff95a083eb6bf54
xImage.0002.69dfdf55dce6ab9a02c8847145b0a873
```

其中 ota_update.in文件

build$ cat output/ota/ota_update.in 

```shell
ota_version=1

img_type=kernel
img_name=xImage
img_size=2809920
img_md5=85864ad6b6fe421eb9b77400070baffc

img_type=rootfs
img_name=rootfs.squashfs
img_size=18915328
img_md5=f62141620d4160ed2576e6ea8782c39b

#关键字 ota_version:做版本检查，避免忙中出错和实际版本不一致
#关键字 img_type:定义升级的内容类型：kernel rootfs
#关键字 img_name:升级的文件名，在升级包目录的文件名前缀
#关键字img_size:升级文件的总大小，因为升级文件会被拆分成多个文件，默认按1Mbyte拆分
#关键字img_md5:升级文件的md5sum值
```





## 五    OTA 烧录工具配置

### 存储介质为 mmc 的烧录工具配置



以`X2000_iLock_V1.0`开发板 OTA 烧录为例，进行烧录工具的说明。配置如下：

![5](X2000_iLock_V1.0_mmc_开发板OTA使用说明文档.assets/5.png)

mmc存储介质的分区表在`bootloader/uboot-x2000/board/ingenic/x2000_base/ilock/partitions.tab`

![6](X2000_iLock_V1.0_mmc_开发板OTA使用说明文档.assets/6.png)

可以根据用户需要修改分区表信息，注意分区信息要与烧录工具中的`POLICY`一致，设置如下：

![7](X2000_iLock_V1.0_mmc_开发板OTA使用说明文档.assets/7.png)

![8](X2000_iLock_V1.0_mmc_开发板OTA使用说明文档.assets/8.png)

*其中 kernel和kernel2 烧录的是同一个kernel固件， rootfs 和 rootfs2 烧录的是同一个rootfs固件。第一次烧录两份文件系统是为了后面ota升级备用使用的。每次板子都会只运行在正常的文件系统上面，保证板子能够正常运行。*

*ota 分区烧录的文件可以为一个空文件*



## 六   OTA 测试服务器搭建方法（仅供参考）

君正不提供服务器，客户需自己解决，以下服务器搭建仅供测试使用。ota的升级文件在设备端，能被wget命令获取即可。

### 1  安装 apache2 服务器，用于测试ota升级

​       安装命令： sudo apt-get install apache2

### 2  启动 apache2 服务器

启动命令：  service apache2 start 

 电脑端放置OTA升级固件的目录：/var/www/html/ 。

注：如果设备端无法访问apache2服务器，有可能是防火墙的问题。

可以关掉电脑端的防火墙再进行测试，命令如下：

暂时关闭防火墙：
systemctl stop firewalld



## 七   OTA服务器文件部署

以`X2000_iLock_V1.0`开发板为例，将版本升级至1，我的电脑的地址是12.10.70.25，所以地址：http://12.10.70.25/ota/x2000_ilock/用于存放ota相关的文件，对应于服务器本机的路径为：`/var/www/html/ota/x2000_ilock`，

该目录下的文件需要自己添加：

```shell
cd /var/www/html/ota/x2000_ilock/

ls
ota_config.in    #定义当前最新版本的ota版本
ota_v1           #版本为1对应的ota升级文件夹
```

1 添加ota_config.in文件，该文件内容如下：

```shell
/*
关键字 current_version 用于定义当前最新的ota的版本
当`current_version=1`时,对应于目录`ota_v1/`存放ota的升级信息和文件
*/
current_version=1
```

2 ota_v'N'/目录 （'N'是数字，这里添加ota_v1)，需要自己手动添加，并将编译好的ota升级镜像文件复制到该文件夹

```shell
 cp output/ota/* /var/www/html/ota/x2000_ilock/ota_v1/ -arf    #复制ota镜像文件到服务器
```

3 手动创建 ota_v1.ok 到 `/var/www/html/ota/x2000_ilock/ota_v1/` 文件夹下，最终确定 ota_v1版本发布

```shell
ota_v1.ok (1是版本号 ota_v'N'.ok 且该文件的内容为空即可)
此文件是做 ota 的最后一步确认，制作 ota镜像的时候不会有这个文件产生，这个文件需要客户自己手动创建生成，以确定此版本的ota验证是成功可行的，可以提供给外部的设备端进行下载更新。设备端如果检测到此文件才会启动ota升级流程
```

所以`/var/www/html/ota/x2000_ilock/ota_v1`文件夹的内容如下：

```shell
lih@lih:/var/www/html/ota/x2000_ilock/ota_v1$ ls -lh
total 23M
-rw-rw-r-- 1 lih lih  627  3月 22 15:27 ota_md5_rootfs.squashfs.00ee3ab42a40337b2f7571038a7036fa
-rw-rw-r-- 1 lih lih  627  3月 22 15:47 ota_md5_rootfs.squashfs.f62141620d4160ed2576e6ea8782c39b
-rw-rw-r-- 1 lih lih   99  3月 22 15:47 ota_md5_xImage.85864ad6b6fe421eb9b77400070baffc
-rw-rw-r-- 1 lih lih  207  3月 22 15:48 ota_update.in
-rw-rw-r-- 1 lih lih    0  3月  9 13:51 ota_v1.ok
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:27 rootfs.squashfs.0000.00ee3ab42a40337b2f7571038a7036fa
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0000.f62141620d4160ed2576e6ea8782c39b
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0001.a00ff99cf20d301b0ca64b7df32c706a
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:27 rootfs.squashfs.0001.bbc490febf6710d6eec7f8e2dd54dca8
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0002.430a5b306db8ea4a0afde598b5311545
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0003.9040f60b0907da1b918756d688f3946c
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0004.020814b27f77eb35475e7ed7afb4fdaa
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0005.d99e5b8a81275379ed596c44a99642cc
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0006.fa6f0bc89175654e6fe6d1c769915aef
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0007.1310b31257156a482f02d4acebf1a5da
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0008.896430e53ba6f55c72d8f389f5bf208d
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0009.ebf098ec5b8358068ac169f76deed755
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0010.a6ca570eaa35038e7cee6ee60857ac50
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0011.8110f599947a09812dc51922df71e821
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0012.4d5514f64c037f34c1a5f5105a5a4167
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0013.fca1296bf4e8f49b15ce0744417d78be
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0014.be983d5702a82ea9a54b45592091cf30
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0015.4abba4c5681d46d261ab6c82a6ae7570
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0016.db2041e17189bd64a6f8c5e554c470be
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 rootfs.squashfs.0017.17b2f79796ca15b81281fa36e69b3faf
-rw-rw-r-- 1 lih lih  40K  3月 22 15:47 rootfs.squashfs.0018.95cb6cfa00a44300f0ce05a4918813c0
-rw-rw-r-- 1 lih lih  40K  3月 22 15:27 rootfs.squashfs.0018.b3421c7f13ed9898858650f2ebfd9f96
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 xImage.0000.85864ad6b6fe421eb9b77400070baffc
-rw-rw-r-- 1 lih lih 1.0M  3月 22 15:47 xImage.0001.6be602b75b789bb1ad1c5d6d35a8662a
-rw-rw-r-- 1 lih lih 697K  3月 22 15:47 xImage.0002.db07e60390f0f9ce663b032e735da8fc
```

*ota_v1/ota_update.in 里面放的是版本号为1的升级包，其中ota_version=1，该值跟第四章第1条那里的设置相对应。*

## 八   OTA  升级的流程

### 1  ota 升级脚本

调用ota升级脚本，ota的升级脚本在 "/etc/ota_bin/" 目录下，ota的配置在文件 "/etc/ota_info" 中，ota_info的内容如下：

```shell
ota_version=0     #表示设备端当前的版本号
ota_site=http://12.10.70.25/ota/x2000_ilock     #表示ota升级的服务器地址
```

ota_info 配置文件是由前面 `make ota_img` 的命令所生成的：

ota的升级脚本文件：

```shell
# ls /etc/ota_bin/ -l
total 44
-rwxrwxr-x    1 root     root         14075 Mar 22  2023 local_ota_update.sh
-rwxrwxr-x    1 root     root         14090 Mar 22  2023 network_ota_update.sh
-rwxrwxr-x    1 root     root          2596 Mar 22  2023 ota_img_data_provider.sh
-rw-rw-r--    1 root     root          3249 Mar 22  2023 ota_local_method.sh
-rwxrwxr-x    1 root     root           590 Mar 22  2023 ota_update_kernel.sh
-rwxrwxr-x    1 root     root           590 Mar 22  2023 ota_update_rootfs_squashfs.sh
-rw-rw-r--    1 root     root           570 Mar 22  2023 ota_update_rtos_bin.sh
-rw-rw-r--    1 root     root          5755 Mar 22  2023 ota_utils.sh
```

执行`sh /etc/ota_bin/network_ota_update.sh`

升级结果如下：

![9](X2000_iLock_V1.0_mmc_开发板OTA使用说明文档.assets/9.png)



`network_ota_update.sh` 脚本的升级过程如下：

```
 1 检查是否设置过下次启动项(检查文件是否存在/tmp/ota_boot_set),如未设置过,则继续
 2  下载测试服务器的 ota_config.in文件，解析当前ota版本号
 3  获取/etc/ota_info 中的版本号并与服务器ota版本号做对比，如果服务器更高，则继续
 4  下载测试服务器 ota_v1/ota_v1.ok 文件，如果无法下载，说明当前v1版本还没有准备好，则退出ota升级流程
 5  下载测试服务器 ota_v1/ota_update.in 文件
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
#查看ota分区对应的设备名
fdisk -l                 #查看分区表
cat /proc/partitions     #查看分区对应的设备

#写入启动分区名 ota:kernel2 或者 ota:kernel
echo ota:kernel2 > /dev/mmcblk0p5

#查看当前的启动分区名
dd if=/dev/mmcblk0p5 bs=1 count=256
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

目前采用的策略是双分区备份策略，即rootfs的升级程序升级kernel2,rootfs2,反之rootfs2升级kernel，rootfs。升级完成之后改变当前的启动分区为被升级的分区并且重启，启动分区的信息保存在“ota”分区中。



## 十   ota 常见问题汇总

1  ota升级完成无法进入到新的系统？

​    可以检查设备端的固件uboot是否支持ota功能。具体可以参考上面 4.1  uboot 添加支持 ota 功能。

2 设备端无法通过wget获取到升级服务器上的文件？

   可以检查电脑端的防火墙是否关闭。如果是局域网，要检查你设备端的网络和你的升级服务器上的网络是否在同一个网段。

3 如果遇到`region .sram' overflowed by 144 bytes`的问题，可以对spl部分进行裁剪，如关闭打印信息

```
文件bootloader/uboot-x2000/include/configs/x2000_base.h中
禁用此标志：
/* #define CONFIG_SPL_SERIAL_SUPPORT */
```

