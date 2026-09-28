# **x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档**



## 一  开发板介绍

### 1 快速上手

快速上手介绍见：

`doc/FAE文档/X2000H_Darwin_X2000_V2.0/x2000H_Darwin_v2.0_开发板快速上手说明.pdf`

### 2 开发版硬件详细介绍

详细硬件介绍见：

`doc/FAE文档/X2000H_Darwin_X2000_V2.0/Darwin_X2000_V2.0开发套件硬件手册_V2.0.pdf`



## 二  Recovery 升级方式原理和介质介绍

### 1 Recovery 升级简介和实现方法

​		对于君正的Recovery升级方式，是指开发版存在两套系统，一套为正常使用的系统，称为主系统（main_os），另一套为用于升级主系统的小系统（Recovery系统），在保证网络等基础功能的前提下，可以针对性的对其进行精简。

​		我们提供在main_os升级recovery系统以及在recovery系统中升级main_os的脚本。

​		基本步骤如下：

1. 将需要更新的recovery升级固件与主系统main_os升级固件放到服务器上。
2. 在main_os中，通过网络检查main_os升级可用性，检查并升级recovery系统。
3. 重启以进入recovery系统，执行脚本，从网络中获取并升级main_os。

​		注意：在recovery系统中执行main_os升级脚本时，如果main_os升级失败，不会自动切换回原系统（防止main_os在已经被修改的情况下进入main_os，而“变砖”），使用者请自行选择升级失败后的策略。



### 2 升级存储介质

​		君正目前支持升级的存储介质包含两种：mtd  和 mmc两种。其中mtd包含：nand 和 nor。



## 三  ota 编译配置

采用recovery方式进行ota升级，需要两套系统，建议打开两个项目，一个用于制作 main_os，一个用于制作recovery系统。**本次验证中制作main_os与recovery系统升级包的项目名分别为main_os_img_maker与recovery_img_maker。**

在本次验证中，我们会将原本版本为3的主系统，未进行烧录的recovery系统，将主系统与recovery系统都升级到版本4

### 1  uboot 添加支持 ota 功能

​		以Darwin_X2000_V2.0开发板编译配置为例，编译出的uboot固件需要支持ota功能（main_os与recovery系统均需要进行配置）：

首先需要确定你当前的编译配置使用的是那个uboot的编译配置文件，通过如下方法可以确定。使用工具 `tools/iconfigtool/IConfigToolApp$ ./IConfigTool` 

选择Config.in文件：`build/Config.in`

选择Config文件： `build/configs/x2000_darwin_v20_nand_defconfig`

![1](x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档.assets/1.png)

如上图，根据uboot相对路径和默认配置，可以确定uboot编译的配置文件为 `bootloader/uboot-x2000/boards.cfg` 的 `x2000_base_xImage_sfc_nand`,在文件 `boards.cfg`中搜索`x2000_base_xImage_sfc_nand`。看是否含有`SPL_OS_OTA_BOOT`的标志。有这个标志表示uboot支持ota的功能，没有就手动加上这个标志

//或选择使用预先写好的uboot默认配置`x2000_base_ota_xImage_sfc_nand`

```
x2000_base_xImage_sfc_nand	 mips        xburst2	x2000_base	 ingenic	x2000_v12	x2000_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_BOOT,SPL_OS_OTA_BOOT,RMEM_MB=16,SPL_PARAMS_FIXER
```



### 2  buildroot  ota编译配置注意事项

​	君正ota采用的是只读系统，所以便于用户开发需要可读可写的分区，我们设置了一个data分区（在IConfigTool中启动的自动挂载脚本），该分区是可读可写分区。这样可以便于用户存放可读可写的文件。

​	在data分区中，会创建一个隐藏文件夹(.recovery)用来**保存recovery系统的版本**，如开发者不对data分区自动挂载，请自行更改recovery版本信息存放位置（`device_tools/recovery_version_utils.sh`中的`recovery_version_dir`），**否则**每次升级因无法检测到当前recovery版本，每次都会执行recovery系统的升级。

如需要ubifs格式的文件系统，需要buildroot支持可以打包该格式的文件系统配置。

方法如下,进入buildroot 执行:

`/buildroot/buildroot$ make menuconfig`  在`Filesystem images`目录下按照下图选择相应的配置即可。

![2](x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档.assets/2.png)

