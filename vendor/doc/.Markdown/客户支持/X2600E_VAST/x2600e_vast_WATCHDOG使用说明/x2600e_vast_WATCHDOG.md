# WATCHDOG

## 一 配置

以PD_X2600E_VAST_V2.0开发板为例进行说明，选择主配置：x2600e_vast_v20_nand_5.10_factory_defconfig 为例.  打开IConfigTool工具,选择配置文件

![2023-11-27_17-49](x2600e_vast_WATCHDOG.assets/01.png)



x2600驱动列表勾选watchdog驱动，libhardware2勾选watchdog shell命令

![2023-11-28_10-51](x2600e_vast_WATCHDOG.assets/02.png)

watchdog0  可以复位。

watchdog1 不能系统复位，可以作为休眠唤醒功能的使用。





![2023-11-28_09-13](x2600e_vast_WATCHDOG.assets/03.png)



![2023-11-28_09-13_1](x2600e_vast_WATCHDOG.assets/04.png)



## 二 使用

串口输⼊ "lsmod" 查看当前是否已安装 watchdog 驱动，且 输⼊ "ls /dev/jz_watchdog" 可查看到该设备节点⽂件

```c
# lsmod                                                              //查看驱动是否安装
Module                  Size  Used by    Tainted: G  
x2600_510_icodec_sound_card     2488  0 
soc_watchdog            5715  0
# 

# ls /dev/jz_watchdog                                                          //查看是否存在watchdog设备节点
/dev/jz_watchdog
```



cmd_watchdog 命令相关使用方法

```c
cmd_watchdog start <ms>
功能：启动看门狗，同时设置最迟喂狗时间
参数：ms                                                //最迟喂狗时间
example：
             cmd_watchdog start 1000
```

```c
cmd_watchdog feed
功能：喂狗
参数：无
example：
            cmd_watchdog feed
```

```c
cmd_watchdog stop
功能：停止看门狗计数
参数：无
example：
             cmd_watchdog stop
```

```c
cmd_watchdog reset
功能：cpu复位重启
参数：无
example：
             cmd_watchdog reset
```



## 三 命令使用参考

```c
cmd_watchdog start 10000   //start 

cmd_watchdog feed          //feed 

cmd_watchdog stop          //stop 

cmd_watchdog reset         //reset 
```

> ① 执⾏ start，然后不做任何操作，start 执⾏10s后系统复位 
>
> ② 执⾏ start，然后执⾏ feed，feed 执⾏10s后系统复位 
>
> ③ 执⾏ start，然后执⾏ stop，stop 后 watchdog 不会再计数，系统也不会复位 
>
> ④ 执⾏ reset，reset 后系统复位
