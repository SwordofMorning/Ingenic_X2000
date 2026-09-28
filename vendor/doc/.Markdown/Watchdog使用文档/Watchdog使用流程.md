# Watchdog 使用流程

## 1.配置

**watchdog 配置、加载，根据各种型号设备，请参考 <u>/doc/开发使⽤说明/⽚上外设模块驱动</u> ⽬录下对应型号⽂档，watchdog 看门狗章节。**

举例：

x1000的板级设备参考 <u>/doc/开发使⽤说明/⽚上外设模块驱动/X1000⽚上外设模块驱动.pdf</u>  ;

x1021的板级设备参考 <u>/doc/开发使⽤说明/⽚上外设模块驱动/X1021⽚上外设模块驱动.pdf</u>  ;

x1520的板级设备参考 <u>/doc/开发使⽤说明/⽚上外设模块驱动/X1520⽚上外设模块驱动.pdf</u>  ;

x1830的板级设备参考 <u>/doc/开发使⽤说明/⽚上外设模块驱动/X1830⽚上外设模块驱动.pdf</u>  ;

x2000的板级设备参考 <u>/doc/开发使⽤说明/⽚上外设模块驱动/X2000⽚上外设模块驱动.pdf</u>  ;

**`打开你所用板极的配置，这里以x2000为例`**

<img src="img/1.png" alt="1" style="zoom:150%;" />

<img src="img/2.png" alt="2" style="zoom:150%;" />



## 2.编译、烧录

**编译、烧录，根据各种型号设备，请参考 <u>/doc/开发使⽤说明/1_Linux工程编译说明.pdf</u> 工程编译流程章节以及 <u>/doc/开发使⽤说明/2_Linux工程烧录介绍.pdf</u> 文档。**



## 3.使用

**1.在已烧录的情况下直接按回车进入命令行，输入 "lsmod" 查看当前是否已安装 watchdog 驱动，且输入 "ls /dev/jz_watchdog" 可查看到该设备节点文件**

```c
lsmod                   //查看驱动是否安装

ls /dev/jz_watchdog     //是否存在设备节点文件
```

<img src="img/3.png" alt="3" style="zoom:150%;" />

**2.输入 cmd_watchdog 查看该命令具体用法**

<img src="img/4.png" alt="4" style="zoom:150%;" />

`若提示没有该命令请查看是否已配置 libhardware2片上外设接口/命令，feed及stop命令使用前提是已start `

**3.命令使用参考**

```c
cmd_watchdog start 10000    //start
cmd_watchdog feed           //feed
cmd_watchdog stop           //stop
cmd_watchdog reset          //reset
```

> ① 执行 start，而后不做任何操作，start 执行10s后系统复位
> ② 执行 start，而后执行 feed，feed 执行10s后系统复位
> ③ 执行 start，而后执行 stop，stop 后 watchdog 不会再计数，系统也不会复位
> ④ 执行 reset，reset 后系统复位