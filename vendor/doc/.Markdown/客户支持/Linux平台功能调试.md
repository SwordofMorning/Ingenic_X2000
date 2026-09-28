**Linux平台功能调试**

功能调试有两种方式，一种是adb调试，一种是串口调试。 
 adb 调试一般针对应用类开发效率非常高。
 串口调试对于uboot和kernel开发，对于检查系统启动日志非常有用。

串口调试的波特率默认是：3Mbps，如果需要修改系统默认的串口波特率，可以修改相应的文件。
 比如修改x1600的串口波特率,其它芯片平台同理： 

bootloader/uboot-x2000/include/configs/x1600_base_common.h &

```
#ifndef CONFIG_BAUDRATE
#define CONFIG_BAUDRATE			3000000
#endif
```

这里默认是 3000000，可以手动修改为115200，然后重新编译uboot即可。   

x2000E修改串口波特率：

bootloader/uboot-x2000/include/configs/x2000_base_common.h

```
#ifndef CONFIG_BAUDRATE
#define CONFIG_BAUDRATE			3000000
#endif
```



x2000E 插针开发板使用的串口的序号是uart3,需要修改

```
/*
 * uart setting
 */
#ifndef CONFIG_SYS_UART_INDEX
#define CONFIG_SYS_UART_INDEX		3     (这里默认是2该成3)
#endif
```
