# 厂商文档索引（`vendor/doc/.Markdown/`）

厂商文档的 Markdown 版（1886 个文件，含配图目录 `*.assets/`）。**PDF 未导入**（体积原因），
需要原件时到厂商 FTP 取（见 `docs/vendor/VENDOR.md`）。
路径均相对 `vendor/doc/.Markdown/`。

## 一、上手与构建（先看这五篇）

| 主题 | 路径 |
|------|------|
| 硬件介绍（X2000 系列 / Darwin V2.x、X2000H 差异） | `客户支持/X2000_Darwin_V2.0/Darwin_X2000_v2.0_Ethernet/ab/001硬件介绍.md` |
| 本板快速上手（出厂配置、烧录步骤） | `客户支持/X2000_Darwin_V2.0/x2000H_Darwin_v2.0_开发板快速上手说明/x2000H_Darwin_v2.0_开发板快速上手说明.md` |
| 工程编译说明（`make <defconfig>` / `make` / 分模块编译） | `1_Linux工程编译说明/1_Linux工程编译说明.md` |
| 烧录介绍（cloner 工具、分区、串口） | `2_Linux工程烧录介绍/img/2_Linux工程烧录介绍.md` |
| 源码获取（repo 工具 + Gerrit 账号流程） | `客户支持/Linux平台源码获取.md` |

## 二、驱动与外设

| 主题 | 路径 |
|------|------|
| 模块驱动添加流程（module_driver 的 soc/devices 分层） | `模块驱动添加流程/模块驱动添加流程.md` |
| Kernel 模块添加及编译手册 | `Kernel模块添加及编译手册/Kernel模块添加及编译手册.md` |
| I2C 使用 | `I2C使用说明文档/I2C使用说明文档.md` |
| SPI 使用 | `SPI使用及说明文档/SPI使用及说明文档.md` |
| 通用模块驱动 | `通用模块驱动使用说明文档/` |
| 片上外设驱动（内核态 / 模块态） | `片上外设内核驱动/`、`片上外设模块驱动/` |
| 预留内存（rmem） | `预留内存使用说明文档/` |

> 说明：本仓库还对 I2C / 预留内存做了网页版摘要，见 `docs/` 下相关笔记（如已生成）。

## 三、无线 / 音频 / 显示 / 相机

| 主题 | 路径 |
|------|------|
| BCM WiFi+BT 使用（本板 CYW43438 模块） | `bt&wifi使用说明文档/bcm bt&wifi使用说明文档/bcm bt&wifi使用说明文档.md` |
| 录音与播放（本板音频） | `客户支持/X2000_Darwin_V2.0/x2000_darwin_录音和播放的使用方法/x2000_darwin_录音和播放的使用方法.md` |
| ALSA 音频驱动添加流程 | `ALSA音频驱动添加流程/` |
| LCD 使用与添加流程 | `LCD使用以及添加流程/` |
| 相机使用与添加（x2000 篇） | `Camera使用以及添加说明文档/Camera_Sensor x2000使用以及添加说明文档/` |
| USB 摄像头 | `客户支持/X2000_Darwin_V2.0/x2000H-darwin如何使用USB摄像头/x2000H-darwin如何使用USB摄像头.md` |

## 四、系统与升级

| 主题 | 路径 |
|------|------|
| 板级 OTA（MTD/nand 双系统） | `客户支持/X2000_Darwin_V2.0/x2000H_darwin_v2.0_mtd_开发板OTA使用说明文档/x2000H_darwin_v2.0_mtd_开发板OTA使用说明文档.md` |
| 添加客户自定义配置文件（等价于换产品配置） | `客户支持/X2000_Darwin_V2.0/Darwin_X2000_添加客户自定义配置文件/Darwin_X2000_添加客户自定义配置文件.md` |
| userdata 分区挂载 | `userdata分区挂载说明文档/` |
| 以太网配置（本板 RTL8201） | `客户支持/X2000_Darwin_V2.0/Darwin_X2000_v2.0_Ethernet/Darwin_X2000_v2.0_Ethernet.md` |
| FAQ（烧录/启动/编译常见问题） | `3_FAQ/FAQ.md` |
| 配置工具 IConfigTool（我们走纯文本 defconfig，可参考其选项含义） | `IConfigTool使用文档/IConfigTool 使用文档.md` |

## 五、其余目录（按需查阅）

`方案应用文档/`、`功耗测试/`、`性能测试/`、`TPC使用说明/`、`第三方库使用说明/`、
`大小核通信说明文档/`、`IO文档必看/`、`NFS文件系统挂载/`、`Linux辅助开发/`、
`客户支持/<各板卡>/`（按板卡分目录，X2000_Darwin_V2.0 / X2000_ilock_V1.0 / X2600E_VAST …）。
