# ota升级操作说明文档

## 1.ota升级配置流程

![](img/1.png)

![](img/2.png)

由上往下，每项宏的配置对应的功能如下所示：

```c
APP_ota_updater_version
//当前ota的版本号,会写入文件系统的/etc/ota_ota_info `ota_version=N`

App_ota_updater_site
//ota的服务器地址路径,会写入文件系统的/etc/ota_ota_info `ota_site=...`

App_ota_updater_block_size
//ota 分包大小,制作ota镜像时会将ota镜像按此大小分包

App_ota_updater_kernel_img_path
//ota kernel 镜像路径,制作ota镜像时需要

App_ota_updater_rootfs_img_path
//ota rootfs 镜像路径,制作ota镜像时需要
```



## 2.ota升级使用流程

### 2.1 编译

```shell
make ota_img
#注意：在编译ota_img之前必须得先编译好buildroot镜像文件与kernel镜像文件
```

在"工程/build"目录下编译ota升级镜像，会在"工程/build/output/ota"目录下生成ota镜像文件：

```shell
cd output/ota/
ls

# 保存rootfs.squashfs.000*的md5sum 用于下载后设备校验
ota_md5_rootfs.squashfs.afaebb32d9165c221553606aaa45ee58

 #保存xImage.000*的md5sum 用于下载后设备校验
ota_md5_xImage.588cdc3f67012baaaa6d2fed65857f54

#镜像文件拆分的配置
ota_update.in

#"rootfs.squashfs.*"表示被拆分后的buildroot镜像文件
rootfs.squashfs.0000.afaebb32d9165c221553606aaa45ee58
rootfs.squashfs.0001.8c601f54cc49f1c2d4fe7b7ca1c46a43
rootfs.squashfs.0002.fdb15f5904fe19f9df1d162c293f0b40
rootfs.squashfs.0003.d3c1db55e1bb1f8dff8a781671e5690d
rootfs.squashfs.0004.98ffc9c2d6eab73262e26f261f7ee94f
rootfs.squashfs.0005.9a89824d02e424b238c8674f121b03c7

#"xImage.*" 表示被拆分后的kernel镜像文件
xImage.0000.588cdc3f67012baaaa6d2fed65857f54
xImage.0001.3896238c825993127c0f62e75ec59612
xImage.0002.ce5a296471261c926ed9280415c5769d
xImage.0003.1ec515e6235e05a652421299571a45b1
```

ota_update.in文件：

```shell
ota_version=3

img_type=kernel
img_name=xImage
img_size=3403840
img_md5=588cdc3f67012baaaa6d2fed65857f54

img_type=rootfs
img_name=rootfs.squashfs
img_size=6144000
img_md5=afaebb32d9165c221553606aaa45ee58

#关键字 ota_version: 做版本检查,避免忙中出错和实际版本不一致
#关键字 img_type: 定义升级的内容类型:kernel rootfs
#关键字 img_name: 升级的文件名,在升级包目录的文件名前缀
#关键字 img_size: 升级文件的总大小,因为升级文件会被拆分成多个文件,默认按1Mbyte拆分
#关键字 img_md5: 升级文件的md5sum值
```

ota文件组织策略详见3.2章



### 2.2 服务器文件的部署

>   君正不提供服务器，客户需自行解决，ota的升级文件能被wget命令获取即可。

以我的测试机器举例子，现在我将升级至版本3。http://192.168.43.52/ota/board_test/ 用于存放ota相关文件，对应于服务器本机的路径为 ~/http/ota/board_test/，该目录下的文件需自己添加：

```shell
cd ~/http/ota/board_test/
ls ./

ota_v2                      #版本为2对应的ota升级文件夹
ota_v3                      #版本为3对应的ota升级文件夹
ota_config.in         #定义当前最新版本的ota版本
```

1.添加ota_config.in，其文件内容如下

```c
/*
关键字 current_version 用于定义当前最新的ota的版本
当`current_version=3`时,对应于目录`ota_v3/`存放ota的升级信息和文件
*/
current_version=3
```