备注：buildroot的配置修改以后，需要执行拷贝到其主配置上去。比如该buildroot的配置为：`build/configs/buildroot/buildroot_wifi_common_defconfig`（对应IConfigTool中`工程/编译配置-buildroot配置文件`）

所以上面修改了配置以后，执行如下操作：

`buildroot/buildroot$ cp .config ../../build/configs/buildroot/buildroot_wifi_common_defconfig`

这样你再整体编译的时候，buildroot的修改才会生效（当使用`make x2000_darwin_v20_nand_defconfig`进行整体配置编译后，会覆盖buildroot_wifi_common_defconfig配置，此时也需重新将配置拷贝到主配置中）。



## 四  制作 main_os 与 recovery系统 升级固件

​		以Darwin_X2000_V2.0开发板编译配置为例制作OTA升级固件。使用ota升级为联网升级，参照`doc/FAE文档/X2000H_Darwin_X2000_V2.0/x2000H_Darwin_v2.0_开发板快速上手说明.pdf`进行以太网与wifi设置。



### 编译 main_os 升级包固件

#### 1 ota相关配置

使用工具 `tools/iconfigtool/IConfigToolApp$ ./IConfigTool` ,选择配置文件：  `build/configs/x2000_darwin_v20_nand_defconfig`

配置如下：

![3](x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档.assets/3.png)

![4](x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档.assets/4.png)

从上往下，每项宏的配置对应的功能如下所示：

```shell
1：Storage medium(存储介质)(APP_ota_updater_storage_medium):
//板子的存储介质，包含两种类型，nand和mmc，可以根据自己的设备存储介质选择不同的类型

2：当前系统的ota版本号(每次编译新的ota版本时都应该递增此值)(APP_ota_updater_version)，对于recovery系统，也表示recovery系统的版本号
//当前ota的版本号，会写入文件系统的 /etc/ota_info 'ota_version=N'
该版本号是指打包升级包的版本号，如果你需要制作版本号为4,该处 N 需要修改为4

3：ota 升级服务器的地址(App_ota_updater_site)
//ota的服务器地址路径，会写入文件系统的 /etc/ota_info 'ota_site=...'

4：ota 分包大小(App_ota_updater_block_size)
//ota分包大小，制作ota镜像时会将ota镜像按此大小分包

5：ota kernel 镜像路径(App_ota_updater_kernel_img_path)
//ota kernel镜像路径，制作ota镜像时需要

6：ota rootfs 镜像路径(App_ota_updater_rootfs_img_path)
//ota rootfs镜像路径，制作ota镜像时需要
```



#### 2 编译 ota 升级包固件

##### 编译 main_os 升级包固件

先使用配置 `build/configs/x2000_darwin_v20_nand_defconfig` 进行整体编译以后，然后进入build，执行如下指令：

```
build$ make ota_img
#注意： 在编译ota_img 之前必须得先保证编译好buildroot镜像文件与kernel镜像文件
```

在 "工程/build"目录下编译ota升级镜像，会在 "工程/build/output/ota" 目录下生成ota镜像文件：

```shell
cd output/ota/
ls

#保存 rootfs.squashfs.000*的md5sum 用于下载后设备校验
ota_md5_rootfs.squashfs.df4cc0f4e8d7c11b16b94e4fd200326e

#保存xImage.000*的md5sum 用于下载后设备校验
ota_md5_xImage.f814dfceac0e6633997f71c5313a5ce7

#镜像文件拆分的配置
ota_update.in

#"rootfs.squashfs.*"表示被拆分后的buildroot镜像文件
rootfs.squashfs.0000.df4cc0f4e8d7c11b16b94e4fd200326e
rootfs.squashfs.0001.c8827dca1a81c64e3a509be9a1358904
rootfs.squashfs.0002.805d3c6077de673126261d62072228be
rootfs.squashfs.0003.cf99cc192d32fe70054b5268b8c4d938
rootfs.squashfs.0004.b8da977feecdbbf82c848292192aaa52
rootfs.squashfs.0005.ca2f66743db7b8336088053967ef4da6
rootfs.squashfs.0006.dc8f97cafa7c65e350203cbfe0bd1fb2
rootfs.squashfs.0007.cd6c75f069e3d78d410bb763f3c68526
rootfs.squashfs.0008.90837e629ab09521c41ef581b9610bcb
rootfs.squashfs.0009.17236e1e8878f18243a444768df406ee
rootfs.squashfs.0010.76d58741eaae6f18ca793a33332ef7a3
rootfs.squashfs.0011.466378a54132911f490444a9e343bf73
rootfs.squashfs.0012.6599a1421f2b67d10ba5927b0eb88b68

#"xImage.*"表示被拆分后的kernel镜像文件
xImage.0000.f814dfceac0e6633997f71c5313a5ce7
xImage.0001.7be8bb40970d23ade3362fe57ffcda37
xImage.0002.ae5a5c19c199954fe1c0840f8e1cfb3c
xImage.0003.8a31a411504341992009ce96ee67f237

```

