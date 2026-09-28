# esp32 wifi使用说明文档

## 1.配置esp32 wifi

### 1.1 配置wifi驱动

1.首先进入模块化驱动

<img src="/home/robin/.config/Typora/typora-user-images/image-20220718142747728.png" alt="image-20220718142747728"  />

2.进入外设

![image-20220718142847800](/home/robin/.config/Typora/typora-user-images/image-20220718142847800.png)

3.点击进入配置wifi设备驱动

![image-20220718143111891](/home/robin/.config/Typora/typora-user-images/image-20220718143111891.png)

4.勾选ESP32 wireless cards support, 并点击进入配置引参数

![image-20220718143529258](/home/robin/.config/Typora/typora-user-images/image-20220718143529258.png)

注:引脚参数根据硬件设计配置,没有使用的引脚或参数配置选项保持默认配置即可.

### 1.2 配置wifi通信mmc端口

1.进入模块化驱动,同上

2.进入x2000驱动列表

![image-20220718144255703](/home/robin/.config/Typora/typora-user-images/image-20220718144255703.png)

3.勾选msc控制器驱动

![image-20220718144508872](/home/robin/.config/Typora/typora-user-images/image-20220718144508872.png)

4.勾选相应的端口

![image-20220718144710481](/home/robin/.config/Typora/typora-user-images/image-20220718144710481.png)

5.选择配置(除以下红框圈起来配置外,其他配置不勾选)

![image-20220718144834655](/home/robin/.config/Typora/typora-user-images/image-20220718144834655.png)

![image-20220718145156927](/home/robin/.config/Typora/typora-user-images/image-20220718145156927.png)

![image-20220718145223841](/home/robin/.config/Typora/typora-user-images/image-20220718145223841.png)

### 1.3 配置wifi开机自启

1.回到主界面,进入wireless(无线设备)

![image-20220718145648755](/home/robin/.config/Typora/typora-user-images/image-20220718145648755.png)

2.勾选并点击进入esp32 wifi

![image-20220718145738418](/home/robin/.config/Typora/typora-user-images/image-20220718145738418.png)

3.勾选开机自启wifi

![image-20220718145851675](/home/robin/.config/Typora/typora-user-images/image-20220718145851675.png)

## 2.搭建ESP_IDF环境

(以下步骤基于https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/linux-macos-setup.html,具体解释说明参考网址)

### 2.1 安装软件包

sudo apt-get install git wget flex bison gperf python3 python3-venv python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

### 2.2 开始搭建ESP_IDF

1.打开终端,运行以下命令

mkdir -p ~/esp         #在用户目录创建esp文件夹

cd ~/esp

git clone --recursive https://github.com/espressif/esp-idf.git 

(文件自动保存在esp/esp_idf目录中)

2.设置工具

为支持 ESP32 的项目安装 ESP-IDF 使用的各种工具，比如编译器、调试器、Python 包等

cd ~/esp/esp-idf

export IDF_GITHUB_ASSETS="dl.espressif.com/github_assets"

./install.sh esp32                                           #要求python版本在3.7以上,时间比较久

~/esp/esp-idf$ git branch                             #查看当前分支

~/esp/esp-idf$ git checkout release/v4.0   #切换到release/v4.0分支

~/esp/esp-idf$ git log -1                                #查看分支commit be7df8bce9c12c020d772ef1e71a773025f5177a (HEAD -> release/v4.0, origin/release/v4.0)

3.设置环境变量

. $HOME/esp/esp-idf/export.sh                   #每次重新打开esp-idf使用时都需要重新设置环境变量

## 3.烧录efuse和固件

### 3.1 烧录efuse

(将驱动中固件下载相关打开,进入下载模式CONFIG_ENTER_DOWNLOAD = y)

cd ~/esp/esp-idf/components/esptool_py/esptool

~/esp/esp-idf/components/esptool_py/esptool$ espefuse.py set_flash_voltage 3.3V 

注:烧录efuse的时候硬件连接电源线,烧录线,串口线,但是不要打开串口调试工具

完成后可看到

Connecting...........
Detecting chip type... Unsupported detection protocol, switching and trying again...
Connecting...
Detecting chip type... ESP32
espefuse.py v4.1

=== Run "set_flash_voltage" command ===
Enable internal flash voltage regulator (VDD_SDIO) to 3.3V.

VDD_SDIO setting complete.

Check all blocks for burn...
idx, BLOCK_NAME,          Conclusion
[00] BLOCK0               is not empty
	(written ): 0x0000000400182226000001330000a8b00015441793d45be800000000
	(to write): 0x00000000000000000001c00000000000000000000000000000000000
	(coding scheme = NONE)
. 
This is an irreversible operation!
Type 'BURN' (all capitals) to continue.
BURN
BURN BLOCK0  - OK (all write block bits are set)
Reading updated efuses...
Successful

### 3.2 烧录固件

https://www.espressif.com.cn/zh-hans/support/download/other-tools?keys=&field_type_tid%255B%255D=13

从网址下载  +flash下载工具  ,windows版本

固件位于/wireless/espressif/esp32/firmware/目录下,烧录分区如下

esp_hosted_bootloader_esp32_sdio_v0.4.bin          0x1000

esp_hosted_partition-table_esp32_sdio_v0.4.bin     0x8000

esp_hosted_partition-table_esp32_sdio_v0.4.bin     0xd000

esp_hosted_firmware_esp32_sdio_v0.4.bin               0x10000

(烧录完成后将驱动中固件下载相关关闭,进入正常模式CONFIG_ENTER_DOWNLOAD = n)

固件烧录成功,通过串口助手可以看到wifi模组打印如下

