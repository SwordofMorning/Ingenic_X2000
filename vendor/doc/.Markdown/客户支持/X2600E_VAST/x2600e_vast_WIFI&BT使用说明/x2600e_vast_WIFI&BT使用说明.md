# WIFI_BT

> **开发板使用的wifi_bt模组是 AW-NB197SM new(43438)**

## 1 WIFI&BT配置

> **以x2600e_vast_v20_nand_5.10_factory_defconfig为例**

### 1.1 wpa工具配置

打开IConfigTool工具，选择配置文件

![选区_001](x2600e_vast_WIFI&BT使用说明.assets/001.png)



若要使⽤wpa 相关应⽤⼯具，那么buildroot 的配置⽂件需要添加，X2600这个默认配置已经添加了，配置为buildroot_x2600_510_wifi_common_defconfig，如果没有添加可以手动选上即可。

![选区_002](x2600e_vast_WIFI&BT使用说明.assets/002.png)



```c
bhu@bhu-PC:~/work/build$ cd ../buildroot/buildroot                            //进入buildroot目录
bhu@bhu-PC:~/work/buildroot/buildroot$ make menuconfig
```

![选区_003](x2600e_vast_WIFI&BT使用说明.assets/003.png)



可以看到wpa 相关应⽤⼯具已经添加，需要保存配置

```c
bhu@bhu-PC:~/work/buildroot/buildroot$ cp .config ../../build/configs/buildroot/buildroot_x2600_510_wifi_common_defconfig 
```



### 1.2 配置bt&wifi设备驱动

![选区_004](x2600e_vast_WIFI&BT使用说明.assets/004.png)



![选区_005](x2600e_vast_WIFI&BT使用说明.assets/005.png)



### 1.3 配置wifi mmc总线

![选区_007](x2600e_vast_WIFI&BT使用说明.assets/006.png)

### 1.4 kernel添加蓝牙通信串口uart3

```c
kenny@kenny-computer:~/source/source_darwin_linux_x2000/build$ grep -nr "kernel"    //先查看kernel的配置 configs/x2600e_vast_v20_nand_5.10_factory_defconfig 
6:APP_kernel_dir=../kernel/kernel
7:APP_kernel_config=x2600_module_base_linux_sfc_nand_defconfig     //kernel配置文件
```

配置文件位于kernel/kernel/arch/mips/configs目录下

```c
kenny@kenny-computer:~/source/source_darwin_linux_x2000/kernel/kernel/arch/mips/configs$ grep -nr ".dts"  ./x2600_module_base_linux_sfc_nand_defconfig
242:CONFIG_DT_X2600_MODULE_BASE_DTS_FILE="x2600_module_base.dts"     //关于uart配置
```

x2600_module_base.dts位于kernel/kernel/module_drivers/dts/目录下,可以对需要用到的串口进行添加修改

通过文档  [确定UART2方法](012UART2.md)   可知该配置用到的串口号为：UART2

```c
/dts-v1/;

#include <dt-bindings/input/input.h>
#include "x2600.dtsi"
#include <dt-bindings/interrupt-controller/irq.h>
#include <generated/autoconf.h>

/ {
	compatible = "ingenic,x2600";
	/*chosen {
		bootargs = "console=ttyS0,115200 mem=128M@0x0ip=off init=/linuxrc ubi.mtd=2 root=ubi0:rootfs ubi.mtd=3 rootfstype=ubifs rw flashtype=nand";
	};*/

};

&uart0 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&uart0_pe>;
};

&uart1 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&uart1_pb>;
};

&uart2 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&uart2_pd>;
};
```



### 1.5 开机自启wifi,bt

wifi，bt 固件路径存放在⼯程⽬录下的wireless/bcm/firmware/wifi_bcm/43438_infineon，wireless/bcm/firmware/bt_bcm编译时会复制到⽂件系统 的/lib/firmware/wifi_bcm，/lib/firmware/bt_bcm⽬录.

如果选择开机⾃启，系统启动时会⾃动将固件加载

![3](x2600e_vast_WIFI&BT使用说明.assets/007.png)



## 2 编译烧录

重新编译配置

```c
bhu@bhu-PC:~/work/build$ make x2600e_vast_v20_nand_5.10_factory_defconfig 

bhu@bhu-PC:~/work/build$ make
```

烧录固件, 没烧录过见[烧录说明](../01GettingStarted/05软件编译和烧录.md)



## 3 WIFI功能验证

使用 WiFi 需要设置想要连接 WiFi 的名称及密码,将配置写入文件,启动 WiFi 时运行配置文件即可

```c
# cd /usr/data/                                    //进入/usr/data/目录更改wifi配置文件

# cat wpa_supplicant.conf 

ctrl_interface=/var/run/wpa_supplicant
update_config=1

network={
    ssid="Guest"                                   //需要连接的wifi名称，不要是中文名称
    scan_ssid=1
    psk="ingenic_guest"                            //需要连接的wifi密码
    priority=1 
    }
```

配置完后重启,默认是开机自启wifi