其中 ota_update.in文件

build$ cat output/ota/ota_update.in 

```shell
ota_version=4

img_type=kernel
img_name=xImage
img_size=3772480
img_md5=f814dfceac0e6633997f71c5313a5ce7

img_type=rootfs
img_name=rootfs.squashfs
img_size=12861440
img_md5=df4cc0f4e8d7c11b16b94e4fd200326e

#关键字 ota_version:做版本检查，避免忙中出错和实际版本不一致
#关键字 img_type:定义升级的内容类型：kernel rootfs
#关键字 img_name:升级的文件名，在升级包目录的文件名前缀
#关键字img_size:升级文件的总大小，因为升级文件会被拆分成多个文件，默认按1Mbyte拆分
#关键字img_md5:升级文件的md5sum值

```



### 编译 recovery 升级包固件

recovery 升级包固件的编译方式与 main_os 的编译方式相同，请参照之前的编译过程进行编译，本次测试将



## 五    OTA 烧录工具配置

### 存储介质为NAND 的烧录工具配置

以Darwin_X2000_V2.0开发板 OTA 烧录为例，进行烧录工具的说明。配置如下：

//板级信息也可选x2000h_sfc_nand_ota_lpddr3_linux.cfg

![5](x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档.assets/5.png)

![6](x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档.assets/6.png)

![7](x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档.assets/7.png)

![8](x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档.assets/8.png)



对于以上的kernel与rootfs，烧录为main_os所编译出来的固件，对于kernel2与rootfs2可以烧录为recovery所编译出来的固件（这时，需要在之前提到的data分区中写入./recovery/recovery_version文件，以及`recovery_version=?` ?为recovery的版本，对应ota版本号）

**或不进行kernel2与rootfs2的烧录，通过网络服务器下载recovery固件，并写入到对应分区**

*填写label和选择ops后会自动补齐offset，均需要与sfc中的分区信息一致*

*ota 分区烧录的文件可以为一个空文件。*

*userdata为/usr/data下挂载的分区，根据需要设置大小与文件。*



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

以`Darwin_X2000_V2.0`开发板为例，将版本升级至4，我的电脑的地址是10.4.229.4，所以地址：http://10.4.229.4/ota/board_x2000/用于存放ota相关的文件，对应于服务器本机的路径为：`/var/www/html/ota/board_x2000`，

该目录下的文件需要自己添加：

```shell
cd /var/www/html/ota/board_x2000/

tree -L 2
.
├── main_os
│   └── ota_v4
├── ota_config.in
└── recovery
    └── ota_v4
```

1 添加ota_config.in文件，该文件内容如下：

```shell
/*
关键字 current_version 用于定义当前最新的main_os的版本
当`current_version=4`时,对应于目录`ota_v4/`存放ota的main_os的升级信息和文件
关键字 recovery_version 用于定义当前最新的recovery的版本
当`recovery_version=4`时,对应于目录`ota_v4/`存放ota的recovery的升级信息和文件
*/
current_version=4
recovery_version=4
```

2 ota_v'N'/目录 （'N'是数字，这里添加ota_v4)，需要自己手动添加，并将编译好的main_os与recovery升级镜像文件复制到该文件夹

```shell
 cp main_os_img_maker/build/output/ota/* /var/www/html/ota/board_x2000/main_os/ota_v4/ -arf    #复制main_os镜像文件到服务器
 cp recovery_img_maker/build/output/ota/* /var/www/html/ota/board_x2000/recovery/ota_v4/ -arf   #复制recovery镜像文件到服务器
```

3 手动创建 ota_v4.ok 到 `/var/www/html/ota/board_x2000/main_os/ota_v4/` 与 `/var/www/html/ota/board_x2000/recovery/ota_v4/` 这两个文件夹下，最终确定 main_os与recovery 的 ota_v4版本发布