I (13) boot: ESP-IDF v4.0.3-215-gbe7df8bce9-dirty 2nd stage bootloader
I (14) boot: compile time 17:16:19
I (14) boot: Enabling RNG early entropy source...
I (19) boot: SPI Speed      : 40MHz
I (23) boot: SPI Mode       : DIO
I (27) boot: SPI Flash Size : 4MB
I (31) boot: Partition Table:
I (35) boot: ## Label            Usage          Type ST Offset   Length
I (42) boot:  0 nvs              WiFi data        01 02 00009000 00004000
I (49) boot:  1 otadata          OTA data         01 00 0000d000 00002000
I (57) boot:  2 phy_init         RF data          01 01 0000f000 00001000
I (64) boot:  3 factory          factory app      00 00 00010000 00100000
I (72) boot:  4 ota_0            OTA app          00 10 00110000 00100000
I (79) boot:  5 ota_1            OTA app          00 11 00210000 00100000
I (87) boot: End of partition table
I (91) boot: Defaulting to factory image
I (96) boot_comm: chip revision: 3, min. application chip revision: 0
I (103) esp_image: segment 0: paddr=0x00010020 vaddr=0x3f400020 size=0x1ec4c (126028) map
I (157) esp_image: segment 1: paddr=0x0002ec74 vaddr=0x3ffbdb60 size=0x0139c (  5020) load
I (159) esp_image: segment 2: paddr=0x00030018 vaddr=0x400d0018 size=0x8da08 (580104) map
I (371) esp_image: segment 3: paddr=0x000bda28 vaddr=0x3ffbeefc size=0x02a34 ( 10804) load
I (375) esp_image: segment 4: paddr=0x000c0464 vaddr=0x40080000 size=0x00400 (  1024) load
I (378) esp_image: segment 5: paddr=0x000c086c vaddr=0x40080400 size=0x1d82c (120876) load
I (455) boot: Loaded app from partition at offset 0x10000
I (455) boot: Disabling RNG early entropy source...
I (456) cpu_start: Pro cpu up.
I (459) cpu_start: Application information:
I (464) cpu_start: Project name:     network_adapter
I (470) cpu_start: App version:      release0.4-60-g22180f7
I (476) cpu_start: Compile time:     Jan 24 2022 17:16:13
I (482) cpu_start: ELF file SHA256:  d6ca7b2436700f3f...
I (488) cpu_start: ESP-IDF:          v4.0.3-215-gbe7df8bce9-dirty
I (495) cpu_start: Starting app cpu, entry point is 0x4008129c
I (0) cpu_start: App cpu up.
I (505) heap_init: Initializing. RAM available for dynamic allocation:
I (512) heap_init: At 3FFAFF10 len 000000F0 (0 KiB): DRAM
I (518) heap_init: At 3FFB6388 len 00001C78 (7 KiB): DRAM
I (524) heap_init: At 3FFB9A20 len 00004108 (16 KiB): DRAM
I (530) heap_init: At 3FFBDB5C len 00000004 (0 KiB): DRAM
I (536) heap_init: At 3FFD2118 len 0000DEE8 (55 KiB): DRAM
I (543) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (549) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (555) heap_init: At 4009DC2C len 000023D4 (8 KiB): IRAM
I (562) cpu_start: Pro cpu start user code
I (580) spi_flash: detected chip: generic
I (580) spi_flash: flash io: dio
I (581) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (593) NETWORK_ADAPTER: *********************************************************************
I (599) NETWORK_ADAPTER:                 ESP-Hosted Firmware version :: 0.4                        
I (607) NETWORK_ADAPTER:                 Transport used :: SDIO                          
I (615) NETWORK_ADAPTER: *********************************************************************
I (621) NETWORK_ADAPTER: Supported features are:
I (625) NETWORK_ADAPTER: - WLAN over SDIO
I (629) ESP_BT: - BT/BLE
I (633) ESP_BT:    - HCI Over SDIO
I (635) ESP_BT:    - BT/BLE dual mode
I (645) BTDM_INIT: BT controller compile version [14ea243]
I (647) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (653) phy_init: phy_version 4660,0162888,Dec 23 2020
I (975) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (975) NETWORK_ADAPTER: ESP Bluetooth MAC addr: 44-17-93-d4-5b-ea

I (979) SDIO_SLAVE: Using SDIO interface

## 4.启动wifi

1.使用命令ifconfig -a查看wifi启动情况,驱动加载成功会显示节点wlan0

![image-20220718100004128](/home/robin/.config/Typora/typora-user-images/image-20220718100004128.png)

设置wifi自启后,如果HWaddr显示00:00:00:00:00:00,代表没有获取到MAC物理地址,没有显示inet addr说明没有分配到ip地址.wifi启动失败,可以手动启动wifi

2.先使用wifi_down.sh命令将wifi关闭

3.再配置/usr/data目录下的wpa_supplicant.conf联网配置文件

#cd  /usr/data                           #进入到/usr/data目录下

#vi wpa_supplicant.conf         #打开配置文件进行设置

ctrl_interface=/var/run/wpa_supplicant
update_config=1
country=GB
network={
    ssid="sw1"                             #设置要连接的wifi名
    psk="\#sz@sw1^"                  #设置要连接的wifi密码
}

4.配置完成后使用命令

#wifi_down.sh /usr/data/wpa_supplicant.conf         #启动wifi

![image-20220718103412072](/home/robin/.config/Typora/typora-user-images/image-20220718103412072.png)

或者使用如下命令启动wifi

#./usr/bin/esp32_cmd sta_connect /usr/data/wpa_supplicant.conf       #获取mac地址并连网

#udhcpc -i wlan0 &        #分配ip地址