# recovery方式进行ota升级的补充说明

本文档为采用recovery方式进行ota升级的补充说明，recovery方式升级的基本原理（分区的升级方法、ota分区策略等）与采用A/B分区方式进行ota升级基本相同，因此，阅读本文档前请先阅读《ota升级逻辑.md》与《ota设备端升级介绍.md》。

## recovery方式进行ota升级的基本介绍

对于通过recovery方式进行系统升级，也是存在两个系统：

一个主系统（main_os）和一个recovery系统，主系统可以探测服务器上是否有更高的系统版本，并从服务器中获取升级包的基本信息用来完成升级系统在本设备的可用性的基本检验，此外主系统还可以对recovery系统进行下载升级；

recovery系统相对主系统可以进行裁剪以获得更小的体积。recovery系统的功能就是完成对主系统的下载升级。

值得注意的是：与传统recovery方式不同的是，君正的recovery升级方案不是在主系统中下载主系统的升级镜像，而是在recovery系统中下载升级镜像，因此在执行升级前，需要保障网络是可用的，且最好与主系统的网络环境保持一致，推荐在主系统与recovery系统共同挂载的userdata分区存放网络配置，在进入recovery系统后通过该配置完成网络连接。如在本方案的测试中，采用的是wifi进行网络连接，共享的配置文件为：`/usr/data/wpa_supplicant.conf`。



## ota 设备升级流程简介

### 使用iconfigtool配置相关选项

使用iconfigtool配置相关选项，并且保存到.config.in文件中
或者由外部配置,make 时加入变量 make config_in=your/config/in/file

（建议对 主系统 以及 recovery系统 各自进行配置）

### 制作 recovery 与 主系统 镜像部署到测试服务器

假设ota_vesion 是 4 以我的测试机器举例子
http://10.4.229.4/ota/board_x2000/ 用于存放ota相关文件
对应于服务器本机的路径为 /var/www/html/ota/board_x2000 (测试中采用apache2作为服务器)

#### 对于 主系统

根据.config.in的配置生成 主系统 镜像,再拷贝镜像到测试服务器对应路径

```shell
make ota_img
rm -rf /var/www/html/ota/board_x2000/main_os/ota_v4/
cp -r ota/ /var/www/html/ota/board_x2000/main_os/ota_v4/
```

手动添加测试服务器 主系统 ota的版本号 current_version=4

```shell
gedit /var/www/html/ota/board_x2000/ota_config.in
```

手动创建ota_v4/ota_v4.ok文件, 最终确定ota_v4 版本发布

```
/var/www/html/ota/board_x2000/main_os/ota_v4/ota_v4.ok
```

#### 对于 recovery系统

根据.config.in的配置生成 recovery 镜像,再拷贝镜像到测试服务器对应路径

```shell
make ota_img
rm -rf /var/www/html/ota/board_x2000/recovery/ota_v4/
cp -r ota/ /var/www/html/ota/board_x2000/recovery/ota_v4/
```

手动更改测试服务器 recovery系统 ota 版本号 recovery_version=4（另起一行）

```shell
gedit /var/www/html/ota/board_x2000/ota_config.in
```

手动创建ota_v4/ota_v4.ok文件, 最终确定ota_v4 版本发布

```
/var/www/html/ota/board_x2000/recovery/ota_v4/ota_v4.ok
```

### ota 设备端升级过程

设备端的升级过程用到两个脚本，一是主系统中执行的network_main_os_update_recovery.sh，二是recovery系统中执行的network_recovery_update_main_os.sh。

假设服务器中主系统版本为4，recovery系统版本为4，userdata分区挂载在/usr/data/下

#### network_main_os_update_recovery.sh 的执行流程

在主系统中,根据客户的更新策略,最终调用network_main_os_update_recovery.sh,脚本的基本流程为：

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
13 随后进行 recovery 系统的升级,升级流程与《ota设备端升级介绍》中介绍的流程一致,本处省略。

脚本执行后，返回值为0则表示已经准备好进入recovery系统对主系统进行升级，且已经设置下次启动分区为recovery系统，重启即可进入recovery系统

#### network_recovery_update_main_os.sh 的执行过程

在recovery系统中,根据用户的方案进行网络配置,并最终调用network_recovery_update_main_os.sh,脚本的基本流程与《ota设备端升级介绍》中介绍的流程基本一致，其中main_os升级信息ota_update.in与升级包存放在测试服务器中的 main_os/ota_v4/ 路径下。

成功完成升级后，脚本返回值为0，重启即可返回主系统，如不成功，请依据错误信息自行设计升级失败的处理策略。