```shell
ota_v4.ok (4是版本号 ota_v'N'.ok 该文件的内容为空即可)
此文件是做 ota 的最后一步确认，制作 ota镜像的时候不会有这个文件产生，这个文件需要客户自己手动创建生成，以确定此版本的ota验证是成功可行的，可以提供给外部的设备端进行下载更新。设备端如果检测到此文件才会启动ota升级流程
```

所以`/var/www/html/ota/board_x2000`文件夹，服务器上的目录与内容如下：

```
lih@lih:/var/www/html/ota/board_x2000/$ tree
.
├── main_os
│   └── ota_v4
│       ├── ota_md5_rootfs.squashfs.df4cc0f4e8d7c11b16b94e4fd200326e
│       ├── ota_md5_xImage.f814dfceac0e6633997f71c5313a5ce7
│       ├── ota_update.in
│       ├── ota_v4.ok
│       ├── rootfs.squashfs.0000.df4cc0f4e8d7c11b16b94e4fd200326e
│       ├── rootfs.squashfs.0001.c8827dca1a81c64e3a509be9a1358904
│       ├── rootfs.squashfs.0002.805d3c6077de673126261d62072228be
│       ├── rootfs.squashfs.0003.cf99cc192d32fe70054b5268b8c4d938
│       ├── rootfs.squashfs.0004.b8da977feecdbbf82c848292192aaa52
│       ├── rootfs.squashfs.0005.ca2f66743db7b8336088053967ef4da6
│       ├── rootfs.squashfs.0006.dc8f97cafa7c65e350203cbfe0bd1fb2
│       ├── rootfs.squashfs.0007.cd6c75f069e3d78d410bb763f3c68526
│       ├── rootfs.squashfs.0008.90837e629ab09521c41ef581b9610bcb
│       ├── rootfs.squashfs.0009.17236e1e8878f18243a444768df406ee
│       ├── rootfs.squashfs.0010.76d58741eaae6f18ca793a33332ef7a3
│       ├── rootfs.squashfs.0011.466378a54132911f490444a9e343bf73
│       ├── rootfs.squashfs.0012.6599a1421f2b67d10ba5927b0eb88b68
│       ├── xImage.0000.f814dfceac0e6633997f71c5313a5ce7
│       ├── xImage.0001.7be8bb40970d23ade3362fe57ffcda37
│       ├── xImage.0002.ae5a5c19c199954fe1c0840f8e1cfb3c
│       └── xImage.0003.8a31a411504341992009ce96ee67f237
├── ota_config.in
└── recovery
    └── ota_v4
        ├── ota_md5_rootfs.squashfs.df4cc0f4e8d7c11b16b94e4fd200326e
        ├── ota_md5_xImage.f814dfceac0e6633997f71c5313a5ce7
        ├── ota_update.in
        ├── ota_v4.ok
        ├── rootfs.squashfs.0000.df4cc0f4e8d7c11b16b94e4fd200326e
        ├── rootfs.squashfs.0001.c8827dca1a81c64e3a509be9a1358904
        ├── rootfs.squashfs.0002.805d3c6077de673126261d62072228be
        ├── rootfs.squashfs.0003.cf99cc192d32fe70054b5268b8c4d938
        ├── rootfs.squashfs.0004.b8da977feecdbbf82c848292192aaa52
        ├── rootfs.squashfs.0005.ca2f66743db7b8336088053967ef4da6
        ├── rootfs.squashfs.0006.dc8f97cafa7c65e350203cbfe0bd1fb2
        ├── rootfs.squashfs.0007.cd6c75f069e3d78d410bb763f3c68526
        ├── rootfs.squashfs.0008.90837e629ab09521c41ef581b9610bcb
        ├── rootfs.squashfs.0009.17236e1e8878f18243a444768df406ee
        ├── rootfs.squashfs.0010.76d58741eaae6f18ca793a33332ef7a3
        ├── rootfs.squashfs.0011.466378a54132911f490444a9e343bf73
        ├── rootfs.squashfs.0012.6599a1421f2b67d10ba5927b0eb88b68
        ├── xImage.0000.f814dfceac0e6633997f71c5313a5ce7
        ├── xImage.0001.7be8bb40970d23ade3362fe57ffcda37
        ├── xImage.0002.ae5a5c19c199954fe1c0840f8e1cfb3c
        └── xImage.0003.8a31a411504341992009ce96ee67f237

```

