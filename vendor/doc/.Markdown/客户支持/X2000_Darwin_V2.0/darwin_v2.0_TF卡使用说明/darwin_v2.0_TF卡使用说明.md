# TF卡

## 一  添加TF卡相关配置

开发板默认未添加TF卡相关配置，需要使用iconfig添加

查看原理图，TF卡使用的是msc2控制器驱动

![5](darwin_v2.0_TF卡使用说明.assets/5.png)

以x2000_darwin_factory_defconfig为例. 打开IConfigTool工具,选择配置文件

![9](darwin_v2.0_TF卡使用说明.assets/9.png)



进入模块化驱动-x2000驱动列表-msc控制器驱动，wifi使用的msc1,添加msc2后，有时wifi会起不来，这里需要先配置下msc加载前的wifi引脚，避免这个问题

![8](darwin_v2.0_TF卡使用说明.assets/8.png)

进入msc2,配置TF 卡相关内容

![2](darwin_v2.0_TF卡使用说明.assets/2.png)

![3](darwin_v2.0_TF卡使用说明.assets/3.png)

![4](darwin_v2.0_TF卡使用说明.assets/4.png)

## 二 配置sd卡自动挂载

为了方便使用，当板子检测到TF卡插入后，我们可以将它自动挂载到/tmp/sdcard/目录下

![6](darwin_v2.0_TF卡使用说明.assets/6.png)

![7](darwin_v2.0_TF卡使用说明.assets/7.png)

## 三  编译并验证

Ctrl+S保存配置

![10](darwin_v2.0_TF卡使用说明.assets/10.png)

重新编译配置

```c
bhu@bhu-PC:~/work/build$ make x2000_darwin_factory_defconfig 

bhu@bhu-PC:~/work/build$ make
```

烧录后验证

```c
# cd /tmp/sdcard/
# ls
mmcblk0p1                                           //TF卡目录

# [   28.868896] mmc1: Problem switching card into high-speed mode!
[   28.868980] mmc1: new SDHC card at address 0001
[   28.869693] mmcblk0: mmc1:0001 SD 7.44 GiB 
[   28.870995]  mmcblk0: p1
sd card insert!                                    //检测到TF卡插入 
[   28.891191] FAT-fs (mmcblk0p1): Volume was not properly unmounted. Some data may be corrupt. Please run fsck
    
    
[  215.870588] mmc1: card 0001 removed
sd card remove!                                    //检测到TF卡拔出


```



## 四  TF 常见问题汇总

TF 卡正常支持文件系统有： fat32(vfat)、ntfs 、exfat

#### 问题1：挂载tf卡之后，看不到卡里的文件信息？

分析：

通fdisk查看挂载的TF卡的信息

```shell
# fdisk -l

Disk /dev/mmcblk0: 7680 MB, 8053063680 bytes, 15728640 sectors
979 cylinders, 255 heads, 63 sectors/track
Units: sectors of 1 * 512 = 512 bytes

Device       Boot StartCHS    EndCHS        StartLBA     EndLBA    Sectors  Size Id Type
/dev/mmcblk0p1    0,0,33      978,254,63          32   15728639   15728608 7679M  7 HPFS/NTFS
Partition 1 has different physical/logical end:
     phys=(978,254,63) logical=(979,15,60)
# 
```

根据打印可知类型为HPFS/NTFS，我们的文件系统需要支持HPFS/NTFS
通过命令查看我们系统已经支持的文件系统

```shell
# cat /proc/filesystems 

nodev   sysfs
nodev   tmpfs
nodev   bdev
nodev   proc
nodev   devtmpfs
nodev   configfs
nodev   debugfs
nodev   sockfs
nodev   pipefs
nodev   ramfs
nodev   devpts
        squashfs
        vfat     <-------- （fat32）
        exfat    <-------- 
        ntfs　　　<--------
nodev   ubifs
nodev   functionfs
# 
```

如果没有出现ntfs，需要勾选kernel配置中ntfs相关配置，需要支持其它格式，仿照下面配置即可。

```shell
kernel/kernel$ make menuconfig 
配置目录
Location:
  │     -> File systems
  │       -> DOS/FAT/EXFAT/NT Filesystems


  <*> VFAT (Windows-95) fs support          ------FAT32(VFAT)
  (437) Default codepage for FAT 
  (iso8859-1) Default iocharset for FAT 
  [*]   Enable FAT UTF-8 option by default  
  <*> exFAT filesystem support              -----exFAT
  (utf8) Default iocharset for exFAT   
  <*> NTFS file system support              -----NTFS
  [ ]   NTFS debugging support
  [*]   NTFS write support 
```

勾选完编译烧录之后打印会出现以下信息
　　ntfs: volume version 3.1.

有些卡显示类型(Type)会出错，以windows系统查看的卡属性为准。

#### 问题2：挂载TF卡成功，中文目录或者中文文件名称显示乱码？

解决方法：

ntfs类型支持中文类型根据打印需求在挂载卡的脚本中添加相关选项

例如： 添加支持 utf-8 字符，修改文件：sdcard_inserting.sh
buildroot/buildroot_patch/rootfs_config/file/sdcard/sdcard_inserting.sh

sdcard_inserting.sh 脚本内容修改如下：

![选区_051](darwin_v2.0_TF卡使用说明.assets/11.png)