2.**ota_v'N'/** 目录('N'是数字,这里添加ota_v3/)，需自己手动添加，并将编译好的ota升级镜像文件复制到该文件夹

```shell
cp build/output/ota/*  ~/http/ota/board_test/ota_v3       #复制ota镜像文件到服务器
```

3.手动创建ota_v3.ok到ota_v3文件夹下，最终确定ota_v3版本发布

>   **ota_v3.ok** (3是版本号 ota_v'N'.ok)
>   **此文件的是做ota的最后一步确认**,制作ota镜像的时候不会有这个文件产生
>   这个文件需要客户手动生成,以确定此版本的ota验证成功,可以提供给设备端下载
>   设备端如果检测到此文件才会启动ota升级流程

所以ota_v3文件夹的内容如下：

```c
ota_md5_rootfs.squashfs.afaebb32d9165c221553606aaa45ee58
ota_md5_xImage.588cdc3f67012baaaa6d2fed65857f54
ota_update.in
ota_v3.ok
rootfs.squashfs.0000.afaebb32d9165c221553606aaa45ee58
rootfs.squashfs.0001.8c601f54cc49f1c2d4fe7b7ca1c46a43
rootfs.squashfs.0002.fdb15f5904fe19f9df1d162c293f0b40
rootfs.squashfs.0003.d3c1db55e1bb1f8dff8a781671e5690d
rootfs.squashfs.0004.98ffc9c2d6eab73262e26f261f7ee94f
rootfs.squashfs.0005.9a89824d02e424b238c8674f121b03c7
xImage.0000.588cdc3f67012baaaa6d2fed65857f54
xImage.0001.3896238c825993127c0f62e75ec59612
xImage.0002.ce5a296471261c926ed9280415c5769d
xImage.0003.1ec515e6235e05a652421299571a45b1
```

### 2.3 设备ota升级的流程

1.调用ota升级脚本，ota的升级脚本在"/etc/ota_bin/"目录下，ota的配置在文件"/etc/ota_info"

```shell
sh /etc/ota_bin/ota_update.sh
```

ota_info配置文件是由前面"make ota_img"的命令所生成的：

```shell
ota_version=0                                                                              #设备当前版本号
ota_site=http://192.168.43.52/ota/board_test/            #ota的服务器路径地址
```

"ota_update.sh"脚本的升级过程如下：

>   1 下载测试服务器的 ota_config.in 文件,解析当前ota版本号
>   2 获取/etc/ota_info 中的版本号并与服务器ota 版本号做对比,如果服务器更高,则继续
>   3 下载测试服务器 ota_v3/ota_v3.ok 文件,如果无法下载,说明当前v3版本还没有准备好,则退出
>   4 下载测试服务器 ota_v3/ota_update.in 文件,
>   5 获取 ota_version= 字段,如果和当前版本不相等,则退出
>   6 获取img_type img_name img_size img_md5 字段,并且验证和保存
>     ota_kernel ota_kernel_name ota_kernel_size ota_kernel_md5
>     ota_rootfs ota_rootfs_name ota_rootfs_size ota_rootfs_md5
>   7 获取需要升级的kernel rootfs分区,检查kernel 分区大小,检查rootfs 分区大小是否满足升级包要求
>   8 开始升级kernel
>   9 启动升级脚本 /etc/ota_bin/ota_update_kernel.sh
>   10 调用download_ota_img 函数下载xImage 相关文件,校验md5值
>   11 去掉文件md5后缀,生成xImage.000N,再生成xImage.000N.ok通知升级脚本读取xImage.000N数据
>   12 继续下载新的xImage.000N+1.md5 文件,等待xImage.000N.done产生,并删除xImage.000N
>   13 继续11的步骤,直到xImage 相关文件下载完成
>   14 等待xImage.ok 生成,表示升级脚本成功,或者xImage.failed 表示升级脚本失败
>   15 按前面的步骤升级rootfs
>   16 如果升级成功,则设置启动分区为刚才升级的分区,返回值0
>   17 如果什么都没有升级,也没有出错,则返回值2,中间任意步骤出错,返回1
>
>
>
>   /tmp/ota 产生ota过程中下载的文件,以及xImage.000N.ok xImage.000N.done 这种临时信号文件
>   /tmp/ota_wget_msg/ 产生wget下载时的log

2.设置ota启动分区

>   若升级成功会自动设置启动分区为升级的分区，若想设置回来可以设置ota分区的内容，uboot会检查ota分区的内容,以决定启动哪个分区：

```shell
# 擦除ota分区的一块数据
flash_erase /dev/mtd5 0 1
# 写入启动分区名 ota:kernel2 或者 ota:kernel
printf "%-256s" "ota:kernel2" | nandwrite -s 0 -p /dev/mtd5 -
# 查看当前的启动分区名
nanddump -s 0 -l 256 /dev/mtd5 -a

#注意：/dev/mtd5 对应ota分区，可用"cat /proc/mtd"命令查看
```



## 3.补充说明

### 3.1 ota_bin目录

```shell
#ota 升级过程均使用脚本编写,都有中文注释

ota_update.sh
#ota 升级主程序,探测是否需要ota升级,以及下载ota镜像执行ota升级流程

ota_local_method.sh
#ota 升级程序的设备端方法和升级流程回调的实现

ota_update_kernel.sh
#ota kernel的升级方法（被ota_update.sh调用）

ota_update_rootfs_squashfs.sh
#ota rootfs.squashfs 的升级方法（被ota_update.sh调用）

ota_utils.sh
#ota 用到的实用函数集

ota_img_data_provider.sh
#ota 流式升级方式的数据提供者
#被ota_update_kernel.sh ota_update_rootfs_squashfs.sh 调用
```

### 3.2 ota文件组织策略

>   编译ota升级镜像时，kernel和buildroot的镜像文件被拆分成多份，分成多个文件,依次追加md5值,并且保存md5值用做文件校验，这样的好处是可以确保下载的文件不会因为服务器缓存的问题而导致的包之间不一致的问题。即使文件名碰巧重合,设备端也可以通过文件后缀的md5值进行校验

1.以 kernel(xImage)为例子，将xImage(大小3403840)拆分成4个文件：

```shell
xImage.0000
xImage.0001
xImage.0002
xImage.0003
```

2.对xImage 以及 xImage.000* 求md5sum，关键字 img_md5 保存 xImage.md5sum，ota_md5_xImage 保存 xImage.000*的md5sum 用于下载后设备校验

3.所有的xImage 相关文件被依次追加前一个xImage包文件的md5值作为后缀：

```c
ota_md5_xImage -> ota_md5_xImage."xImage.md5"
xImage.0000 -> xImage.0000."xImage.md5"
xImage.0001 -> xImage.0001."xImage.0000.md5"
xImage.0002 -> xImage.0002."xImage.0001.md5"
xImage.0003 -> xImage.0003."xImage.0002.md5"
```

### 3.3 ota 分区策略

更新设备分区的时候必须保证当前被更新的分区不被占用,
所以roofs分区的程序不能更新rootfs分区的内容,只能去更新另外一个rootfs分区

从安全的角度上也是一样,如果当前能更新程序所在的分区,比如rootfs打包在kernel中的情况
但是遇到掉电/死机/重启等不正常的时候,再一次启动分区的内容不是完整的,所以设备就会无法启动

所以必须为ota升级程序划分专门的kernel分区和rootfs分区
另外uboot不能进行升级,一旦升级失败机器将无法启动

**目前采用的策略是双分区备份策略**
即rootfs的升级程序升级kernel2, rootfs2,反之 rootfs2 升级kernel, rootfs
升级完成之后改变当前的启动分区为被升级的分区并且重启,启动分区的信息保存在"ota"分区中
分区列表的结构如下,针对128M nand,分区大小可以根据实际情况调整

```shell
uboot    1M
kernel   4M
rootfs   48M
kernel2  4M
rootfs2  48M
ota      1M
userdata 剩余大小
```

### 3.4 简单的服务器搭建步骤（仅供参考）

1.安装apache2

```shell
apt-get install apache2
```

其配置文件在：/etc/apache2/apache2.conf

2.启动服务，将本机作为服务器

```shell
service httpd start
```

3.由配置文件可知，服务器默认的访问路径在`/var/www/html`目录下，在该目录下创建文件夹或软链接

4.与服务器一个局域网的其他机器，即可通过访问服务器的ip地址来访问该服务器：

```shell
http://194.169.1.3.134/file
```

