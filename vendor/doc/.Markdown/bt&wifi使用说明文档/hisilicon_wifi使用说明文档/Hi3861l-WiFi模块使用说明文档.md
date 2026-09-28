# Hi3861l-WiFi模块使用说明文档

## 1.配置开发板连接到WiFi

1.1 开发板上电，先检查WiFi模块是否正常，我们可以使用命令 ifconfig -a 简单检查WiFi功能是否正常，正常的情况下，将会出现wlan0，如下图所示。

<img src="img/1.png" style="zoom:100%;" />

注：

​		若未能出现wlan0，可能是Hi3861lWiFi模块固件未烧录或者是WiFi驱动未加载，后续操作无法进行。

1.2进入到 /usr/data 目录，该目录下存在连接WiFi的配置文件 wpa_supplicant.conf，如下图示

<img src="img/2.png" style="zoom:100%;" />

1.3 使用vi  命令，来修改联网配置文件 wpa_supplicant.conf 中ssid 和psk 行的双引号内的内容，进行配置连接到的WiFi。如下图所示的内容配置，表示将连接的WiFi名为 Z2，密码为66666666。

<img src="img/3.png" style="zoom:100%;" />

注： 

​      A. 若/usr/date目录下无此配网文件，可以手动创建。该配网文件 /usr/data/wpa_supplicant.conf 内容为

network={
    ssid="Guest"
    key_mgmt=WPA-PSK
    psk="ingenic_guest"

}

​     B. Hi3861L只工作在2.4G频段，所以链接的**AP热点必须支持2.4G频段**,否则会出现无法链接的现象。

​     C. 在测试使用时,需链接匹配的天线，否则容易出现无法链接, 链接获取不到IP等状况。

1.4 在保存修改的 wpa_supplicant.conf 内容后，通过命令 ps | grep hichannel_service 检查配网后台程序是否正常运行。若配网后台进程运行正常，效果将类似下图，可继续进行配网操作。

<img src="img/4.png" style="zoom:100%;" />

注：

​		若未能通过本操作查找到配网后台进程 hichannel_service，可通过下面的命令手动启动配网进程服务：

hichannel_service & 启动成功后的效果类似下图，表示Hi3861l工作正常，并获取到了mac地址。若手动启动配网进程未能获取到mac地址，可能是开发板的Hi3861l模块暂时未烧录更新的固件。

<img src="img/5.png" style="zoom:100%;" />

1.5 若1.4步骤操作结果正常，可通过命令 hichannel_cli netcfg 进行配置开发板连接到WiFi，下图是读取配置文件成功，并正常接入WiFi获取到了IP地址。

<img src="img/6.png" style="zoom:100%;" />

注：

​		在进行过1.5 中的配网操作后，以后WiFi模块上电将默认连接入该WiFi。若开发板未能在重新上电后自动获取到IP地址，类似地，我们可通过  hichannel_cli getip 获取到IP到开发板，如下图所示

<img src="img/7.png" style="zoom:100%;" />

1.5 配网最后，进行验证。我们可以通过 ifconfig 命令检查网络情况，如下图。

<img src="img/8.png" style="zoom:100%;" />

## 2.配置开发板启动AP（热点）功能

2.1 使用默认的热点设置：通过命令 hichannel_cli startap，执行成功将可见softap start success! 字符提示，然后显示热点的IP信息。目前默认启动的AP名为SmartLife-0F3E 密码为12345678。

<img src="img/9.png" style="zoom:100%;" />

![image-20220325164525320](/home/ingenic/work/linux/doc/.Markdown/bt&wifi使用说明文档/hisilicon bt&wifi使用说明文档/img/10.png)

2.2 用户配置热点信息

​		a. AP需要密码 

​		命令格式：hichannel_cli startap ssid=WiFi名 psk=WiFi密码 

​		执行上面的命令后可以搜索到对应的WiFi，执行效果见下图

<img src="img/11.png" style="zoom:100%;" />

​		b. AP无需密码

​		命令格式 hichannel_cli startap ssid=WiFi名

​		执行上面的命令后可以搜索到对应的WiFi，执行效果见下图

<img src="img/12.png" style="zoom:100%;" />

## 3.配置开发板向服务器进行保活

3.1 配置开发板接入WiFi，详见 1.配置开发板连接到WiFi 部分内容。

3.2 测试该功能需要先开启一个TCP服务器，这里使用window环境下的软件sokit进行演示。运行该软件，将TCP地址修改成测试电脑的IP地址，然后点击TCP按钮即可开启TCP服务器并监听对应端口，开启成功后可见下方出现successfully字样。

<img src="img/13.png" style="zoom:100%;" />

3.3 通过命令 hichannel_cli filterwifi 将数据报文处理功能下放到WiFi模块Hi3861l。执行成功的效果如下图。

<img src="img/14.png" style="zoom:100%;" />

3.4 接着我们配置保活的服务器信息，执行命令

hichannel_cli keeplive serverip=192.168.1.101 port=9000 expire=10 

其中serverip对应保活目标服务器IP、port对应保活目标服务器端口、expire对应保活周期（秒）即发送保活消息间隔。 命令执行成功的效果如下图。

<img src="img/15.png" style="zoom:100%;" />

3.5 sokit软件端可见当前连接区域出现建立成功的TCP通信，效果见下图。

<img src="img/16.png" style="zoom:100%;" />

## 4.配置开发板进入深度睡眠状态

4.1 在3.配置开发板向服务器进行保活的基础上，执行命令 hichannel_cli deepsleep，将使得开发板掉电，而WiFi模块不掉电，命令行效果为无法再和开发板通信，如下图所示。

<img src="img/17.png" style="zoom:100%;" />

## 5.通过网络唤醒开发板

5.1 按照步骤 3.配置开发板向服务器进行保活和 4.配置开发板进入深度睡眠状态 中的说明完成操作，使得开发板对服务器进行保活然后进入深度睡眠状态。

5.2 通过服务器端与连接上的开发板端进行TCP通信，发送wakeup字符到开发板端，开发板将被WiFi模块上电启动，操作过程详见下图。

<img src="img/18.png" style="zoom:100%;" />

5.3 点击发送后将可以看到sokit软件端的TCP连接被关闭，另外开发板被上电启动，效果见下图所示。

<img src="img/19.png" style="zoom:100%;" />

5.4 另外开发板被上电启动，效果见下图所示。

<img src="img/20.png" style="zoom:100%;" />

## 6.WiFi模块OTA功能

6.1 执行命令 hichannel_cli startota PATH=/usr/data/Hi3861L_demo_ota.bin来进行WiFi模块的固件升级，固件升级中，将会有log提示。在完成WiFi模块的固件升级后，开发板将进行一次重启。固件升级过程见下图。

<img src="img/21.png" style="zoom:100%;" />

注：

​		1.固件升级对网络无要求。

​		2.若不指定固件的路径即执行命令 hichannel_cli startota ，系统将默认使用 /usr/data/Hi3861L_demo_ota.bin 来进行固件升级。若无固件升级文件在开发板中，需使用命令 adb push yourotafilename.bin /usr/data/，将使用的固件包发送到开发板，然后执行固件升级命令即可进行固件升级。

