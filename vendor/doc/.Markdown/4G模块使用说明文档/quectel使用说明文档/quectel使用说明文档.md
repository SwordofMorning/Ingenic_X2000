# quectel 4G 模块使用说明文档

1. 这里以 EC20 4G模块为说明
<img src="image/EC20-4G模块.jpg" style="zoom:30%;" />

2. 使用的开发板是 x2000-evb 开发板
<img src="image/x2000-evb.jpg" style="zoom:30%;" />

### 内核配置

1. 编译了整个文件系统和kernel
```
$ cd ingenic_linux_project/
$ cd build
$ make clean
$ make x2000_evb_v11_nand_defconfig
$ make
```

2. 进入kernel 使能 ppp 相关配置
    make menuconfig 选中如下选项
    CONFIG_PPP_ASYNC CONFIG_PPP_SYNC_TTY CONFIG_PPP_DEFLATE
```
[*] Device Drivers
  [*] Network device support
    [*] PPP (point-to-point protocol) support
```

<img src="image/ppp_kernel.png" style="zoom:100%;" />

3. 进入kernel 使能 dwc usb 相关配置
   make menuconfig 选中如下选项
   CONFIG_USB_DWC2_HOST 或者 CONFIG_USB_DWC2_DUAL_ROLE
```
 [*] Device Drivers                                        
   [*] USB support
     [*] DesignWare USB2 DRD Core Support
       [*] DWC2 Mode Selection
```
<img src="image/usb_dual_role.png" style="zoom:100%;" />

4. 进入kernel 使能 usb serial 相关选项
   make menuconfig 选中如下选项
   CONFIG_USB_SERIAL CONFIG_USB_SERIAL_OPTION
```
   [*] Device Drivers
     [*] USB support
       [*] USB Serial Converter support
         [*] USB driver for GSM and CDMA modems
```
<img src="image/usb_serial.png" style="zoom:100%;" />

5. 安装 quectel 4G模块相关脚本
   使用IConfigtool 选中APP_wireless_quectel
```
[ root ]
  - [ wireless(无线设备) ]
    - [ quectel 4G 模块 ]
```
<img src="image/iconfig_quectl.png" style="zoom:100%;" />

7. 最好删除 adb 或者其它usb相关的启动脚本
   使用IConfigtool ***不勾选*** APP_br_adb_server_start
```
 [ root ]
  - [ buidroot 相关配置 ]
   - [ rootfs 相关配置 ]
     - [ adb 服务脚本 ]
       - [ 开机启动adb服务 ]
```
<img src="image/iconfig_no_adb.png" style="zoom:100%;" />

8. 重新编译,并烧录
```
$ cd ingenic_linux_project/
$ cd build
$ make
```

9. 确认usb 设别节点被识别
    如果usb设备识别成功dmesg会有如下打印
```
# dmesg
....
[   11.250037] usb 1-1: new high-speed USB device number 2 using dwc2
[   11.470058] usb 1-1: New USB device found, idVendor=2c7c, idProduct=0125
[   11.470069] usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
[   11.470076] usb 1-1: Product: Android
[   11.470081] usb 1-1: Manufacturer: Android
[   11.471448] option 1-1:1.0: GSM modem (1-port) converter detected
[   11.472046] usb 1-1: GSM modem (1-port) converter now attached to ttyUSB0
[   11.472319] option 1-1:1.1: GSM modem (1-port) converter detected
[   11.472531] usb 1-1: GSM modem (1-port) converter now attached to ttyUSB1
[   11.472784] option 1-1:1.2: GSM modem (1-port) converter detected
[   11.473059] usb 1-1: GSM modem (1-port) converter now attached to ttyUSB2
[   11.473467] option 1-1:1.3: GSM modem (1-port) converter detected
[   11.473843] usb 1-1: GSM modem (1-port) converter now attached to ttyUSB3
```
```
# ls /dev/ttyUSB*
/dev/ttyUSB0  /dev/ttyUSB1  /dev/ttyUSB2  /dev/ttyUSB3
```
10. 测试ppp拨号功能
    先检查 ppp相关脚本是否已经安装
```
# tree /etc/ppp/
/etc/ppp/
|-- ip-up
`-- peers
    |-- quectel-chat-connect
    |-- quectel-chat-disconnect
    `-- quectel-ppp

# cat /usr/bin/gsm_up.sh 
#!/bin/sh

pppd call quectel-ppp &
```
    执行gsm.up.sh