`ota_v4/ota_update.in` 里面放的是版本号为4的升级包，其中ota_version=4，该值跟第四章第1条那里的设置相对应。



## 八   OTA  升级的流程

### 1  ota 升级脚本

调用ota升级脚本，ota的升级脚本在 `/etc/ota_bin/` 目录下，ota的配置在文件 `/etc/ota_info` 中，ota_info的内容如下：

```shell
# cat /etc/ota_info 

ota_version=4
ota_site=http://10.4.229.4/ota/board_x2000
```

ota_info 配置文件是由前面 "make ota_img" 的命令所生成的：

ota的升级脚本文件：

```shell
# ls /etc/ota_bin/ -l
total 84
-rwxrwxr-x    1 root     root         14119 Apr 14  2023 local_ota_update.sh
-rwxrwxr-x    1 root     root          8323 Apr 14  2023 main_os_info_check.sh
-rwxrwxr-x    1 root     root         14076 Apr 14  2023 network_main_os_update_recovery.sh
-rwxrwxr-x    1 root     root         14134 Apr 14  2023 network_ota_update.sh
-rwxrwxr-x    1 root     root         14309 Apr 14  2023 network_recovery_update_main_os.sh
-rwxrwxr-x    1 root     root          2596 Apr 14  2023 ota_img_data_provider.sh
-rw-rw-r--    1 root     root          4319 Apr 14  2023 ota_local_method.sh
-rwxrwxr-x    1 root     root          1077 Apr 14  2023 ota_update_kernel.sh
-rwxrwxr-x    1 root     root          1077 Apr 14  2023 ota_update_rootfs_squashfs.sh
-rwxrwxr-x    1 root     root          1053 Apr 14  2023 ota_update_rtos_bin.sh
-rw-rw-r--    1 root     root          5798 Apr 14  2023 ota_utils.sh
-rw-rw-r--    1 root     root           727 Apr 14  2023 recovery_version_utils.sh

```

执行`sh /etc/ota_bin/network_main_os_update_recovery.sh`

![9](x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档.assets/9.png)

首先对服务器的main_os版本进行检测，如服务器版本更高，则继续执行

如recovery版本不匹配或没有进行`.recovery/recovery_version`中版本的设置，则会从服务器中获取可用的recovery版本

检查可用后，进行重启操作并执行`sh /etc/ota_bin/network_recovery_update_main_os.sh`对main_os进行升级，升级结果如图

![10](x2000H_Darwin_v2.0_mtd_开发板OTA升级之Recovery方式升级说明文档.assets/10.png)



"network_main_os_update_recovery.sh" 脚本的升级过程如下：

```shell
 1 检查是否设置过下次启动项(检查文件是否存在/tmp/ota_boot_set),如未设置过,则继续
 2 下载测试服务器中的 ota_config.in文件,获取字段 current_verison= 后的版本信息,为服务器上可用的主系统升级包镜像的版本号
 3 获取本地的主系统版本信息,信息存放在/etc/ota_info中,并与服务器中的版本比较,如服务器版本更高,则继续
 4 下载测试服务器中的 main_os/ota_v4/ota_v4.ok 文件,如果无法下载,说明当前v4版本还没有准备好,则退出
 5 下载测试服务器中的 main_os/ota_v4/ota_update.in 文件
 6 获取 ota_version= 字段,如果和 ota_config.in 中获取的版本信息不相等,则退出
 7 获取主系统的kernel rootfs分区,检查kernel 分区大小,检查rootfs 分区大小是否满足升级包要求,如不满足,则退出
 8 下载测试服务器中的 ota_config.in,获取字段 recovery_version= 后的版本信息,为服务器上可用的recovery系统升级包镜像的版本号
 9 获取本地的recovery系统版本信息,信息存放在/usr/data/.recovery/recovery_version中,并与服务器中的版本比较,如服务器版本更高,则继续执行升级操作,否则,无需升级recovery系统,设置启动分区为recovery系统,并退出,返回值为0
10 下载测试服务器中的 recovery/ota_v4/ota_v4.ok 文件,如果无法下载,说明当前v4版本还没有准备好,则退出
11 下载测试服务器中的 recovery/ota_v4/ota_update.in 文件
12 获取 ota_version= 字段,如果和 ota_config.in 中获取的版本信息不相等,则退出
13  获取img_type img_name img_size img_md5 字段，并且验证和保存
    ota_kernel ota_kernel_name ota_kernel_size ota_kernel_md5
    ota_rootfs ota_rootfs_name ota_rootfs_size ota_rootfs_md5
14  获取需要升级的kernel rootfs分区，检查kernel分区大小，检查rootfs 分区大小是否满足升级包要求
15  开始升级kernel
16  启动升级脚本 /etc/ota_bin/ota_update_kernel.sh
17  调用download_ota_img 函数下载 xImage 相关文件，校验 md5值
18  去掉文件 md5 后缀，生成 xImage.000N，再生成 xImage.000N.ok 通知升级脚本读取 xImage.000N数据
19  继续下载新的 xImage.000N+1.md5文件，等待xImage.000N.done产生，并删除 xImage.000N
20  继续11的步骤，直到xImage 相关文件下载完成
21  等待xImage.ok 生成，表示升级脚本成功，或者 xImage.failed 表示升级脚本失败
22  按前面的步骤升级rootfs
23  如果升级成功，则设置启动分区为recovery系统的分区，返回值0
24  如果什么都没有升级，也没有出错，则返回值2,中间任意步骤出错，返回1
 
脚本执行后，返回值为0则表示已经准备好进入recovery系统对主系统进行升级，且已经设置下次启动分区为recovery系统，重启即可进入recovery系统
```

