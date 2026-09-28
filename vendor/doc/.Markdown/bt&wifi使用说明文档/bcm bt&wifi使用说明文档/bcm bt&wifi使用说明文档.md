

# bcm bt&wifi使用说明文档



## 1 配置bt&wifi

### 1.1 配置bt&wifi 驱动

1.首先进入工程/编译配置

<img src="img/1.png" style="zoom:100%;" />

​      2.如果要使用wpa 相关应用工具，那么buildroot 的配置文件就选择 configs/buildroot/buildroot_wifi_common_defconfig，x2000芯片wifi相关配置为buildroot_x2000_wifi_common_defconfig。

<img src="img/2.png" style="zoom:100%;" />

3.回到主界面，我们进入模块化驱动配置，对bt&wifi设备驱动进行配置。

<img src="img/3.png" style="zoom:100%;" />

4.进入外设配置

<img src="img/4.png" style="zoom:100%;" />

5.点击进入配置bt,wifi设备驱动。

<img src="img/5.png" style="zoom:100%;" />

6.勾选cypress驱动进行配置

<img src="img/6.png" style="zoom:100%;" />

7.配置引脚，固件等参数。

​          wifi固件路径存放在工程目录下的wireless/bcm/firmware/wifi_bcm/，编译时会复制到文件系统的/lib/firmware/wifi_bcm/目录，驱动运行时会将固件加载到wifi芯片，下面填wifi固件路径是在文件系统中的路径，目录路径是固定的，只准修改固件文件名。

​         默认提供的是bcm43438的,如果想支持其他型号，可勾选支持，并将相应的固件放到wireless/bcm/firmware/wifi_bcm/目录下， 再修改下图的固件名称。

<img src="img/7.png" style="zoom:100%;" />

8.最后回到主界面，我们点击进入wireless(无线设备)配置界面，进行蓝牙的相关配置。 

<img src="img/8.png" style="zoom:100%;" />

9.下面我们以配置博通的蓝牙为例。

<img src="img/9.png" style="zoom:100%;" />

10.    1.开机自启动蓝牙，并下载蓝牙固件，当然前提是bcm的驱动先正常运行
       
        bt 固件路径存放在工程目录下的wireless/bcm/firmware/bt_bcm/，编译时会复制到文件系统的/lib/firmware/bt_bcm/目录，
       
       如果选择开机自启，系统启动时会自动将固件加载到蓝牙。
       
         2. 设置用于蓝牙通信的串口路径，x2000系列usart dev path路径选择为/dev/ttyS3
         3. 选择相应平台的bsa工具包，这里是以x1021为例，选择Xburst1的bsa工具包;x2000选择Xburst2的bsa工具包。

<img src="img/10.png" style="zoom:100%;" />

x2000对应选择xburst2

<img src="img/11.png" style="zoom:100%;" />

### 1.2 配置wifi通信MMC端口

**Xburst1系列在kernel 的menuconfig里面配置**

make menuconfig

 Prompt: JZMMC_V12 MMC1	// 这里以配置mmc1为例
  │   Location:

  │     -> Device Drivers 

  │       -> MMC/SD/SDIO card support (MMC [=y])

  │         -> Ingenic(XBurst)  MMC/SD Card Controller(MSC) v1.2 support (JZMMC_V12 [=y])  



**Xburst2 系列 的mmc驱动因为已经移植到module driver工程里去了，所以要在IConfigTool里面配置。**

<img src="img/30.png" style="zoom:100%;" />

<img src="img/31.png" style="zoom:100%;" />

<img src="img/32.png" style="zoom:100%;" />

<img src="img/33.png" style="zoom:100%;" />

<img src="img/34.png" style="zoom:100%;" />

### 1.3 配置蓝牙通信串口

**Xburst1系列**

需要在kernel 的menuconfig 中将uart驱动打开并使能。

**Xburst2系列**

除了要在kernel 的menuconfig中将uart驱动打开，还需要在kernel-x2000的x2000_module_base.dts文件中添加串口配置，如下图我将添加uart3。

详情可参考X2000片上外设内核驱动的UART章节。

<img src="img/29.png" style="zoom:100%;" />

## 2 测试 bt&wifi

### 2.1 测试wifi

首选要保证驱动的安装,(配置工具勾选后，开机会自动安装bt,wifi驱动)。下面以BCM的wifi为例，模块名为cywdhd

用lsmod命令查看已安装的驱动，为了确保安装过程中没有错误还可以用dmesg命令看一下调试信息。没有报错说明驱动正常安装。

<img src="img/16.png" style="zoom:100%;" />

1.设置wifi名称和密码

