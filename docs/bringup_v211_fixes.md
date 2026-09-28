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

## 7. 第二轮修正（首轮新镜像上板后）

| # | 现象 | 根因 | 修法 |
|---|------|------|------|
| 6 | 开机时 `S70USB` 报 `can't create gadget 目录下的 idVendor: nonexistent directory`，主机识别不到 USB 设备 | 脚本缺陷：写描述符之前没有创建 gadget 目录 `/sys/kernel/config/usb_gadget/<name>`，于是 VID/PID 等都没写进去（设备以 0 VID/PID 枚举） | `gadget_build()` 开头 `mkdir -p "$g"`；同时把设备类设为 **0xEF/0x02/0x01（IAD 复合设备）**，否则 Windows 会把整个设备绑到 PTP 驱动、RNDIS 网卡永远不出现 |
| 7 | `sshd` / `ssh-keygen` 启动失败：`libssp.so.0: cannot open shared object file` | buildroot 的 `TOOLCHAIN_EXTERNAL_LIBS += libssp.so.*` 在本工具链上匹配不到（该工具链只对部分 ABI 提供共享 libssp），目标 rootfs 因此缺库；而 openssh 链接时用的是 host sysroot 里那份 | 新增 `fs_overlay/common/extra-libs.txt` 与 `build.sh copy_extra_libs()`：打包 rootfs 之前把列出的库从工具链 sysroot 拷入镜像 `/lib`（本次 3 个 libssp 文件） |
| 8 | 开机自动跑产测交互测试（`test pwm_led ... ok`、`please press the key which you want to test`），且 `rm` 删不掉 | 厂商产测工具由 `APP_test_shell` 提供，安装 `S99factory_test_shell` 与 `/etc/factory_test/`；根文件系统是只读 squashfs，运行期无法删除 | 关闭 `APP_test_shell`，并在 `fs_overlay/common/prune.txt` 删除这两个路径（buildroot 不会自动卸载） |

> 运行期需要改文件时请用可写分区 `/usr/data`（ubi），其余内容靠改配置重新出镜像。

## 8. 已知但未处理的小问题

- 开机打印 `MAC d0:31:10:0x:c6:0x` 与 `ifconfig: invalid hw-addr`：厂商脚本
  `wireless/bcm/bin/S43wifi_bcm_init_config` 从 flash 读 MAC 后拼出的字符串含 `0x` 字面量
  （说明 MAC 分区里没有有效值），`ifconfig ... hw ether` 因此失败。不影响 WiFi 使用（驱动使用
  自己的 MAC）。要消除可写入有效 MAC，或让脚本在 MAC 非法时跳过。

## 9. USB 口与主机侧准备

- 板上有两个 USB 口：**Type-C 只供电**，**Micro-USB（丝印 Download）是 OTG/UDC 口**
  （内核里是 `13500000.otg`，就是烧录用的那个口）。MTP/RNDIS gadget 只在 Micro-USB 上生效，
  主机的 USB 数据线必须插这个口。
- Windows 侧：插好后在设备管理器里应出现"便携设备/MTP"与"Remote NDIS Compatible Device"
  （或"未知设备"）。若 RNDIS 没自动装驱动，手动指向系统自带的 Remote NDIS 驱动后，
  用 `ipconfig` 应看到新网卡；若没有 DHCP（未放 `/etc/udhcpd_usb.conf`），把主机该网卡设为
  `192.168.8.100/24`，板端是 `192.168.8.168`。
- Linux 主机：`dmesg` 会看到 rndis_host/cdc_ether 绑定，`ifconfig` 出现新网卡，直接配同网段即可。

## 10. WiFi 测试步骤（本镜像已含驱动/固件/工具）

```sh
ifconfig wlan0 up                      # 驱动加载后 wlan0 已存在
# 建一份 wpa_supplicant 配置（/usr/data 是可写分区）
cat > /usr/data/wpa_supplicant.conf <<EOF
ctrl_interface=/var/run/wpa_supplicant
network={
    ssid="你的AP"
    psk="密码"
}
EOF
wifi_up.sh                             # 或: wpa_supplicant -B -i wlan0 -c /usr/data/wpa_supplicant.conf
udhcpc -i wlan0                        # 取 IP
wpa_cli -i wlan0 status                # 看 wpa_state=COMPLETED
ping -I wlan0 192.168.50.1             # 连通性
```

补充：AP 模式（hostapd）**当前镜像没有编进去**（buildroot 里 `BR2_PACKAGE_HOSTAPD` 未选），
需要 AP 测试的话我再把它加进 buildroot 配置；`iw`/`iwlist` 也未选，扫描请用 `wpa_cli scan`。

