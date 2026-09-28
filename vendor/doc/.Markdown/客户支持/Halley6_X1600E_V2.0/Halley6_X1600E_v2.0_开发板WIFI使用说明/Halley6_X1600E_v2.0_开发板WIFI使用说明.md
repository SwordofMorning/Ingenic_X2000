#### **Halley6_X1600E_V2.0 开发板WIFI使用说明**



##### 1： 先按照如下文档，让Halley6_X1600E_V2.0开发板先跑起来。

doc/FAE文档/Halley6_X1600E_V2.0/Halley6_X1600E_v2.0_开发板快速上手说明文档.pdf

使用/tools/iconfigtool/IConfigToolApp$ ./IConfigTool 工具，选择build/configs$ ls x1600e_halley6_nand_defconfig 配置。

配置如下面所示：

![2022-09-28_17-55](Halley6_X1600E_v2.0_开发板WIFI使用说明.assets/2022-09-28_17-55.png)





![2022-09-28_17-56](Halley6_X1600E_v2.0_开发板WIFI使用说明.assets/2022-09-28_17-56.png)





![2022-09-28_17-57](Halley6_X1600E_v2.0_开发板WIFI使用说明.assets/2022-09-28_17-57.png)



![2022-09-28_17-59](Halley6_X1600E_v2.0_开发板WIFI使用说明.assets/2022-09-28_17-59.png)



Ctrl+ S保存配置。

##### 2：重新编译烧录固件，可以参考doc/FAE文档/Halley6_X1600E_V2.0/Halley6_X1600E_v2.0_开发板快速上手说明文档.pdf



##### 3：测试WIFI相关的操作

`adb shell`

`cp /etc/wpa_supplicant.conf /usr/data/ -arf`

`/usr/data/wpa_supplicant.conf`  修改成如下内容所示，且ssid和psk修改成你自己的。



![2022-09-28_18-03](Halley6_X1600E_v2.0_开发板WIFI使用说明.assets/2022-09-28_18-03.png)

启动wifi命令如下：

先执行wifi_down.sh ,再执行 wifi_up.sh

![2022-09-28_18-08](Halley6_X1600E_v2.0_开发板WIFI使用说明.assets/2022-09-28_18-08.png)





![2022-09-28_18-09](Halley6_X1600E_v2.0_开发板WIFI使用说明.assets/2022-09-28_18-09.png)



判断wifi是否正常启动的命令如下：

![2022-09-28_18-12](Halley6_X1600E_v2.0_开发板WIFI使用说明.assets/2022-09-28_18-12.png)



![2022-09-28_18-12_1](Halley6_X1600E_v2.0_开发板WIFI使用说明.assets/2022-09-28_18-12_1.png)



