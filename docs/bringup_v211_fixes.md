# V2.1.1 板级 bring-up 修正记录（2026-09-28）

第一次烧录我们自建仓库构建的镜像后，在板上观察到 5 个问题。本文记录根因、修法、以及
需要上板复核的项。产品配置见 `configs/build/configs/vot_x2000_ingenic_board_defconfig`。

## 1. 根因总述

我们最初使用的是**SDK 快照（2024-04-07）的默认 Darwin 配置**，而厂商为**这块 V2.1.1 板**
（2025-02 出厂包）另有一份配置。两者在 41 处取值上不同，下面的故障全部落在这批差异里。
另外两类问题与"配置变更后 buildroot 的持久 target 不会自动清理"有关。

## 2. 故障、根因与修法

| # | 现象 | 根因 | 修法 |
|---|------|------|------|
| 1 | `insmod cywdhd.ko: No such device`，日志里 `wlan power on/off` 反复循环 | WiFi 模组的电源/复位脚在配置里是关闭的：`MD_X2000_510_WIFI_POWER_ON=-1`、`MD_X2000_510_WIFI_REG_ON=-1`。这两个键被 `module_driver/package/soc/x2000_510/msc/msc.mk` 写进 SDIO 主机初始化参数；为 -1 时模组（CYW43438 / AW-NM372SM）根本不上电，SDIO 枚举不到设备 | 设为厂商出厂值 `PB14` / `PD18` |
| 2 | `/usr/bin/h264e-nl-server: version 'GLIBC_2.29' not found` 反复刷屏 | 配置打开了 `APP_br_h264_server`，把**厂商预编译二进制**装进 rootfs；该二进制需要 GLIBC_2.29，而本 rootfs 由 `mips-gcc930-glibc228` 工具链构建 = glibc 2.28 | 关闭 `APP_br_h264_server`（厂商出厂镜像也没有这个程序） |
| 3 | 每个注册 platform device 的模块 insmod 都 Oops（keyboard_adc / soc_dmic / soc_mac / icodec…），`__device_attach_driver`/`platform_match` 读模块区地址 0xc04e35xx 崩 | 推测为 **问题 1 的次生伤害**：bcmdhd 模块 init 失败后未清理它注册的 platform_driver，驱动链表残留指向已卸载模块内存的指针；之后任何 `platform_device_add()` 遍历链表即踩空 | 先修 1；重烧后复核 Oops 是否消失 |
| 4 | 触摸失灵：`GTP-ERROR I2C Read: 0x814E ... errcode -6` | 触摸挂在错误的 I2C 总线：`MD_GTP_I2C_BUSNUM=6`（厂商 4），且 `MD_X2000_510_I2C4_BUS` 未打开 | 总线改 `4`，打开 I2C4 |
| 5 | （预防性）背光参数与厂商标定不一致 | `MD_PWM_BACKLIGHT0_FREQ=1000000`（厂商 50000）、`MAX_BRIGHTNESS=300`（厂商 100） | 对齐厂商值 |

## 3. 本次同时做的取舍（按评审结论）

关闭：`APP_awtk`（GUI，用户不需要）、adb 全套（`APP_br_adb_server` 等）、开机 logo
（`APP_br_display_logo`）、SD 卡/U 盘/USB 大容量存储与 mmc 挂载助手（NAND 板上无对应分区，
也是日志里 `Check whether the partition table is initialized successfully!` 的来源）、
`MD_I2C_GPIO`（板上有 soc_i2c）。

保留：`APP_br_ssh`（配合 buildroot 的 `BR2_PACKAGE_OPENSSH` 提供 SSH/sFTP）、
`APP_usb_mtp` + 我们自己的 gadget 脚本（MTP + RNDIS）、ubi 数据分区挂载、
`APP_wireless_bcm`、`APP_test_shell`、`APP_libhardware2`/`APP_libutils2`/`APP_libisp`/
`APP_libmedia`/`APP_speexdsp`。

