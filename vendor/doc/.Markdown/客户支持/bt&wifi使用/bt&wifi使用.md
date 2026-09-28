## 一，引导语

本页面仅针对linux平台的BT&WIFI的使用进行展示，包括代码配置、测试方法、驱动实现原理、参考文档说明等。目前在X1600和X2000上有测试。同一套linux代码下，wifi&bt的配置大同小异，可以实现cpu跨平台参考。

## 二，目前支持wifi列表及性能功耗

目前linux支持的wifi列表：

![Ｗｉｆｉ支持列表](bt&wifi使用.assets/Ｗｉｆｉ支持列表.png)

目前linux性能对比：

![1195px-Ｗｉｆｉ测试](bt&wifi使用.assets/1195px-Ｗｉｆｉ测试.png)

## 三，wifi 模块使用及添加文档

具体请参考：[wifi模块使用及添加说明文档.pdf](ftp://ftp.ingenic.com.cn/sz_ingenic/SDK/wifi模块使用及添加说明文档.pdf) 内容以下：

### 1，wifi 配置流程

主要讲解wifi驱动、msc、时钟、wifi自启动的相应代码配置。

### 2，wifi 模块驱动

主要讲解wifi工作原理及sdio相关基础知识及HOST层驱动匹配。

### 3，测试网络吞吐量 ( 带宽 )

主要讲解应用buildroot中的iperf/iperf3工具的配置及测试带宽的方法。

## 四，不同wifi模组的使用配置说明举例文档

doc/开发使用说明/bcm\ bt\&wifi使用说明文档/bcm bt&wifi使用说明文档.pdf

doc/开发使用说明/bcm\ bt\&wifi使用说明文档/esp32 wifi使用说明文档.pdf

doc/开发使用说明/bcm\ bt\&wifi使用说明文档/Hi3861l-WiFi模块使用说明文档.pdf

doc/开发使用说明/bcm\ bt\&wifi使用说明文档/realtek bt&wifi使用说明文档.pdf

## 五，wifi调试特别注意

如果同一份代码配置编译过其他wifi模组， 那么按照上述文档配置新的wifi模组之后，在编译buildroot之前，需要删除之前产生的wifi自启动脚本，不然会出现系统起来后，多个wifi脚本同时启动的乱象。具体如下：

```
 cd ../buildroot/buildroot/output/target/etc/init.d/                 
 rm -rf S48wifi_esp8089_up                      #delate all wifi shell files name Sxxwifi_xx_up
 cd ../../../../../../build
 make buildroot
```