```c
# ifconfig                                                                  //可以看到有wlan0出现

lo        Link encap:Local Loopback  
          inet addr:127.0.0.1  Mask:255.0.0.0
          UP LOOPBACK RUNNING  MTU:65536  Metric:1
          RX packets:5 errors:0 dropped:0 overruns:0 frame:0
          TX packets:5 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1 
          RX bytes:186 (186.0 B)  TX bytes:186 (186.0 B)

wlan0     Link encap:Ethernet  HWaddr 70:3A:2D:14:C6:90                     
          inet addr:12.10.70.118  Bcast:12.10.70.255  Mask:255.255.255.0
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:10 errors:0 dropped:0 overruns:0 frame:0
          TX packets:10 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000 
          RX bytes:10 (10.0 B)  TX bytes:10 (10.0 B)
          

# wifi_

wifi_down.sh  wifi_up.sh                                                     //可以通过wifi_up和wifi_down打开和关闭wifi

# ping www.baidu.com                                                         //ping百度看是否有网络

PING www.baidu.com (14.215.177.38): 56 data bytes
64 bytes from 14.215.177.38: seq=0 ttl=55 time=10.647 ms
64 bytes from 14.215.177.38: seq=1 ttl=55 time=24.899 ms
64 bytes from 14.215.177.38: seq=2 ttl=55 time=36.635 ms
64 bytes from 14.215.177.38: seq=3 ttl=55 time=10.538 ms
64 bytes from 14.215.177.38: seq=4 ttl=55 time=14.128 ms
64 bytes from 14.215.177.38: seq=5 ttl=55 time=11.692 ms
64 bytes from 14.215.177.38: seq=6 ttl=55 time=32.595 ms
^C
--- www.baidu.com ping statistics ---
7 packets transmitted, 7 packets received, 0% packet loss
round-trip min/avg/max = 10.538/20.162/36.635 ms
```

## 4 BT功能验证

查看bt是否启动，如果没有启动，则在/run/blue_bsa/目录下没有相关文件，需要手动启动 bt_enable_bsa.sh脚本

```c
# ls /run/blue_bsa/

ble_local_keys    bt-avk-fifo       bt-daemon-socket
```

执⾏商家提供的测试命令,这些命令存放在⼯程的 wireless/bcm/bin/xburst2_bsa/app/⽬录下

```c
bhu@bhu-PC:~/work/wireless/bcm/bin/xburst2_bsa/app$ ls
app_ag  app_avk  app_dg  app_hh  app_manager  app_ops  app_tm
app_av  app_ble  app_hd  app_hs  app_opc      app_pan  simple_app
```

通过adb将测试命令传到⽂件系统的/run/blue_bsa⽬录下，下⾯我⽤app_manager进⾏测试，扫描我的⼿机蓝⽛设备

```c
# ./app_manager 
```

```c
Bluetooth Application Manager Main menu:            //功能菜单
        1 => Abort Discovery
        2 => Discovery                              //搜索蓝牙设备              
        3 => Discovery test
        4 => Bonding
        5 => Cancel Bonding
        6 => Remove device from security database
        7 => Services Discovery (all services)
        8 => Device Id Discovery
        9 => Set local Device Id
        10 => Get local Device Id
        11 => Stop Bluetooth
        12 => Restart Bluetooth
        13 => Accept Simple Pairing
        14 => Refuse Simple Pairing
        15 => Enter BLE Passkey
        16 => Act As HID Keyboard (SP passkey entry)
        17 => Read Device configuration
        18 => Read Local Out Of Band data
        19 => Enter remote Out Of Band data
        20 => Set device visibility
        21 => Set device BLE visibility
        22 => Set AFH Configuration
        23 => Set Tx Power Class2 (specific FW needed)
        24 => Set Tx Power Class1.5 (specific FW needed)
        25 => Change Dual Stack Mode (currently:DUAL_STACK_MODE_BSA)
        26 => Set Link Policy
        27 => Enter Passkey
        28 => Get Remote Device Name
        29 => RSSI Measurement
        30 => Set class of device
        96 => Kill BSA server
        97 => Connect to BSA server
        98 => Disconnect from BSA server
        99 => Quit
Select action => 2                                   //选择搜索蓝牙设备
Start Regular Discovery
```

```c
New Discovered device:6
        Bdaddr:7c:e0:95:1b:05:67
        Name:iPhone                                 //搜索到蓝牙的设备名称
        ClassOfDevice:00:00:00 => Misc device
        Services:0x00000000 ()
        Rssi:-53
        DeviceType:BLE InquiryType:BLE AddressType:Random
        Extended Information:
            Flags:0x1a [LE_General Controller_LE/BR/EDR Host_LE/BR/EDR]
            TxPower:8 dB
            Manufacturer Specific CompanyId:0x004C [Apple, Inc.]:
                Data: 10 06 29 1D 7C F7 3C 08 
New Discovered device:7
        Bdaddr:dc:85:de:1a:57:64
        Name:jwu                                  //搜索到蓝牙的设备名称
        ClassOfDevice:1c:01:0c => Computer
        Services:0x00000000 ()
        Rssi:-78
        VidSrc:2 Vid:0x1D6B Pid:0x0246 Version:0x0530
        DeviceType:BR/EDR InquiryType:BR AddressType:Public
        Extended Information:
            FullName: jwu
            TxPower:4 dB
            DeviceId: VendorId:0x1D6B [USB] ProductId:0x0246 Version:0x0530
            Complete Service [UUID16]:
                0x110E [A/V Remote Control]
                0x110C [A/V Remote Control Target]
                0x110A [Audio Source]
                0x110B [Audio Sink]
                0x1112 [Headset Audio Gateway]
                0x1108 [Headset]
                0x1133 [Message Notification Server]
                0x1132 [Message Access Server]
                0x112F [Phonebook Server]
                0x1104 [IrMC Sync]
                0x1106 [OBEX File Transfer]
                0x1105 [OBEX Object Push]
            Complete Service [UUID128]:
                0x010000EE020000800010000005500000
Discovery complete                                         //搜索完成
```