## 11. 第三轮：可写根文件系统 + WiFi 统一管理（2026-09-28）

### 11.1 根文件系统改为可写（UBIFS on UBI）

之前 rootfs 是**只读 squashfs**（厂商的 NAND 默认：`root=/dev/mtdblock_bbt_ro2 rootfstype=squashfs ro`），
所以运行期无法改文件（`rm` 报 read-only）。现在改为：

| 项 | 之前 | 现在 |
|----|------|------|
| 根文件系统 | squashfs（只读） | **UBIFS（读写）**，放在名为 `rootfs` 的 UBI 卷里 |
| 内核启动参数 | `root=/dev/mtdblock_bbt_ro2 rootfstype=squashfs ro` | `ubi.mtd=rootfs root=ubi0:rootfs rootwait rootfstype=ubifs rw` |
| 交付镜像 | `rootfs.squashfs` | `rootfs.ubi`（烧录用）+ `rootfs.ubifs`（UBI 内层，便于自己 ubinize）；`rootfs.squashfs` 仍保留作回退 |
| 覆盖的文件 | — | `configs/uboot/x2000_base.h`（选 `CONFIG_ROOTFS_UBI`）、`configs/uboot/x2000_base_common.h`（设备串与 `rw`）、`configs/build/configs/buildroot/vot_x2000_ingenic_board_defconfig`（UBIFS/UBI 镜像参数） |

要点：

- 新增的 buildroot 配置里：`BR2_TARGET_ROOTFS_UBIFS=y`（`-m 0x800 -e 0x1f000 -c 750 -x lzo`，对应 2048B 页 /128KiB 块，
  卷上限约 91 MiB）+ `BR2_TARGET_ROOTFS_UBI=y`（ubinize，卷名 `rootfs`、`vol_flags=autoresize`）。
  因为 UBI 是 autoresize，**烧录时卷会自动铺满 rootfs 分区**，剩余空间就是可写空间。
- **分区要求**：`rootfs.ubi` 当前 42 MiB，且它还要容纳运行期写入，
  建议 rootfs 分区不小于 **64 MiB**（当前推断的布局：`boot 1M + kernel 8M + rootfs 96M + userdata 23M`）。
  分区与 `-c` 的关系：`-c` 必须 ≥ 分区 PEB 数，否则挂载会报卷过大。
- **必须重烧 u-boot**：启动参数在内核 cmdline 里由 u-boot 生成，换了 rootfs 类型就必须一起更新。
- 运行时注意：UBIFS 是读写挂载，直接改动会持久化；若想恢复出厂状态，重烧 `rootfs.ubi` 即可。

### 11.2 WiFi 统一管理：S60WiFi

新增 `fs_overlay/common/etc/init.d/S60WiFi`（结构与你 Hi3556 的 S60WiFi 一致：Part I `Func_*` / Part II `API_*` / Part III main）：

```text
S60WiFi {start|stop|restart|status|scan|connect <ssid> [pwd]|reconnect|ap <ssid> [pwd]|insmod|rmmod}
```

- `start`：开机自动执行 —— 确保驱动加载（必要时 `insmod /module_driver/cywdhd.ko`）、设置 MAC
  （调用厂商 `S43wifi_bcm_init_config`：macaddr.txt → 烧录分区 → efuse → 随机）、
  若有已保存的站点配置则自动连接并取地址；没有则只提示用法。
- `connect <ssid> [pwd]`：生成 `wpa_supplicant.conf` → 关联（等 `wpa_state=COMPLETED`，默认 30s）→
  `udhcpc` 取地址；成功后把 SSID/PSK 存到 `/usr/data/wifi/sta.conf`（下次 `start`/`reconnect` 直接复用）。
- `ap <ssid> [pwd]`：hostapd（nl80211）+ busybox udhcpd 起热点，默认 `192.168.7.168/24`，
  地址池 `192.168.7.20-100`；`WIFI_AP_OPEN=1` 可开无密码热点。
- `scan`：`wpa_cli scan` + `scan_results`（镜像里已加 `iw`，需要时可改用 `iw dev wlan0 scan`）。
- `status`：接口 / 驱动 / supplicant 状态 / 进程 / 已保存配置一览。
- 参数可用环境变量覆盖：`WIFI_IF WIFI_KO WIFI_DIR AP_IP AP_NETMASK AP_CHANNEL WPA_TIMEOUT DHCP_ROUNDS`。

配套改动：

- 镜像里新增 **hostapd** 与 **iw**（buildroot 配置），AP 模式因此可用；
- 移除厂商的 `S44wifi_bcm_up`（WiFi 自动启动改由 S60WiFi 负责），保留 `S41`（蓝牙固件下载）与
  `S43`（MAC 初始化）；
