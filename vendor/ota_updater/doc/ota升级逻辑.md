# ota 升级逻辑

ota 升级就是通过网络将服务器的镜像文件下载到设备,然后更新设备的分区

ota更新的单位是分区,不是某个文件
如果讲什么差分升级之类的升级类型,不属于君正支持的范畴!

## 分区的升级方法
**有自行开发能力的客户只需了解此节内容,其余章节仅供参考**
针对nand使用mtd utils 系列的命令

nandwrite 命令
flash_erase 命令
ubiformat 命令

emmc或sd卡使用dd命令

升级kernel
```
# 擦除kernel2 分区
flash_erase /dev/mtd3 0 0
# 写入kernel2 分区
nandwrite -m -p /dev/mtd3 /tmp/xImage
# 也可以通过管道的形式,流式升级
# '-' 表示命令从标准输入获取数据
# --input-size 表示数据长度
# get_kernel_size_bytes 获取kernel大小
# get_kernel_data 获取kernel数据并且输出到标准输出
kernel_size=`get_kernel_size_bytes`
get_kernel_data | nandwrite -m -p --input-size=$kernel_size /dev/mtd3 -
```

```
#emmc 写入方式
# 写入kernel2 分区
dd if=/tmp/xImage of=/dev/mmcblk0p3
```

升级rootfs(squashfs)
```
# 擦除rootfs2 分区
flash_erase /dev/mtd4 0 0
# 写入rootfs2 分区
nandwrite -m -p /dev/mtd4 /tmp/rootfs.squashfs
# 也可以通过管道的形式,流式升级
rootfs_size=`get_rootfs_size_bytes`
get_rootfs_data | nandwrite -m -p --input-size=$rootfs_size /dev/mtd4 -
```

```
#emmc 写入方式
# 写入rootfs2 分区
dd if=/tmp/rootfs.squashfs of=/dev/mmcblk0p4
```

升级rootfs(ubi),注意是`.ubi`不是`.ubifs`
```
# 擦除rootfs2 分区
flash_erase /dev/mtd4 0 0
# 写入rootfs2 分区
ubiformat /dev/mtd4 -y -f /tmp/rootfs.ubi
# 也可以通过管道的形式,流式升级
rootfs_size=`get_rootfs_size_bytes`
get_rootfs_data | ubiformat /dev/mtd4 -y --input-size=$rootfs_size -
```

设置ota启动分区
uboot会检查ota分区的内容,以决定启动哪个分区

```
# 擦除一块
flash_erase /dev/mtd5 0 1
# 写入启动分区名 ota:kernel2 或者 ota:kernel
printf "%-256s" "ota:kernel2" | nandwrite -s 0 -p /dev/mtd5 -
# 查看当前的启动分区名
nanddump -s 0 -l 256 /dev/mtd5 -a
```

```
#emmc 写入方式
# 写入启动分区名 ota:kernel2 或者 ota:kernel
printf "%-256s" "ota:kernel2" | dd of=/dev/mmcblk0p5
# 查看当前的启动分区名
hexdump /dev/mmcblk0p5 -c
```

## ota 分区策略

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

```
uboot    1M
kernel   4M
rootfs   48M
kernel2  4M
rootfs2  48M
ota      1M
userdata 剩余大小
```

## ota 升级文件组织策略
**君正不提供服务器,也不提供文件服务器的部署方法,客户需自行解决**
**ota的升级文件能被wget命令获取即可**
以我的测试机器举例子
http://192.168.43.52/ota/board_test/ 用于存放ota相关文件
对应于服务器本机的路径为 /var/www/html/ota/board_test

```shell
$ cd /var/www/html/ota/board_test/
$ ls ./
ota_config.in  ota_v2  ota_v3
```
1. **ota_config.in**
```shell
$ cat ota_config.in

current_version=3
```
**关键字 current_version** 用于定义当前最新的ota的版本
当`current_version=3`时,对应于目录`ota_v3/`存放ota的升级信息和文件

2. **ota_v'N'/** 目录('N'是数字,如ota_v3/)
```shell
$ cd ota_v3/
$ ls ./
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

**ota_update.in**
```shell
$ cat ota_update.in
ota_version=3

img_type=kernel
img_name=xImage
img_size=3403840
img_md5=588cdc3f67012baaaa6d2fed65857f54

img_type=rootfs
img_name=rootfs.squashfs
img_size=6144000
img_md5=afaebb32d9165c221553606aaa45ee58

```
**关键字 ota_version**: 做版本检查,避免忙中出错和实际版本不一致
关键字 img_type: 定义升级的内容类型:kernel rootfs
关键字 img_name: 升级的文件名,在升级包目录的文件名前缀
关键字 img_size: 升级文件的总大小,因为升级文件会被拆分成多个文件,默认按1Mbyte拆分
关键字 img_md5: 升级文件的md5sum值

以 kernel(xImage)为例子
将xImage(大小3403840)拆分成4个文件
xImage.0000 xImage.0001 xImage.0002 xImage.0003
对xImage 以及 xImage.000* 求md5sum
关键字 img_md5 保存 xImage.md5sum
ota_md5_xImage 保存 xImage.000*的md5sum 用于下载后设备校验

所有的xImage 相关文件被依次追加前一个xImage包文件的md5值作为后缀
ota_md5_xImage -> ota_md5_xImage."xImage.md5"
xImage.0000 -> xImage.0000."xImage.md5"
xImage.0001 -> xImage.0001."xImage.0000.md5"
xImage.0002 -> xImage.0002."xImage.0001.md5"
xImage.0003 -> xImage.0003."xImage.0002.md5"

这样的好处是可以确保下载的文件不会因为服务器缓存的问题而导致的包之间不一致的问题
即使文件名碰巧重合,设备端也可以通过ota_md5_xImage."img_md5"中的md5值进行校验

rootfs.squashfs 的逻辑和xImage 一致,
也是分成多个文件,依次追加md5值,并且保存md5值用做文件校验

**为啥不加密?**
君正不提供加密的逻辑,何况这就是个文件下载,有开发能力客户可以搞定
另外如果大家的加密逻辑是一样的,那么加密也就没有意义了

**ota_v3.ok** (3是版本号 ota_v'N'.ok)
**此文件的是做ota的最后一步确认**,制作ota镜像的时候不会有这个文件产生
这个文件需要客户手动生成,以确定此版本的ota验证成功,可以提供给设备端下载
设备端如果检测到此文件才会启动ota升级流程

注意:**ota 的旧版本千万不要去删除**
1 删除的时候可能导致用户正在进行的ota变成失败的状态
2 目前使用nand,一个ota版本也就是几十Mbyte,即使到1G,10个ota版本也才10G

注意:**ota 测试版本的服务器路径最好和发布版本在同一服务器**
测试版: http://192.168.43.52/ota/board_test/
发布版: http://192.168.43.52/ota/board_release/
这样测试版和发布版本环境几乎一致,容易提前测到一些问题
