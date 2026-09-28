## userdata分区挂载说明

Linux工程编译出来的文件系统为 squashfs 格式，该文件系统是只读的。若有需要，可挂载一个可读可写的分区（以下统一称：userdata分区）存放用户数据。这里提供几种不同的挂载方法。

> 开始之前，请查阅《Linux工程烧录介绍.pdf》熟悉烧录流程。



> NOTE：
>
> 用户想要挂载 userdata 分区，需要用烧录工具预留出 userdata 分区所需要的空间（一般为除了 uboot、kernel、rootfs 之外剩下的空间）。
>
> 使用 nand 存储类型的开发板，本工程默认挂载的是ubi文件系统；
>
> 使用 nor 存储类型的开发板，本工程默认挂载的是 jffs2 文件系统；
>
> 使用 mmc 存储类型的开发板，本工程默认挂载的是 ext4 文件系统（与其他两种类型大同小异，这里不作详细阐述）；



#### 一、自动挂载 userdata 分区操作流程：

##### 1、使用 IConfigTool 配置自动挂载 userdata 分区选项。

###### 1.1、nand存储器类型的开发板配置流程：

<img src="img/1.png" style="zoom:67%;" />

<img src="img/2.png" style="zoom:67%;" />

<img src="img/3.png" style="zoom:67%;" />



###### 1.2、nor 存储器类型的开发板配置流程：

​	与nand大致相同，区别在于 *rootfs 相关配置* 中须选择 *jffs2文件系统相关配置* 选项

<img src="img/4.png" style="zoom:67%;" />

##### 2、重新编译文件系统 *make buildroot* ，并烧录，烧录时需要预留出 userdata 分区。

###### 2.1、nand 存储器类型的开发板烧录工具配置：

<img src="img/5.png" style="zoom:67%;" />

###### 2.2、nor 存储器类型的开发板烧录工具配置：

<img src="img/7.png" style="zoom:67%;" />

##### 3、烧录成功后，userdata分区会自动被挂载在usr/data分区，此时用户可以对该分区进行正常的读写操作。



#### 二、手动挂载 userdata 分区操作流程：

##### 本工程针对 ubifs 、 jffs2 和 ext4 三种文件系统分别提供了 mount_ubifs.sh 、 mount_jffs2.sh 和 mount_mmc_ext4.sh 三个脚本。

```shell
#以下示例是在使用 nand 存储的板子上将 userdata分区挂载在/usr/data/目录
mount_ubifs.sh userdata /usr/data/
#usage: mount_ubifs.sh partition_name/dev_path mount_path
#参数1：partition_name/dev_path 可以填写分区名或mtd设备节点。（如果填写分区名需要和烧录工具的分区名一致）
#参数2：mount_path 填写挂载在文件系统的路径
```

```shell
#以下示例是在使用 nor 存储的板子上将userdata分区挂载在/usr/data/目录
mount_jffs2.sh userdata /usr/data/
#使用方式与 mount_ubifs.sh 相同

#以下示例是在使用 mmc 存储的板子上将userdata分区挂载在/usr/data/目录
mount_mmc_ext4.sh userdata /usr/data/
#使用方式与 mount_ubifs.sh 相同
```

> 若该分区已被挂载，可先执行以下命令

```shell
#查看已被挂载的设备节点
df -h
#取消挂载
umount /usr/data

#如果之前挂载的是ubifs，还需要将userdata(mtd3)分区从ubi分离
ubidetach /dev/ubi_ctrl -m 3
```

