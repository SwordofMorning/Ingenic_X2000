# 				x2600linux_nand启动挂载emmc的方法

## 一 硬件环境

本文以x2600e halley7开发板为硬件平台，来展示系统从nand启动后的挂载访问emmc方法。

本开发板需要5V DC供电。两个typeC  usb口分别负责烧录和串口打印。

串口log输出：uart0_pe09, 波特率：3000000. 

![1](x2600linux_nand启动挂载emmc的方法.assets/1.png)

## 二 代码实现

在nand启动的系统中挂载emmc有两种方式。一种是在kernel中加载mmc驱动的方式，另一种是iconfigTool工具配置的模块驱动加载方式。本文详述这两种方式。

### 2.1 模块驱动加载方式

![1](x2600linux_nand启动挂载emmc的方法.assets/1_1.png)

根据硬件设计，本开发板的emmc供电为1.8v，因此要修改kernel的dts文件来设置默认msc的供电电压。此处一定要根据硬件设计对应来设置1.8v或者3.3v。

![2](x2600linux_nand启动挂载emmc的方法.assets/2.png)

可知，当前加载的kernel路径为：../kernel/kernel

 kernel默认配置为：x2600_module_base_linux_sfc_nand_defconfig

查找当前kernel使用的dts文件：

![3](x2600linux_nand启动挂载emmc的方法.assets/3.png)

可知，当前kernel加载的dts文件为：kernel/kernel/module_drivers/dts/x2600_module_base.dts

![4](x2600linux_nand启动挂载emmc的方法.assets/4.png)

编译：

```
sxyzhang@T430:~/my/work/linux/x2670_sz/build$ make x2600e_halley7_v1.0_nand_5.10_factory_defconfig
sxyzhang@T430:~/my/work/linux/x2670_sz/build$ make
```

烧录：

<img src="x2600linux_nand启动挂载emmc的方法.assets/5.png" alt="5" style="zoom:150%;" />

### 2.2 kernel默认加载方式

![6](x2600linux_nand启动挂载emmc的方法.assets/6.png)

在kernel中加载，所以此处要修改kernel配置。首先设置当前kernel使用的编译工具链环境变量。

![7](x2600linux_nand启动挂载emmc的方法.assets/7.png)

可知，当前kernel编译工具链为：../tools/toolchains/mips-gcc930-glibc228/mips-linux-gnu-xx， 因此设置环境变量：

![8](x2600linux_nand启动挂载emmc的方法.assets/8.png)

kernel配置中选中msc驱动：

![9](x2600linux_nand启动挂载emmc的方法.assets/9.png)

修改完kernel配置之后一定要拷贝到默认配置中，否则再次整体配置make x2600e_halley7_v1.0_nand_5.10_factory_defconfig之后会覆盖上述kernel配置。 拷贝命令如下：

```
cp .config arch/mips/configs/x2600_module_base_linux_sfc_nand_defconfig
```

修改对应的dts添加设备树节点，并设置对应默认电压：

<img src="x2600linux_nand启动挂载emmc的方法.assets/10.png" alt="10" style="zoom:150%;" />

编译：

```
sxyzhang@T430:~/my/work/linux/x2670_sz/build$ make x2600e_halley7_v1.0_nand_5.10_factory_defconfig
sxyzhang@T430:~/my/work/linux/x2670_sz/build$ make
```

烧录：

<img src="x2600linux_nand启动挂载emmc的方法.assets/5.png" alt="5" style="zoom:150%;" />

## 三 测试过程

上述两种方式配置、修改、编译、烧录之后，启动log如下说明加载mmc驱动成功了：

![11](x2600linux_nand启动挂载emmc的方法.assets/11.png)

第一次使用需要格式化mmc分区，不然会找不到分区：

```
mkdosfs /dev/mmcblk0
```

挂载分区：

```c
# mkdir -p /usr/data/test                      // 新建挂载点
# mount /dev/mmcblk0 /usr/data/test/           // 将mmc分区挂载到挂载点
# 
# mount                                        // 查看挂载情况，最后一个分区就是挂载的mmc分区
/dev/root on / type squashfs (ro,relatime)
devtmpfs on /dev type devtmpfs (rw,relatime,size=116340k,nr_inodes=29085,mode=755)
tmpfs on /dev type tmpfs (rw,relatime)
proc on /proc type proc (rw,relatime)
devpts on /dev/pts type devpts (rw,relatime,gid=5,mode=620,ptmxmode=666)
tmpfs on /dev/shm type tmpfs (rw,relatime,mode=777)
tmpfs on /tmp type tmpfs (rw,relatime)
tmpfs on /run type tmpfs (rw,nosuid,nodev,relatime,mode=755)
sysfs on /sys type sysfs (rw,relatime)
/dev/ubi1_0 on /usr/data type ubifs (rw,sync,relatime,assert=read-only,ubi=1,vol=0)
none on /sys/kernel/config type configfs (rw,relatime)
adb on /dev/usb-ffs/adb type functionfs (rw,relatime)
/dev/mmcblk0 on /usr/data/test type vfat (rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=iso8859-1,shortname=mixed,erro)
# 
# 
# 
# df -h                                     // 查看每个分区的具体情况
Filesystem                Size      Used Available Use% Mounted on
/dev/root                10.1M     10.1M         0 100% /
devtmpfs                113.8M         0    113.8M   0% /dev
tmpfs                   113.8M         0    113.8M   0% /dev
tmpfs                   113.8M         0    113.8M   0% /dev/shm
tmpfs                   113.8M      4.0K    113.8M   0% /tmp
tmpfs                   113.8M     12.0K    113.8M   0% /run
/dev/ubi1_0             128.5M    140.0K    123.6M   0% /usr/data
/dev/mmcblk0              6.9G      4.0K      6.9G   0% /usr/data/test
```

## 四 容易出错的地方

上述两种mmc的加载方式中反复用到修改kernel的默认配置文件及dts，以及将修改后的kernel配置保存到默认配置文件中，所以要仔细阅读本文中的kernel编译目录和dts查找的方法，不要找错kernel目录或者找错kernel的配置文件，更不能找错kernel的dts文件。这些在上文都有详细的描述检索方法，请仔细阅读。 