# 一 编译方法

**整体编译命令如下:**

```c
bhu@bhu-PC:~/work$ make x2000_halley5_v30_nand_factory_defconfig

bhu@bhu-PC:~/work$ make 
```

**编译以后固件目录,如下:**

```c
bhu@bhu-PC:~/work/build/output$ ls -lh
总用量 8.0M
-rw-r--r-- 1 bhu bhu 4.4M 4月  24 14:45 rootfs.squashfs
-rw-r--r-- 1 bhu bhu  24K 4月  24 14:45 u-boot-spl-pad.bin
-rw-r--r-- 1 bhu bhu 3.7M 4月  24 14:45 xImage
```



# 二 最新烧录工具获取

**ubuntu版本**

```c
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz 
```

**windows版本**

```c
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windows.zip
```



# 三 烧录方法

以ubuntu版工具为例

接上板子上的USB_DOWNLOAD 接口，供电为5V3A，进入烧录工具

```c
bhu@bhu-PC:~/Desktop/cloner-2.5.33-ubuntu_alpha$ sudo ./cloner 
```

![1](RD_X2000_HALLEY5_BASEBOARD_V3.0_开发板快速上手说明文档.assets/1.png)

![2](RD_X2000_HALLEY5_BASEBOARD_V3.0_开发板快速上手说明文档.assets/2.png)

![3](RD_X2000_HALLEY5_BASEBOARD_V3.0_开发板快速上手说明文档.assets/3.png)

配置好后保存配置，板子上长按BOOT不动，再按RST_N松开，松开BOOT进入烧录模式



# 四 常用功能



## 1. 串口调试



开发板使用串口uart2,波特率为3000000



## 2. Camera



```c
# ls /dev/mscaler                                              //设备节点
mscaler0-ch0  mscaler0-ch2  mscaler1-ch1
mscaler0-ch1  mscaler1-ch0  mscaler1-ch2
 
# cmd_isp init /dev/mscaler0-ch0 width=640 height=480 frame_nums=3 format=NV12         //初始化isp设备

# cmd_isp power_on /dev/mscaler0-ch0                            //使能设备
    
# cmd_isp stream_on /dev/mscaler0-ch0                           //打开图像录制

# cmd_isp get_frame /dev/mscaler0-ch0 > /tmp/frame              //获取图片保存
    
# cmd_isp stream_off /dev/mscaler0-ch0                          //关闭图像录制

# cmd_isp power_off /dev/mscaler0-ch0                           //关闭设备
```



## 3. SPK

```c
# aplay -l                                                                                     //当前播放设备详情
**** List of PLAYBACK Hardware Devices ****
card 1: icodecsoundcard [icodec-sound-card], device 0: x2000 icodec pcm internal-codec-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0

      
# aplay -Dplughw:1,0 /usr/data/audio_pcm.aiff 
Playing WAVE '/usr/data/audio_pcm.aiff' : Signed 16 bit Little Endian, Rate 16000 Hz, Stereo   //播放音频文件

```



## 4. AMIC

```c
# arecord -Dhw:1,0 -d 10 -f S16_LE -r 16000 -c 2 -t wav /usr/data/test.wav                     //amic录音

Recording WAVE '/usr/data/test.wav' : Signed 16 bit Little Endian, Rate 16000 Hz, Stereo
```



## 5. Ethernet

```c
# ifconfig eth0 10.4.3.202 netmask 255.255.255.0 up               //网络设备开启，配置ip地址及子网掩码
# route add default gw 10.4.3.1                                   //配置网关地址
# echo nameserver 8.8.8.8  > /tmp/resolv.conf                     //添加域名解析
# cat /tmp/resolv.conf 
nameserver 8.8.8.8
# ping www.baidu.com                                              //网络测试
PING www.baidu.com (14.119.104.254): 56 data bytes
64 bytes from 14.119.104.254: seq=0 ttl=55 time=7.531 ms
64 bytes from 14.119.104.254: seq=1 ttl=55 time=7.573 ms
# ifconfig eth0 down                                              //网络设备关闭
```



## 6. WIFI/BT

```c
# cat /usr/data/wpa_supplicant.conf                                    //配置wifi热点名称及密码

ctrl_interface=/var/run/wpa_supplicant
update_config=1

network={
    ssid="Guest"                                                       //wifi名称
    scan_ssid=1
    psk="ingenic_guest"                                                //wifi密码                          
    priority=1 
    }
# wifi_down.sh                                                        //关闭wifi
# wifi_up.sh                                                          //重启wifi
# ping www.baidu.com                                                  //网络测试
PING www.baidu.com (14.119.104.189): 56 data bytes                     
64 bytes from 14.119.104.189: seq=0 ttl=55 time=12.775 ms
64 bytes from 14.119.104.189: seq=1 ttl=55 time=13.204 ms

--- www.baidu.com ping statistics ---
2 packets transmitted, 2 packets received, 0% packet loss
round-trip min/avg/max = 12.775/12.989/13.204 ms

```

```c
# ls /run/blue_bsa/                                              //查看bt是否启动，没有则该目录下没文件，需手动运行bt_enable_bsa.sh

app_manager       bt-avk-fifo       bt_config.xml
ble_local_keys    bt-daemon-socket
    
# ./app_manager                     //通过adb将⼯程wireless/bcm/bin/xburst2_bsa/app/⽬录下的app_manager push到这个目录并运行
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
Discovery complete  
```

