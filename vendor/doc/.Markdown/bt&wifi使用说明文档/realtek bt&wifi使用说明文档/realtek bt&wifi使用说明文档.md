# realtek bt&wifi使用说明文档



## 1 配置bt&wifi

### 1.1 配置bt&wifi 驱动

1.首先进入工程/编译配置

<img src="img/1.png" style="zoom:100%;" />

​      2.buildroot配置文件选择 configs/buildroot/buildroot_wifi_common_defconfig(因为要用到wifi相关应用命令)，x2000系列配置文件选择 configs/buildroot/buildroot_x2000_wifi_common_defconfig。

<img src="img/2.png" style="zoom:100%;" />

3.回到主界面，我们进入模块化驱动配置，对bt&wifi设备驱动进行配置。

<img src="img/3.png" style="zoom:100%;" />

4.进入外设配置

<img src="img/4.png" style="zoom:100%;" />

5.点击进入配置bt,wifi设备驱动。

<img src="img/5.png" style="zoom:100%;" />

6.勾选realtek驱动进行配置

<img src="img/6.png" style="zoom:100%;" />

7.配置引脚参数。

<img src="img/7.png" style="zoom:100%;" />

8.最后回到主界面，我们点击进入wireless(无线设备)配置界面，进行蓝牙的相关配置。 

<img src="img/8.png" style="zoom:100%;" />

9.下面我们以配置博通的蓝牙为例。

<img src="img/9.png" style="zoom:100%;" />

10.    1.开机自启动蓝牙，并下载蓝牙固件，当然前提是bt,wifi的驱动先正常运行(蓝牙固件放在/lib/firmware/bt_bcm/中,系统启动时会自动将固件加载到蓝牙)。
         2. 设置用于蓝牙通信的串口路径
         3. 选择相应平台的bsa工具包，这里是以x1021为例，所以选择Xburst1的bsa工具包。

<img src="img/10.png" style="zoom:100%;" />

### 1.2 配置wifi通信MMC端口

**Xburst1系列在kernel 的时menuconfig里面配置**

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

进入IConfigTool，进入如下路径，勾选如下驱动。

<img src="img/44.png" style="zoom:100%;" />

## 2 测试 bt&wifi

### 2.1 测试wifi

首选要保证驱动的安装,(配置工具勾选后，开机会自动安装bt,wifi驱动)。模块名称为8723ds

用lsmod命令查看已安装的驱动，为了确保安装过程中没有错误还可以用dmesg命令看一下调试信息。没有报错说明驱动正常安装。

<img src="img/23.png" style="zoom:100%;" />

1.设置wifi名称和密码

/usr/data挂载了可读写ubi文件系统，在这个目录下创建wifi配置文件如下。

注意事项：配置文件'='号前后不要添加空格，否则将会报错。

```
# cd /usr/data
# touch wpa_supplicant.conf
# cat wap_supplicant.conf
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1
	network={
	ssid="Guest"
	psk="ingenic_guest"
	}
```

<img src="img/12.png" style="zoom:100%;" />

> ap_scan=1 模式第一步将会试图扫描周围存在的网络。只有在无可匹配的网络情况下才会创建一个新的IBSS或者AP模式的网络。
>
> ssid="Guest" 连接wifi的名称
>
> psk="ingenic_guest" 连接wifi的密码
>

2.下面进行使用测试

` wpa_supplicant -Dnl80211 -iwlan0 -c/usr/data/wpa_supplicant.conf &` 

注意事项：假设wpa_supplicant命令不存在，请查阅文档末尾文件系统相关工具配置

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

输入lsmod 查看驱动是否安装成功。模块名为8723ds。

为了确保安装过程中没有错误还可以用dmesg命令看一下调试信息。没有报错说明驱动正常安装

<img src="img/23.png" style="zoom:100%;" align='left'>

2.如果前面配置蓝牙的时候没有勾选 “**开机自启蓝牙**”，这里就要手动启动蓝牙，命令如下。

没有勾选，则在/etc/init.d目录下，将没有如下图框住的配置文件

<img src="img/43.png" align='left'>

在启动蓝牙之前，先设置蓝牙的mac 地址，在tmp/目录下创建文件名为btmac.txt的文件将mac地址写在里面。如果不设置，将会分配随机mac地址。

<img src="img/24.png" style="zoom:100%;">

然后手动启动蓝牙，运行bt_enable_rtk.sh启动。

<img src="img/25.png" style="zoom:100%;">

查看蓝牙设备节点

`#hciconfig -a`

注意事项：如果hciconfig、hcitool命令不存在，也参考文档末尾文件系统相关命令配置。

<img src="img/26.png" style="zoom:100%;">开启蓝牙设备节点

`#hciconfig hci0 up `

<img src="img/27.png" style="zoom:100%;">

使用hcitool来测试蓝牙的功能。

`#hcitool -i hci0 scan`

<img src="img/28.png" style="zoom:100%;">



# 文件系统相关工具配置
## wap_supplicant相关命令配置

wpa_supplicant命令不存在的问题

1.1进入如下目录，执行如下命令，进行文件系统命令配置

<img src="img/35.png" align='left'>

1.2进入包的配置

  选中 wpa_supplicant选项

```
-> Target packages   
	-> Networking applications   
		-> wpa_supplicant 
```

<img src="img/36.png" align='center'>

保存退出,重新编译文件系统

<img src="img/37.png" align='left'>

查询以下目录是否存在spa_supplicant

<img src="img/38.png" align='left'>



##  hci相关命令配置

1.文件系统缺少相关的蓝牙组件hciconfig、hcitool

进入文件系统配置目录/x2000/buildroot/buildroot/，使用命令make menuconfig进入对应的配置界面，勾选如下工具。保存退出，重新编译文件系统。

```
-> Target packages
	-> Networking applications
		-> bluez-utils 5.x
```

<img src="img/39.png" align='center'>

2.经过步骤一，文件系统还是没有出现相关的文件组件，请参考以下做法：

在如下目录运行对应的命令，出现的该问题的原因是工具已经编译好，Makefile认为该文件没有进行更新，所以没有进行将工具打包到文件系统镜像的操作。

<img src="img/41.png" align='left'>

<img src="img/42.png" align='left'>