- 站点/热点配置保存在可写分区 `/usr/data/wifi/`，与新的可写 rootfs 配合。

## 12. 第四轮：SSH 修复（2026-09-28）

### 12.1 现象与两个根因

现象：能 ping 通板子，但 SSH 连不上；板上 `ps` 里没有 sshd 进程，控制台也没有明显报错（只有一行 OK）。

1. **镜像里缺 `sshd` / `dbus` 系统账号**。buildroot 的用户表（`output/build/buildroot-fs/full_users_table.txt`，内容就是 `dbus` 与 `sshd` 两条）是在**生成文件系统镜像那一步**用 fakeroot 应用到 target 上的；我们的流程为了绕开厂商 post-build 的干扰而**自己打包镜像**，恰好跳过了这一步。结果镜像里没有 `sshd` 用户，OpenSSH 启动即退出
   （`Privilege separation user sshd does not exist`）；而 buildroot 的 `S50sshd` 不检查 sshd 的退出码，照样打印 `OK`，所以表面上"看不出错误"。
2. **厂商的"只读系统 SSH 适配"在可写系统上是负担**。构建期脚本把 `etc/ssh` 改成指向 `usr/data/ssh/` 的软链接，真正的 `sshd_config` 放在 `save/`，靠 `S30cp_ssh` 开机拷进去；任一环失败都静默。

连带发现并修掉的两个问题：

3. **镜像内文件属主**：我们自己打包时没有用 fakeroot，所有文件属主都变成构建用户（设备上 uid 1000 = dbus）。openssh 对此有硬检查：
   `/etc/ssh/sshd_config` 必须 root 所有且不可组/其他可写，否则 `Bad owner or permissions`；
   `/var/empty` 必须 root 所有，否则 `%s must be owned by root and not group or world-writable` 直接 fatal。
4. **overlay 文件权限**：从仓库检出的文件带 775（组可写），会直接踩中上面那条配置检查；现统一为脚本 755 / 配置 644。

### 12.2 修法

- 新增 `scripts/pack_images.sh`（**必须在 fakeroot 下运行**，由 `build.sh` 调用）：
  1. 把 target 全树 `chown -h -R 0:0`；
  2. 用 buildroot 自带的 `support/scripts/mkusers` + 用户表，补上 `sshd`/`dbus` 等账号；
  3. 按守护进程要求修正特例：`/var/empty` 归 root（openssh 要求）、`/var/run/dbus` 归 dbus（套接字目录）；
  4. 按 buildroot 配置里的参数重新打包 `rootfs.squashfs` / `rootfs.ubifs` / `rootfs.ubi`。
- `build.sh` 的 phase 4/4 与 `fs` 目标改为调用 `pack_images`；并把"残留清理（prune）"移动到"覆盖（overlay）"**之前**，这样能安全清掉旧的 `etc/ssh` 软链与 `save/` 目录。
- 关闭厂商的 `APP_br_ssh`，改成我们自己的 SSH 方案：
  - `fs_overlay/common/etc/ssh/sshd_config`：允许 root 登录、密码认证、`Subsystem sftp internal-sftp`、`PidFile /var/run/sshd.pid`；
  - `fs_overlay/common/etc/init.d/S50sshd`：首次启动生成主机密钥、把 `/usr/data/ssh/authorized_keys` 安装到 `/root/.ssh/`、用 `sshd -e` 启动并**如实报告 FAILED**；
  - prune 掉 `etc/ssh`(软链)、`save/`、`etc/init.d/S30cp_ssh`。
- **只需重烧 `rootfs.ubi`**：u-boot 与内核未变（cmdline 一致）。

### 12.3 登录方式（root 在镜像里没有密码）

1. **公钥（推荐）**：把 `id_rsa.pub` 写到可写分区 `/usr/data/ssh/authorized_keys`
   （该分区正是 MTP 导出的存储位置，插 USB 复制即可），下次启动 `S50sshd` 会自动装到 `/root/.ssh/authorized_keys`；
   也可立即生效：`/etc/init.d/S50sshd restart`。
2. **设密码**：直接 `passwd` 修改（rootfs 现在可写），随后 `ssh root@<板子IP>` 用密码登录。

排查命令：`/etc/init.d/S50sshd status`；若仍失败，用 `/usr/sbin/sshd -e -d` 前台运行看具体报错。

### 12.4 顺带收益

用户表补齐后，之前日志里的 `dbus-daemon: ... Could not get UID and GID for username "dbus"`、
`Starting NFS statd: rpc.nfsd: Unable to access /proc/fs/nfsd` 之类的"账号不存在"类报错也会消失。