只读文件系统squashfs的usr/data目录是可写的，在这个目录下创建wifi配置文件如下。

注意事项：配置文件'='号前后不要添加空格，否则将会报错。

```
# cd /usr/data
# touch wpa_supplicant.conf
# cat wpa_supplicant.conf
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1
	network={
	ssid="Guest"
	psk="ingenic_guest"
	}
```

<img src="img/36.png" style="zoom:100%;" align='left'>

>ap_scan=1 模式第一步将会试图扫描周围存在的网络。只有在无可匹配的网络情况下才会创建一个新的IBSS或者AP模式的网络。
>
>ssid="Guest" 连接wifi的名称
>
>psk="ingenic_guest" 连接wifi的密码
>

2.下面进行使用测试

` wpa_supplicant -Dnl80211 -iwlan0 -c/usr/data/wpa_supplicant.conf &` 

<img src="img/13.png" style="zoom:100%;" />

> wpa_supplicant WPA的应用层认证客户端，负责完成认证相关的登录、加密等工作。
>
> -D 驱动类型名称
>
> -i 接口名称
>
> -c 配置文件

3.动态分配IP

`udhcpc -i wlan0`

<img src="img/14.png" style="zoom:100%;" />

> udhcpc  动态主机配置协议
>
> -i：使用的网络名称

4.ping 网络测试

`ping www.baidu.com`

<img src="img/15.png" style="zoom:100%;" />

### 2.2 测试蓝牙

1.首先确保bt,wifi驱动正常安装( IConfigTool配置工具勾选后，开机会自动安装bt,wifi驱动)。

输入lsmod 查看驱动是否安装成功。模块名为cywdhd。

为了确保安装过程中没有错误还可以用dmesg命令看一下调试信息。没有报错说明驱动正常安装

<img src="img/16.png" style="zoom:100%;">

2.如果前面配置蓝牙的时候没有勾选 “**开机自启蓝牙**”，这里就要手动启动蓝牙，命令如下。

没有勾选，则在/etc/init.d目录下，将没有如下图框住的配置文件

<img src="img/35.png" align='left'>

手动启动脚本命令

<img src="img/17.png" style="zoom:100%;">

运行完后会在/run/blue_bsa/目录下产生相关文件，**执行蓝牙的应用时也必须要进入到这个目录**。

<img src="img/18.png" style="zoom:100%;">

3.执行商家提供的测试命令，这里以x1021为例，这些命令存放在工程的wireless/bcm/bin/xburst1_bsa/app/目录下(例如x2000就是xburst2_bsa/app/)

通过adb将测试命令传到文件系统的/run/blue_bsa目录下，下面我用app_manager进行测试，扫描我的手机蓝牙设备。

<img src="img/19.png" style="zoom:100%;">

<img src="img/20.png" style="zoom:100%;">

<img src="img/21.png" style="zoom:100%;">

<img src="img/22.png" style="zoom:100%;">



## Q1  运行 app_manager 失败

调用app_manager调试蓝牙设备的时候出现报错（如下图），有可能是因为内存不足引起的。

<img src="img/37.png" style="zoom:100%;">

从下面命令的运行结果可以看出当前系统剩余内存为1.252M，蓝牙应用占用的内存大小为1.5M，因此蓝牙应用很可能因为内存不足而运行失败。

```shell
#使用free命令查看当前内存使用情况
# free -k
              total        used        free      shared  buff/cache   available
Mem:          19292        9364        1252           0        8676           0
-/+ buffers/cache:         9364        9928
Swap:             0           0           0

#ls -lh /run/blue_bsa/app_manager
-rwxrwxrwx    1 root     root        1.5M Nov 20  2020 /run/blue_bsa/app_manager
```

解决方法：对编译生成的app_manager使用mips-linux-gnu-strip工具进行压缩应用的大小。

```shell
#在module工程目录下执行以下命令
#把交叉编译工具添加到环境变量中
export PATH=$PWD/tools/toolchains/mips-gcc520-glibc222/bin:$PATH

#使用strip工具压缩蓝牙应用
mips-linux-gnu-strip wireless/bcm/bin/xburst1_bsa/app/app_manager

#通过adb工具将压缩后的蓝牙应用下载到开发板
adb push wireless/bcm/bin/xburst1_bsa/app/app_manager /run/blue_bsa/

#在开发板串口终端下执行以下命令
#重新执行蓝牙启动脚本
bt_enable_bsa.sh

#进入指定路径,执行蓝牙应用
cd /run/blue_bsa/
./app_manager
```

蓝牙应用正常调用之后，会出现功能菜单，如下图。

<img src="img/38.png" style="zoom:100%;">