```
# gsm_up.sh
# pppd options in effect:
debug		# (from /etc/ppp/peers/quectel-ppp)
nodetach		# (from /etc/ppp/peers/quectel-ppp)
dump		# (from /etc/ppp/peers/quectel-ppp)
noauth		# (from /etc/ppp/peers/quectel-ppp)
user test		# (from /etc/ppp/peers/quectel-ppp)
password ??????		# (from /etc/ppp/peers/quectel-ppp)
remotename 3gppp		# (from /etc/ppp/peers/quectel-ppp)
/dev/ttyUSB3		# (from /etc/ppp/peers/quectel-ppp)
115200		# (from /etc/ppp/peers/quectel-ppp)
lock		# (from /etc/ppp/peers/quectel-ppp)
connect chat -s -v -f /etc/ppp/peers/quectel-chat-connect		# (from /etc/ppp/peers/quectel-ppp)
disconnect chat -s -v -f /etc/ppp/peers/quectel-chat-disconnect		# (from /etc/ppp/peers/quectel-ppp)
nocrtscts		# (from /etc/ppp/peers/quectel-ppp)
modem		# (from /etc/ppp/peers/quectel-ppp)
hide-password		# (from /etc/ppp/peers/quectel-ppp)
novj		# (from /etc/ppp/peers/quectel-ppp)
novjccomp		# (from /etc/ppp/peers/quectel-ppp)
ipcp-accept-local		# (from /etc/ppp/peers/quectel-ppp)
ipcp-accept-remote		# (from /etc/ppp/peers/quectel-ppp)
ipparam 3gppp		# (from /etc/ppp/peers/quectel-ppp)
noipdefault		# (from /etc/ppp/peers/quectel-ppp)
ipcp-max-failure 30		# (from /etc/ppp/peers/quectel-ppp)
defaultroute		# (from /etc/ppp/peers/quectel-ppp)
usepeerdns		# (from /etc/ppp/peers/quectel-ppp)
noccp		# (from /etc/ppp/peers/quectel-ppp)
abort on (BUSY)
abort on (NO CARRIER)
abort on (NO DIALTONE)
abort on (ERROR)
abort on (NO ANSWER)
timeout set to 30 seconds
send (AT^M)
expect (OK)
^M
OK
 -- got it

send (ATE0^M)
expect (OK)
^M
^M
OK
 -- got it

send (ATI;+CSUB;+CSQ;+CPIN?;+COPS?;+CGREG?;&D2^M)
expect (OK)
^M
^M
Quectel^M
EC20F^M
Revision: EC20CEFDR02A10M4G^M
^M
SubEdition: V05^M
^M
+CSQ: 19,99^M
^M
+CPIN: READY^M
^M
+COPS: 0,0,"CHN-CT",7^M
^M
+CGREG: 0,1^M
^M
OK
 -- got it

send (AT+CGDCONT=1,"IP","3gnet",,0,0^M)
expect (OK)
^M
^M
OK
 -- got it

send (ATD*99#^M)
expect (CONNECT)
^M
^M
CONNECT
 -- got it

Script chat -s -v -f /etc/ppp/peers/quectel-chat-connect finished (pid 763), status = 0x0
Serial connection established.
using channel 3
Using interface ppp0
Connect: ppp0 <--> /dev/ttyUSB3
sent [LCP ConfReq id=0x1 <asyncmap 0x0> <magic 0x3b37f232> <pcomp> <accomp>]
rcvd [LCP ConfReq id=0x2 <asyncmap 0x0> <auth chap MD5> <magic 0x9a48d973> <pcomp> <accomp>]
sent [LCP ConfAck id=0x2 <asyncmap 0x0> <auth chap MD5> <magic 0x9a48d973> <pcomp> <accomp>]
rcvd [LCP ConfAck id=0x1 <asyncmap 0x0> <magic 0x3b37f232> <pcomp> <accomp>]
rcvd [LCP DiscReq id=0x3 magic=0x9a48d973]
rcvd [CHAP Challenge id=0x1 <3e197ef0a70a33470e3247ddcb9d30df>, name = "UMTS_CHAP_SRVR"]
sent [CHAP Response id=0x1 <0e100688c96423530e9347a6f6b996ed>, name = "test"]
rcvd [CHAP Success id=0x1 ""]
CHAP authentication succeeded
CHAP authentication succeeded
sent [IPCP ConfReq id=0x1 <addr 0.0.0.0> <ms-dns1 0.0.0.0> <ms-dns2 0.0.0.0>]
rcvd [IPCP ConfReq id=0x0]
sent [IPCP ConfNak id=0x0 <addr 0.0.0.0>]
rcvd [IPCP ConfNak id=0x1 <addr 10.12.58.70> <ms-dns1 202.96.134.33> <ms-dns2 202.96.128.166>]
sent [IPCP ConfReq id=0x2 <addr 10.12.58.70> <ms-dns1 202.96.134.33> <ms-dns2 202.96.128.166>]
rcvd [IPCP ConfReq id=0x1]
sent [IPCP ConfAck id=0x1]
rcvd [IPCP ConfAck id=0x2 <addr 10.12.58.70> <ms-dns1 202.96.134.33> <ms-dns2 202.96.128.166>]
Could not determine remote IP address: defaulting to 10.64.64.64
local  IP address 10.12.58.70
remote IP address 10.64.64.64
primary   DNS address 202.96.134.33
secondary DNS address 202.96.128.166
Script /etc/ppp/ip-up started (pid 767)
Script /etc/ppp/ip-up finished (pid 767), status = 0x0

```
测试联网
```
# ping www.baidu.com
PING www.baidu.com (14.215.177.38): 56 data bytes
64 bytes from 14.215.177.38: seq=0 ttl=54 time=46.530 ms
64 bytes from 14.215.177.38: seq=1 ttl=54 time=81.310 ms
64 bytes from 14.215.177.38: seq=2 ttl=54 time=39.274 ms
64 bytes from 14.215.177.38: seq=3 ttl=54 time=24.760 ms
```