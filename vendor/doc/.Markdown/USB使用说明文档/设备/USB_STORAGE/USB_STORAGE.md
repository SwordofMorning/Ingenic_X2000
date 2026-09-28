# USB_STORAGE

# 控制器作为usb device

## 1.相关配置

### 1.1.IConfigTool配置

> 使用控制器作为usb device，要先把adb服务关闭

1.打开IConfigTool，逐级进入到 **[adb服务脚本]** 界面，取消勾选adb服务自启动

<img src="img/1.png">

2.如果需要**开启开机自动挂载**功能，逐级进入到  **[usb_storage相关设置]** 界面，并且进行如下配置：

> 如果不需要该功能，取消下列勾选或者跳过本处配置即可

<img src="img/5.png">

3.配置完IConfigTool配置需要重新应用配置

```shell
cd ~/your_linux_project
#以x2000_mmc0_defconfig为例，用户应根据当前使用的配置为准
make x2000_mmc0_defconfig
```

### 1.2.Buildroot配置

1.配置buildroot选项，编译mkfs.fat工具

```C
Target packages --->
	Filesystem and flash utilities --->
	[*]	dosfstools
	[*]   mkfs.fat
```

### 1.3.Kernel配置

1.配置 **USB Gadget Drivers** 选项

```c
Location:
	Device Drivers	--->
    	[*]	USB support	--->
      	<*>	USB Gadget Support	--->
      	<*>   USB Gadget Drivers	--->
        		USB functions configurable through configfs
```

2.配置 **Loopback device support** 选项

```c
Location:
	Device Drivers	--->
	[*]	Block devices
	<*>	Loopback device support
        (16)  Number of loop devices to pre-create at init time
```

### 1.4.重新编译

1.完成buildroot和kernel配置后，需要进行重新编译

```shell
cd ~/your_linux_project
make buildroot && make kernel
```



## 2.挂载测试

### 2.1.自动挂载

如果在**第1.1章**中开启了自动挂载功能，开机启动后会有以下提示信息：

<img src="img/6.png">

自动挂载成功后，可以在 PC 端将设备当作 usb设备 进行使用

<img src="img/7.png">



### 2.2.手动挂载

在本地对镜像文件进行添加测试文件，使用开发板作为usb device进行挂载，挂载后在PC端查看是否存在该测试文件

```shell
1.storage_config.sh stop

2.cd /usr/data

# 手动创建一个10M大小的镜像文件
3.dd if=/dev/zero of=fat32.img bs=1k count=10240

4.mkfs.fat fat32.img

# 若loop0已经被其他设备使用，可以使用loop1、loop2等其他空闲的loop设备节点
5.losetup /dev/loop0 fat32.img

# 创建本地挂载点
6.mkdir mntdir

7.mount -t vfat fat32.img mntdir/

#检查是否成功挂载
8.df mntdir
/dev/loop1                2028         2      2026   0% /usr/data/mntdir

9.echo "write something for test..." > mntdir/test.txt

#在作为usb device进行挂载测试前，先取消本地挂载
10.umount mntdir

#执行挂载脚本后，会弹出刚刚制作的镜像，刚刚手动添加的测试文件存在，并且内容正确，则说明测试成功
11.storage_config.sh start /usr/data/fat32.img
```

<img src="img/2.png">

在PC端的usb界面中，添加另外一个测试文件，再从本地查看镜像是否正确新增了该测试文件

<img src="img/3.png">

```shell
# 再次进行本地挂载
12.mount -t vfat fat32.img mntdir/

# 查看刚刚新增的测试文件
13.cat mntdir/test2.txt
```

> 测试文件存在并且内容正确，则说明挂载测试成功。

<img src="img/4.png">

```shell
# 卸载设备
14.storage_config.sh stop
```