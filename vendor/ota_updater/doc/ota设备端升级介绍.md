# ota设备端升级介绍

## ota_updater的源码
ota_updater的源码在 ota_updater/ 工程
```shell
$ cd ota_updater/
$ ls ./
Config.in  device_tools  doc  host_tools  Makefile  ota  tools
```

### 文件 `Config.in`
定义ota相关的选项

APP_ota_updater_version
当前ota的版本号,会写入文件系统的/etc/ota_ota_info `ota_version=N`

App_ota_updater_site
ota的服务器地址路径,会写入文件系统的/etc/ota_ota_info `ota_site=...`

App_ota_updater_block_size
ota 分包大小,制作ota镜像时会将ota镜像按此大小分包

App_ota_updater_kernel_img_path
ota kernel 镜像路径,制作ota镜像时需要

App_ota_updater_rootfs_img_path
ota rootfs 镜像路径,制作ota镜像时需要

### 文件 Makefile
定义ota工程相关命令

make install
安装ota 升级脚本到文件系统的/etc/ota_bin/目录
会用到Config.in的配置, 将ota版本等选项写入到文件系统的/etc/ota_ota_info

make clean_install
清除文件系统的 /etc/ota_bin/ /etc/ota_ota_info

make ota_img
会用到Config.in的配置, 制作ota升级镜像,用于服务器部署,生成在ota 目录

### 目录 device_tools
对应于文件系统的 /etc/ota_bin/ 目录
ota 升级过程均使用脚本编写,都有中文注释

#### 升级主程序

local_ota_update.sh
ota 本地升级主程序，探测是否需要 ota 升级，以及本地读取 ota 镜像执行 ota 升级流程

network_ota_update.sh
ota 网络升级主程序，探测是否需要 ota 升级，以及下载 ota 镜像执行 ota 升级流程

network_main_os_update_recovery.sh
ota recovery方式进行ota升级的主程序，主系统 中用来探测是否需要 ota 升级，下载 recovery 镜像以及升级 recovery 系统

network_recovery_update_main_os.sh
ota recovery方式进行ota升级的主程序，recovery系统 中用来下载 主系统 镜像以及升级 主系统

#### 功能性程序与函数

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

### 目录 host_tools
存放ota镜像制作的脚本

mk_ota.sh
制作ota镜像的主程序,输出到ota/ 目录
参数 ota_version=? ota 版本号
参数 block_size=? ota 镜像分包大小
参数 kernel_img=? kernel 镜像文件
参数 rootfs_img=? rootfs 镜像文件

mk_ota_img.sh
用于制作拆分ota 镜像,重命名+md5,保存md5值
参数 1 ota 镜像文件
参数 2 ota 镜像文件分包大小

## ota 设备升级流程简介

### 使用iconfigtool配置相关选项
使用iconfigtool配置相关选项,并且保存到.config.in文件
或者由外部配置,make 时加入变量 make config_in=your/config/in/file

### 制作ota 镜像部署到测试服务器
假设ota_vesion 是 3 以我的测试机器举例子
http://192.168.43.52/ota/board_test/ 用于存放ota相关文件
对应于服务器本机的路径为 /var/www/html/ota/board_test (测试中采用apache2作为服务器)

根据.config.in的配置生成ota镜像,再拷贝镜像到测试服务器对应路径
```shell
make ota_img
rm -rf /var/www/html/ota/board_test/ota_v3/
cp -r ota/ /var/www/html/ota/board_test/ota_v3/
```

手动更改测试服务器ota 版本号 current_version=3
```shell
gedit ~/http/ota/board_test/ota_config.in
```

手动创建ota_v3/ota_v3.ok文件, 最终确定ota_v3 版本发布
```
> /var/www/html/ota/board_test/ota_v3/ota_v3.ok
```

### ota 设备端升级过程
假设服务器ota 版本为3
使用客户定义的方式触发ota升级,最终调用到 /etc/ota_bin/network_ota_update.sh
ota的配置在 /etc/ota_info 文件中

1 检查是否设置过下次启动项(检查文件是否存在/tmp/ota_boot_set),如未设置过,则继续
2 下载测试服务器的 ota_config.in 文件,解析当前ota版本号
3 获取/etc/ota_info 中的版本号并与服务器ota 版本号做对比,如果服务器更高,则继续
4 下载测试服务器 ota_v3/ota_v3.ok 文件,如果无法下载,说明当前v3版本还没有准备好,则退出
5 下载测试服务器 ota_v3/ota_update.in 文件
6 获取 ota_version= 字段,如果和当前版本不相等,则退出
7 获取img_type img_name img_size img_md5 字段,并且验证和保存
  ota_kernel ota_kernel_name ota_kernel_size ota_kernel_md5
  ota_rootfs ota_rootfs_name ota_rootfs_size ota_rootfs_md5
8 获取需要升级的kernel rootfs分区,检查kernel 分区大小,检查rootfs 分区大小是否满足升级包要求
9 开始升级kernel
10 启动升级脚本 /etc/ota_bin/ota_update_kernel.sh
11 调用download_ota_img 函数下载xImage 相关文件,校验md5值
12 去掉文件md5后缀,生成xImage.000N,再生成xImage.000N.ok通知升级脚本读取xImage.000N数据
13 继续下载新的xImage.000N+1.md5 文件,等待xImage.000N.done产生,并删除xImage.000N
14 继续11的步骤,直到xImage 相关文件下载完成
15 等待xImage.ok 生成,表示升级脚本成功,或者xImage.failed 表示升级脚本失败
16 按前面的步骤升级rootfs
17 如果升级成功,则设置启动分区为刚才升级的分区,产生临时文件/tmp/ota_boot_set,返回值0
18 如果什么都没有升级,也没有出错,则返回值2,中间任意步骤出错,返回1

/tmp/ota 产生ota过程中下载的文件,以及xImage.000N.ok xImage.000N.done 这种临时信号文件
/tmp/ota_wget_msg/ 产生wget下载时的log

