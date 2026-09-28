## 																							halley6_v2.0以太网使用方法



### **一  硬件简介**

HALLEY6_BASEBOARD_V2.0 + X1600E + SPI nand。

<img src="halley6_v2.0以太网配置方法.assets/1.png" alt="1" style="zoom:150%;" />



<img src="halley6_v2.0以太网配置方法.assets/2.png" alt="2" style="zoom:150%;" />

### **二  以太网硬件设计**

#### 1 **功能引脚切换电路**

本设计中，I2S和MAC的引脚是复用的，做功能切换的是两个切换芯片U13和U17。当 U13、U17 的 2 脚（IN）为低电平 0 时，此时为 MAC 功能。当 U13、U17 的 2 脚（IN）为高电平1 时，此时为I2S功能。

因此，由下图切换电路的设计可见，跳冒CON6插上就是I2S导通，跳冒CON6拔出就是MAC导通。

<img src="halley6_v2.0以太网配置方法.assets/3.png" alt="3" style="zoom:150%;" />

X1600 处理器集成 1 路 MAC 控制器，支持传输速度为 10/100Mbps，支持 RMII PHY 接口。Halley6 开发板
外加物理层处理芯片 IP101GR，IP101GR 的时钟信号默认由 X1600 控制器内部输出时钟提供。也可修改硬件由外部
25MHz 晶体提供，此时软件上也需要有必要的调整。

#### 2  **控制引脚设计**

<img src="halley6_v2.0以太网配置方法.assets/4.png" alt="4" style="zoom:150%;" />



![5](halley6_v2.0以太网配置方法.assets/5.png)

### **三  软件配置**

<img src="halley6_v2.0以太网配置方法.assets/6.png" alt="6" style="zoom:150%;" />

保存上述配置：

<img src="halley6_v2.0以太网配置方法.assets/7.png" alt="7" style="zoom:150%;" />

### **四  编译**

```
sxyzhang@T430:~/wj_sz/build$ make x1600e_halley6_nand_defconfig
sxyzhang@T430:~/wj_sz/build$ make
```

### **五  测试**

#### 1  编译烧录后的网络设备节点

<img src="halley6_v2.0以太网配置方法.assets/8.png" alt="8" style="zoom:150%;" />

#### 2  网络设备的开启

**注意:**网络设备名称,如 eth0 ,请根据具体板级设备进行修改 ;

```
ifconfig [网络设备] [up]
举例:
	ifconfig eth0 up

如需关闭这个网络设备,可以使用：
	ifconfig [网络设备] [down]
举例:
	ifconfig eth0 down
```

#### 3  手动配置网络设备ip

**注意**:网络设备名称,如 eth0 ,请根据具体板级设备进行修改 ; ip 地址、子网掩码,请根据具体网络环境进行修改 ;

##### 3.1  设置 ip 地址和子网掩码

```
ifconfig [网络设备] [ip地址] netmask [子网掩码]
举例:
ifconfig eth0 194.169.3.195 netmask 255.255.255.0
```

配置完成结果如下：

<img src="halley6_v2.0以太网配置方法.assets/9.png" alt="9" style="zoom: 200%;" />

##### 3.2  配置网关地址，添加默认路由

**注意:**网关地址,请根据具体网络环境进行修改 ;

```
route add default gw [网关地址]
举例:
route add default gw 194.169.3.1
```

##### 3.3  在开发板上天加域名解析

```
echo nameserver 8.8.8.8' >> /tmp/resolv.conf
```

#### 4  动态分配网络设备ip

##### 4.1 配置路由器 ( 如果路由器可以正常上网就不需要配置 )

PC 机连接路由器网络,进入路由器配置界面进行路由器网络配置,配置好后重新启动路由器即可。

<img src="halley6_v2.0以太网配置方法.assets/10.png" alt="10" style="zoom:150%;" />



<img src="halley6_v2.0以太网配置方法.assets/11.png" alt="11" style="zoom:150%;" />



<img src="halley6_v2.0以太网配置方法.assets/12.png" alt="12" style="zoom:150%;" />

##### 4.2 动态获取IP地址

路由器与开发板用网线连接后,进行动态获取 ip 地址

```
udhcpc -i eth0
```

#### 5  测试网络域名，ping 百度

结果如下，表示网络配置成功

<img src="halley6_v2.0以太网配置方法.assets/13.png" alt="13" style="zoom: 200%;" />