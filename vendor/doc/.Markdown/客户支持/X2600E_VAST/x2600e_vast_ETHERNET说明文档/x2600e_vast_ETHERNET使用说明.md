# ETHERNET

## 一 功能简介

​        以太网控制器也称以太网适配器，就是我们通常称的“网卡”，x2600E 以太网接口,只有一个phy_clk。支持百兆rmii。

## 二 驱动配置



![2023-12-01_11-10](x2600e_vast_ETHERNET使用说明.assets/01.png)

选择主配置文件：x2600e_vast_v20_nand_5.10_factory_defconfig



进入iconfig配置以太网驱动

![2023-11-24_17-17](x2600e_vast_ETHERNET使用说明.assets/02.png)





![2023-11-24_17-19](x2600e_vast_ETHERNET使用说明.assets/03.png)



## 三 基础验证

### 3.1  查看网络设备节点

```c
# ifconfig -a
eth0      Link encap:Ethernet  HWaddr 00:00:00:00:00:00              //以太网设备节点一般是ethX命名的   
          BROADCAST MULTICAST  MTU:1500  Metric:1
          RX packets:0 errors:0 dropped:0 overruns:0 frame:0
          TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000 
          RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)
          Interrupt:63 

lo        Link encap:Local Loopback  
          inet addr:127.0.0.1  Mask:255.0.0.0
          UP LOOPBACK RUNNING  MTU:65536  Metric:1
          RX packets:0 errors:0 dropped:0 overruns:0 frame:0
          TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000 
          RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)

wlan0     Link encap:Ethernet  HWaddr D0:31:10:00:00:00  
          UP BROADCAST MULTICAST  MTU:1500  Metric:1
          RX packets:0 errors:0 dropped:3 overruns:0 frame:0
          TX packets:1962 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000 
          RX bytes:0 (0.0 B)  TX bytes:671004 (655.2 KiB)

#            
```

### 3.2 配置ip地址及子网掩码

配置格式为

```c
ifconfig [⽹络设备] [ip地址] netmask [⼦⽹掩码] up
```

```c
# ifconfig eth0 10.4.3.202 netmask 255.255.255.0 up

# [  638.103441] eth0: device MAC address 4a:9e:6d:a6:60:79

# [  640.100211] stmmaceth stmmaceth.0 eth0: Link is Up - 100Mbps/Full - flow control off
```

```c
# ifconfig                                                   //查看网络配置是否生效
eth0      Link encap:Ethernet  HWaddr 12:7F:5B:25:4C:4C  
          inet addr:10.4.3.202  Bcast:10.4.3.255  Mask:255.255.255.0
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:11 errors:0 dropped:0 overruns:0 frame:0
          TX packets:13 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000 
          RX bytes:1040 (1.0 KiB)  TX bytes:1106 (1.0 KiB)
          Interrupt:63 

lo        Link encap:Local Loopback  
          inet addr:127.0.0.1  Mask:255.0.0.0
          UP LOOPBACK RUNNING  MTU:65536  Metric:1
          RX packets:8 errors:0 dropped:0 overruns:0 frame:0
          TX packets:8 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1 
          RX bytes:584 (584.0 B)  TX bytes:584 (584.0 B)
              
              
```

### 3.3 配置网关地址

配置格式为

```c
route add default gw [⽹关] 
```

```c
# route add default gw 10.4.3.1
```

### 3.4 ping 百度测试

开发板已经默认添加了域名解析，可以不用自己添加

```c
# cat /tmp/resolv.conf 
nameserver 202.96.134.133 # wlan0
nameserver 8.8.8.8 # wlan0
# ping www.baidu.com
PING www.baidu.com (14.215.177.38): 56 data bytes
64 bytes from 14.215.177.38: seq=0 ttl=55 time=7.632 ms
64 bytes from 14.215.177.38: seq=1 ttl=55 time=7.666 ms
64 bytes from 14.215.177.38: seq=2 ttl=55 time=7.565 ms
64 bytes from 14.215.177.38: seq=3 ttl=55 time=7.618 ms
64 bytes from 14.215.177.38: seq=4 ttl=55 time=7.779 ms
^C
--- www.baidu.com ping statistics ---
5 packets transmitted, 5 packets received, 0% packet loss
round-trip min/avg/max = 7.565/7.652/7.779 ms
```

### 3.5 关闭网络设备

```c
# ifconfig eth0 down
# ifconfig

lo        Link encap:Local Loopback  
          inet addr:127.0.0.1  Mask:255.0.0.0
          UP LOOPBACK RUNNING  MTU:65536  Metric:1
          RX packets:17 errors:0 dropped:0 overruns:0 frame:0
          TX packets:17 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1 
          RX bytes:1444 (1.4 KiB)  TX bytes:1444 (1.4 KiB)

wlan0     Link encap:Ethernet  HWaddr D0:31:10:85:43:27  
          inet addr:12.10.70.36  Bcast:12.10.70.255  Mask:255.255.255.0
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:1704 errors:0 dropped:455 overruns:0 frame:0
          TX packets:8 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000 
          RX bytes:142987 (139.6 KiB)  TX bytes:1400 (1.3 KiB)
```



## 四 mac phy 寄存器调试

![2023-05-19_16-00](x2600e_vast_ETHERNET使用说明.assets/05.png)

​	

```
cmd_mdio
	read operation: cmd_mdio eth reg_addr
	write operation: cmd_mdio eth addr value

For example:
	cmd_mdio eth0 1   读mac_phy寄存器
	cmd_mdio eth0 0 0x12  写mac_phy寄存器
```