**`APP_lib2d` 必须保留**：`libmedia_ffmpeg.so` 链接 `-l2d`，关掉会导致 libmedia 编译失败
（这是构建失败后才发现的依赖，不是推断）。

## 4. USB：MTP + RNDIS 合并到一个 gadget

厂商的脚本是"一个模式一个脚本"（MTP / adb / mass storage），而 configfs 里**同一时刻只能有一个
gadget 绑定 UDC**，所以它们会互相抢占。我们改为：

- `fs_overlay/common/etc/init.d/S70USB`：一个复合 gadget，同时挂 `ffs.mtp` + `rndis.usb0`
  （可选 `acm.usb0` 串口）。用法：`S70USB {start|stop|restart|status|mtp|rndis|serial}`，
  开机默认 `start` = MTP + RNDIS。
- `fs_overlay/common/etc/umtprd/umtprd.conf`：我们的 umtprd 配置（存储指向 `/usr/data`，
  即 ubi 可写数据分区；厂商版指向 `/tmp/sdcard`，本板无 SD 数据分区）。
- 因此关闭了厂商的 `APP_usb_mtp_server`（它只做 MTP 且会独占 UDC）；`umtprd` 二进制仍由
  `APP_usb_mtp` 提供。
- 内核侧依赖已具备：`CONFIG_USB_CONFIGFS_F_FS`、`CONFIG_USB_F_FS`、`CONFIG_USB_CONFIGFS_RNDIS`、
  `CONFIG_USB_F_RNDIS`、`CONFIG_USB_U_ETHER`、`CONFIG_USB_CONFIGFS_ACM`。

RNDIS 侧：设备地址 `192.168.8.168/24`（可用 `RNDIS_IP` 覆盖）；若存在 `/etc/udhcpd_usb.conf`
则顺带启动 udhcpd 给主机发地址，否则请在主机侧手工配同网段地址。

## 5. 构建包装层的三处修正（build.sh）

1. **配置校验**：只判断 `.config.in` 是否存在是不够的——一次失败运行会留下**产品键为空**的
   `.config.in`（厂商的 `check_config.sh` 会生成这种存根），导致后续 `make uboot` 直接报
   `No rule to make target 'distclean'`。现在校验 `APP_uboot_config` 是否等于本产品的值，
   且 `all` 每次强制重新应用配置（phase 0）。
2. **覆盖顺序**：厂商 post-build 在 buildroot 的 target-finalize 里执行，会删除/覆盖我们的文件
   （`umtprd.conf` 就被删过一次）。现在顺序改为：最后一次 buildroot → 合并 `fs_overlay/` →
   **自己用 buildroot 的 mksquashfs 参数重新打包 rootfs**（`-comp lzo`，与 buildroot 配置一致）。
3. **残留清理**：buildroot 不会卸载包，配置里关掉的包会把文件留在持久 `output/target` 里
   （实测：awtk、adb、厂商 h264 server）。新增 `fs_overlay/<layer>/prune.txt`（每行一个
   rootfs 相对路径），在覆盖阶段删除；本次用它清掉 `usr/bin/adbd`。

## 6. 上板复核清单

```sh
cat /proc/meminfo | head -2        # X2000H 应见 ~500 MB（CONFIG_HIGHMEM=y 生效）
dmesg | grep -i -E "cywdhd|bcmdhd|wlan"   # WiFi 模块是否 probe 成功、无 Oops
ifconfig -a                        # wlan0 / usb0 是否出现
cat /proc/mtd                      # NAND 分区表
ls /sys/class/backlight/           # 背光设备
# 触摸：看是否有事件
cat /proc/bus/input/devices | grep -i -A3 gt9xx
# MTP + RNDIS（插 USB 到主机）
/etc/init.d/S70USB status
```

预期：WiFi 模组上电并 probe（不再反复 power on/off）、无 kernel Oops、触摸坐标事件正常、
背光可控、主机看到 MTP 设备与 RNDIS 网卡。
