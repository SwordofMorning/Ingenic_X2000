# Linux工程烧录介绍



## 1 最新烧录工具获取

**ubuntu版本：**

```c
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz 
```

**windows版本：**

```c
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windows.zip 
```



## 2 烧录工具使用说明

###  2.1 打开烧录软件 

<img src="img/1.png" />

### 2.2 配置对应的板级信息

**这里选择自己对应的板级配置，以x2000（nand存储器）为例子**

<img src="img/2.png"  />

### 2.3 查看并配置分区信息

**设置烧录分区的偏移地址(offset)和文件大小(size)以及分区名称(partition name)**

![](img/3.png)

### 2.4 选择要烧录的镜像文件

选中要烧录的镜像文件（uboot、kernel、roofs、data）

​		uboot镜像路径在：build/output/u-boot-with-spl.bin

​		kernel镜像路径在：build/ouput/xImage

​		roofs镜像路径在：build/ouput/rootfs.squashfs

在ops选择存储器类型，板级是nor flash 就选SFC_NOR，nand falsh 就选SFC_NAND，选择与自己存储器相对应的类型

最后在setting选项栏选择将要烧录的镜像文件路径,保存配置



![](img/4.png)



### 2.5 烧录

**首次烧录或全部文件烧录时建议将全部擦除勾选中**

![](img/5.png)

烧录方法：

点击软件开始, 按住开发板BOOT键不放, 开发板重新上电或者按reset 键复位开发板

如下界面表示烧录完成 （可能在擦除的时候会比较久）

![](img/6.png)

![](img/7.png)

## 3 userdata分区文件系统配置以及烧录说明

### 3.1 自动挂载分区说明

工程编译出来的文件系统为squashfs格式，该文件系统是只读的. 若有需要，可挂载一个ubi文件系统的分区存放用户数据

以x2000_darwin_factory_defconfig为例，其配置流程如下：

打开IConfigTool工具，选择配置文件

![](img/21.png)



![](img/8.png)

![](img/9.png)

![](img/10.png)

![](img/11.png)

保存配置文件并重新编译

![](img/22.png)

```shell
make x2000_darwin_factory_defconfig
make
```

烧录成功后，userdata分区会自动被挂载在usr/data分区，该分区是ubi格式的文件系统。可制作ubifs镜像烧录到该分区，详见3.2章。也可制作ubi镜像文件加载到该分区，详见3.3章



### 3.2 ubifs镜像文件的制作与烧录

可使用以下命令制作ubfis镜像

```shell
mkfs.ubifs -r ./rootfs -o rootfs.ubifs -m 2048 -e 126976 -c 560

 #-r：制定文件内容的位置
 #-o：输出的文件
 #-m：最小输入输出大小。这里为2KiB(2048bytes)，一般为页大小
 #-e：逻辑可擦除块大小。这里为124KiB=(每块的页数-2)*页大小=（64-2）*2KiB=124KiB=126976bytes
 #-c：最多逻辑可擦除块的数目，实际上是设置此卷的最大容量。计算公式为：容量/页大小*每块的页数

 # 块大小与页大小请参考flash的数据手册
```

ubfis镜像的烧录。其工具使用步骤详见第二章。在烧录前选中要烧录的分区以及文件如图：

![](img/12.png)

烧录成功后，该文件系统会自动挂载到/usr/data/目录下

### 3.3 ubi镜像文件的制作与挂载

可使用以下命令制作ubi镜像

```shell
ubinize -o rootfs.ubi -m 2048 -p 128KiB  ubinize.cfg
#-o：输出的文件‘
#-m：页面的大小
#-p：物理可擦出块大小为128KiB=每块的页数*页大小=64*2KiB=128KiB
#-s：用于UBI头部信息的最小输入输出单元，一般与最小输入输出单元(-m参数)大小一样
```

ubinize.cfg是ubiniz工具制作ubi镜像需要指定的配置文件，其文件内容如下：

```shell
[ubifs]
mode=ubi
image=rootfs.ubifs                    # mkfs.ubi生成的源镜像
vol_id=0                                         #卷序号
vol_size=70MiB                           #卷大小
vol_type=dynamic                    #动态卷
vol_alignment=1
vol_name=rootfs                       #卷名
vol_flags=autoresize
```

将rootfs.ubi文件系统复制到"buildroot/output/target/"的任意位置下(我这里直接放到了根目录下)，重新编译buildroot并重新烧录

```shell
make buildroot
烧录
```

烧录成功后可在板子终端看到该ubi镜像文件：

![](img/13.png)

运行以下命令

```shell
#清除userdata分区
flash_erase /dev/mtd3 0 0
#将镜像文件写入userdata分区
ubiformat /dev/mtd3 -y -f /rootfs.ubi
#挂载分区
mount_ubifs.sh userdata /usr/data/

#我测试板子的userdata分区对应的节点为：/dev/mtd3
#"cat /proc/mtd"可以查看分区名对应的mtd设备序号
```

**若该分区已被挂载，可先执行以下命令**

```shell
#查看已被挂载的设备节点
df -h
#取消挂载
umount /dev/ubi1_0
#将userdata(mtd3)分区从ubi分离
ubidetach /dev/ubi_ctrl -m 3
```

## 4 串口工具的使用与说明

**以ubuntu环境下minicom串口工具为例:**

**君正平台默认使用波特率为3000000bps,安装命令：sudo apt-get install minicom**

### 4.1 安装串口驱动（串口芯片ch343）

进入doc/开发使用说明/CH343串口驱动安装/CH343_Driver目录, 执行make指令

```c
bhu@bhu-PC:~/work/doc/开发使用说明/CH343串口驱动安装/CH343_Driver$ make
make -C /lib/modules/6.1.12/build  M=/home/bhu/work/doc/开发使用说明/CH343串口驱动安装/CH343_Driver  
make[1]: 进入目录“/usr/src/linux-headers-6.1.12”
warning: the compiler differs from the one used to build the kernel
  The kernel was built by: gcc (Uos 8.3.0.5-1+dde) 8.3.0
  You are using:           gcc (Uos 8.3.0.3-3+rebuild) 8.3.0
  CC [M]  /home/bhu/work/doc/开发使用说明/CH343串口驱动安装/CH343_Driver/ch343.o
  MODPOST /home/bhu/work/doc/开发使用说明/CH343串口驱动安装/CH343_Driver/Module.symvers
  CC [M]  /home/bhu/work/doc/开发使用说明/CH343串口驱动安装/CH343_Driver/ch343.mod.o
  LD [M]  /home/bhu/work/doc/开发使用说明/CH343串口驱动安装/CH343_Driver/ch343.ko
make[1]: 离开目录“/usr/src/linux-headers-6.1.12”
```

sudo make load 或者 sudo insmod ch343.ko 可以动态加载驱动程序

sudo make install 可以让驱动程序永久工作

完成后lsmod可以查看驱动是否加载成功

![](img/19.png)

将开发板接入主机，查看dev目录下的tty口，可以看到一个叫ttyCH343USBx的接口

![](img/20.png)

### 4.2 打开minicom串口设置

> 命令： sudo minicom -s

![](img/14.png)

### 4.3 修改串口信息

按a键进行设置使用的串口设备节点 例如：Serial Device：/dev/ttyCH343USB0

按e键进行设置串口波特率（A ：增加波特率 B：减小波特率）例如Bps/Par/Bits：3000000 8N1

按f键进行硬件流控的关闭和打开 例如：Hardware Flow Control：NO

按g键进行软件流控的关闭和打开 例如：Software Flow Control：NO

![](img/17.png)

回车退出，串口配置完成。

![](img/18.png)