"network_recovery_update_main_os.sh"脚本的升级过程如下：

```shell
 1  下载测试服务器的 ota_config.in文件，解析main_os版本号
 2  下载测试服务器 main_os/ota_v4/ota_v4.ok 文件，如果无法下载，说明当前v4版本还没有准备好，则退出并提示错误并退出升级流程（理论上不会存在这种情况，因为在main_os中就会进行此项检验）
 3  下载服务器中 ota_v4/ota_update.in 文件
 4  获取 ota_version=字段，如果和当前版本不相等，则退出main_os升级流程（在main_os中已做检验）
 5  获取img_type img_name img_size img_md5 字段，并且验证和保存
    ota_kernel ota_kernel_name ota_kernel_size ota_kernel_md5
    ota_rootfs ota_rootfs_name ota_rootfs_size ota_rootfs_md5
 6  获取需要升级的kernel rootfs分区，检查kernel分区大小，检查rootfs 分区大小是否满足升级包要求（在main_os中已做检验）
 7  开始升级kernel
 8  启动升级脚本 /etc/ota_bin/ota_update_kernel.sh
 9  调用download_ota_img 函数下载 xImage 相关文件，校验 md5值
10  去掉文件 md5 后缀，生成 xImage.000N，再生成 xImage.000N.ok 通知升级脚本读取 xImage.000N数据
11  继续下载新的 xImage.000N+1.md5文件，等待xImage.000N.done产生，并删除 xImage.000N
12  继续11的步骤，直到xImage 相关文件下载完成
13  等待xImage.ok 生成，表示升级脚本成功，或者 xImage.failed 表示升级脚本失败
14  按前面的步骤升级rootfs
15  如果升级成功，则设置启动分区为main_os的分区，返回值0
16  如果什么都没有升级，也没有出错，则返回值2,中间任意步骤出错，返回1

成功完成升级后，脚本返回值为0，重启即可返回主系统，如不成功，请依据错误信息自行设计升级失败的处理策略。
```



### 2 ota 启动分区

设置ota启动分区，ota分区用来存储关键字`ota:kernel`或`ota:kernel2`

一般将ota:kernel作为main_os的启动标志，`ota:kernel2`作为recovery系统的启动标志，uboot会检查ota分区的内容，以决定启动哪个分区：

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

所以对于recovery进行ota升级的方法，必须为ota升级程序划分recovery系统所用的kernel分区和rootfs分区，另外uboot不能进行升级，一旦升级失败机器将无法启动。

recovery进行ota方式采用的策略为：main_os中的程序（在rootfs分区）升级recovery系统（对应kernel2与rootfs2） 反之recovery系统升级main_os（对应kernel，rootfs）。两个系统的启动根据”ota“中的关键字进行启动，分区列表的结构如下，针对 128M nand ，分区大小可以根据实际情况进行调整。

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



