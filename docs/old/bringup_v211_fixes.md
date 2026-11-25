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

> 本节已被第 13 节取代：现在的出厂镜像自带 root 口令（`root` / `cdjp123`），不再需要放公钥。
> 下面两种做法作为历史记录保留，改口令与改公钥的路径仍然有效。

1. **公钥（推荐）**：把 `id_rsa.pub` 写到可写分区 `/usr/data/ssh/authorized_keys`
   （该分区正是 MTP 导出的存储位置，插 USB 复制即可），下次启动 `S50sshd` 会自动装到 `/root/.ssh/authorized_keys`；
   也可立即生效：`/etc/init.d/S50sshd restart`。
2. **设密码**：直接 `passwd` 修改（rootfs 现在可写），随后 `ssh root@<板子IP>` 用密码登录。

排查命令：`/etc/init.d/S50sshd status`；若仍失败，用 `/usr/sbin/sshd -e -d` 前台运行看具体报错。

### 12.4 顺带收益

用户表补齐后，之前日志里的 `dbus-daemon: ... Could not get UID and GID for username "dbus"`、
`Starting NFS statd: rpc.nfsd: Unable to access /proc/fs/nfsd` 之类的"账号不存在"类报错也会消失。

## 13. 第五轮：出厂登录（root + 口令，串口与 SSH 共用一套账号）

### 13.1 目标

出厂设备统一一个账号：**用户名 `root`，口令 `cdjp123`**；SSH 与串口都用它，
不再使用"往板上放公钥"的方式（原 `authorized_keys` 流程已删除），板上不需要做任何准备。

### 13.2 真源与实现

- 真源是 `products/darwin_v211.conf`：

  ```sh
  ROOT_PASSWORD="cdjp123"
  ROOT_PASSWORD_SALT="votX2000v211"    # 固定盐 => 哈希稳定 => 镜像可复现
  ```

- `build.sh` 的 `root_password_hash()` 用固定盐算出 `$6$`（SHA-512 crypt）哈希
  （优先 `openssl passwd -6`，其次 `python3 crypt`；两者结果逐字节一致，实测过）；
  `scripts/pack_images.sh` 在 **fakeroot 内、mkusers 之后**把哈希写进 `/etc/shadow`
  root 行的字段 2，并把该文件置为 `0600 root:root`。**镜像里只有哈希，没有明文。**

### 13.3 为什么这样能登录（三处都实测/查证过）

1. **算法**：目标端 `libcrypt` 是 glibc 2.28（`sshd` 的 `NEEDED` 里有 `libcrypt.so.1`，
   支持 `$1$/$5$/$6$`）；busybox 是 `CONFIG_USE_BB_CRYPT=y` + `CONFIG_USE_BB_CRYPT_SHA=y`
   （内置 crypt 同样支持 SHA-256/512）。选 `$6$` 两边都认。
2. **读 shadow**：OpenSSH 编译时带 shadow 支持（`config.h: HAVE_SHADOW_H`、`HAS_SHADOW_EXPIRE`），
   `auth.c` 的 `shadow_pw()` 优先取 `getspnam()`；busybox 是 `CONFIG_FEATURE_SHADOWPASSWDS=y`
   （`login`/`passwd`/`su` 都读 shadow）。所以口令只需放 shadow 一份，`/etc/passwd` 保持 `x`。
3. **老化字段必须留空**（这一条最容易踩）：shadow 的 root 行是 `root:<hash>:::::::`。
   空字段经 glibc 解析为 `-1`，sshd 的 `auth_shadow_pwexpired()` 命中 `sp_max == -1`
   分支 → "password expiration disabled"，正常放行；反过来若写成 `sp_lstchg=0`，会被判成
   `User root password has expired (root forced)` 而强制改密。本机用 glibc 的 `sgetspent()`
   实测：`root:$6$..:::::::` → `lstchg=-1 max=-1 expire=-1`，与厂商/busybox 生成的 shadow 形态一致。

### 13.4 SSH 侧改动

- `fs_overlay/common/etc/ssh/sshd_config`：`PasswordAuthentication yes`、
  **`PubkeyAuthentication no`**（出厂镜像不需要密钥准备）、`PermitRootLogin yes`、
  `PermitEmptyPasswords no`。要恢复公钥登录，把 `PubkeyAuthentication` 改回 `yes`
  并把公钥放到 `/root/.ssh/authorized_keys`（rootfs 可写，重启不丢）。
- `fs_overlay/common/etc/init.d/S50sshd`：删除"从 `/usr/data/ssh/authorized_keys` 装公钥"的逻辑；
  `status` 改为报告 root 口令是否已设置。

### 13.5 自查与验证

- `./build.sh env` 打印 root 登录来源（口令配置在哪个文件、盐值）；
- `./build.sh check` 会算出哈希并与沙箱里的 `/etc/shadow` 比对，不一致会提示 `run ./build.sh fs`；
- 镜像侧验证（本轮已做）：`unsquashfs` 看 `/etc/shadow` 是 `0600 root:root` 且 root 行有 `$6$` 哈希，
  再用 `crypt.crypt('cdjp123', <hash>) == <hash>` 反算通过、错误口令不通过；
- 板上验证：`./build.sh fs` 后只烧 `rootfs.ubi`，`ssh root@<板子IP>` 输入 `cdjp123`；
  `cat /etc/shadow` 应见 `root:$6$...`，`/etc/init.d/S50sshd status` 应显示 `root password: set`。

### 13.6 待确认 / 注意事项

1. 出厂口令写在仓库里 = 任何拿到仓库或固件的人都知道它。量产前建议改 `ROOT_PASSWORD`，
   或出厂后用 `passwd` 逐台改成不同口令。
2. **串口目前不要求登录**：`etc/inittab` 是 `console::respawn:-/bin/sh`，串口直接给 root shell。
   要不要改成 `getty`+`login`（同样的账号口令）属于取舍：改了更像"出场配置"，
   但一旦口令对不上就只能重新烧录；不改则拿得到串口就能进系统。等确认。

## 14. 第六轮：Windows 认不出 RNDIS（MS OS 描述符）+ USB 口确认

### 14.1 现象

Windows 能读到 MTP（资源管理器里出现设备），但设备管理器/网络适配器里没有 RNDIS，
板子上 `usb0` 的 rx/tx 包数一直是 0（主机一个包都没发过来）。

### 14.2 根因：面描述符里没有 Microsoft OS 描述符

gadget 是 class `0xEF/0x02/0x01`（IAD）的复合设备，两个功能对 Windows 的意义完全不同：

- MTP 接口是 class `0x06/0x01/0x01`（Still Imaging）→ Windows 有自带驱动，仅凭类别就能绑定，
  所以 MTP 一直是好的（这也解释了"只有 MTP 能用"）；
- RNDIS 接口组是 class `0xE0/0x01/0x03` → 在**复合设备**里 Windows 不会只凭类别去挂 netrndis.inf，
  必须由设备通过 Microsoft OS 描述符明确告诉它"这个接口就是 RNDIS"。

板端实测（修改前）：`functions/rndis.usb0/os_desc/interface.rndis/compatible_id` 为空、
gadget 的 `os_desc/{use,b_vendor_code,qw_sign}` 全 0、`os_desc/` 下没有指向配置的软链，
也就是内核根本没有对外提供这些描述符。

内核依据（`vendor/kernel/kernel/drivers/usb/gadget/`）：

- `function/f_rndis.c` 调 `usb_os_desc_prepare_interf_dir()`，所以 configfs 里天然存在
  `functions/rndis.usb0/os_desc/interface.rndis/`；但 compatible id 默认是全 0
  （`rndis_ext_compat_id` 没有默认值），必须由用户写；
- `configfs.c: os_desc_link()` 要求 `os_desc/` 里有一条指向某个 config 的软链，
  且该 config 必须已经在 `cdev->configs` 里（`mkdir configs/c.1` 时就通过
  `usb_add_config_only()` 注册了，所以顺序是：建 config → 建软链）；
- `configfs_composite_bind()` 在 bind 时把 `use_os_string/b_vendor_code/qw_sign` 抄进
  composite device，并把 RNDIS 的 compatible id 编进 Extended Compat ID 描述符，
  所以这几项和软链必须在写 `UDC` **之前**就位。

### 14.3 修法（都在 `fs_overlay/common/etc/init.d/S70USB`）

```sh
echo 1        > "$g/os_desc/use"
echo 0xcd     > "$g/os_desc/b_vendor_code"
echo "MSFT100" > "$g/os_desc/qw_sign"
echo "RNDIS"   > "$g/functions/rndis.usb0/os_desc/interface.rndis/compatible_id"
echo "5162001" > "$g/functions/rndis.usb0/os_desc/interface.rndis/sub_compatible_id"
ln -s "$g/configs/c.1" "$g/os_desc/c.1"
```

顺带修掉两个同源问题：

1. **序列号不再写死**：原来固定 `0123456789`。Windows 用序列号区分设备实例，写死会有两个后果：
   两块板子互相顶掉同一个驱动绑定；以及 Windows 复用"这台设备没有 MS OS 描述符"的缓存，
   于是描述符改了也不生效。现在用 `X2000-<wlan0 MAC>`（唯一且稳定，读不到时退回固定值）。
2. **主机拿不到 IP**：`S70USB` 本来就会在 `usb0` 上起 udhcpd，但镜像里一直缺少
   `/etc/udhcpd_usb.conf`，那个分支从没执行过，Windows 只能退到 169.254.x.x。
   新增 `fs_overlay/common/etc/udhcpd_usb.conf`：板子仍是 `192.168.8.168/24`，
   主机从 `192.168.8.100-149` 领地址（网段里刻意避开板子地址）。
3. `S70USB status` 增加 RNDIS 段：os_desc 状态 / compatible id / usb0 的 rx·tx 包数，
   用来一眼判断"主机到底有没有挂上 RNDIS 驱动"。rx 长期为 0 = 主机没挂驱动或没发包。

### 14.4 Windows 侧验证与清理

- 设备管理器 → 网络适配器：应出现 **Remote NDIS Compatible Device**；
- `ipconfig`：RNDIS 网卡应拿到 `192.168.8.x`；`ping 192.168.8.168` 应通（板子）；
- 若之前残留过"未知设备"或旧实例：设备管理器里勾选"显示隐藏的设备"，把该设备（含旧的
  MTP 实例）卸载后重新插拔——换了序列号后 Windows 会当成新设备重新读描述符；
- 兜底方案（不依赖 MS OS 描述符）：在 Windows 上手动安装内核自带的
  `Documentation/usb/linux.inf`，或直接给 RNDIS 网卡配静态 IP `192.168.8.50/24`；
- 板端自查：`/etc/init.d/S70USB status`。

### 14.5 USB 口确认（"是烧录口还是 Type-C 供电口"）

- 厂商《x2000H_Darwin_v2.0 开发板快速上手说明》第 5 节原文：
  "上 **TYPE_C_USB_Power** 接口，给开发板供电。然后再使用 **Micro_USB&Download** 接口
  接上 usb 数据线进行烧录使用"；进烧录模式是"按住 BOOT_KEY，再按 RST_KEY 松开"。
  即：**Type-C 只供电，Micro-USB&Download 才是数据/烧录口**。
- 结论：Windows 能认到 MTP，就说明数据线插在 **Micro-USB&Download** 口上——Type-C 口没有数据线，
  插在那里什么都不会枚举（板子上也不会有任何 USB 设备控制器活动）。
- 板端佐证：`/sys/class/udc/` 下只有一个 UDC `13500000.otg`（dwc2），它同时就是 ROM 下载用的那路 OTG；
  本次操作时状态为 `configured`，dmesg 里有 `new device is high-speed` 与 `new address 21`，
  说明是主机侧正常枚举的。

### 14.6 主机侧的两个前提（2026-09-28 实测补充）

板端描述符补齐后，Windows 仍给出"其他设备 → RNDIS、驱动程序名称 null"。此时设备侧已逐条验证
（os_desc 三项 + compatible id + 软链 + 内核里 `fill_ext_compat()`/`if_id` 的对应关系都确认无误），
剩下的是主机侧必须同时满足的两个条件：

1. **Windows 按"设备实例"缓存整份驱动安装结果**。实例 ID 是
   `USB\VID_xxxx&PID_xxxx\<序列号>`；同一个实例重新插拔**不会**重读 Microsoft OS 描述符，
   接口编号变化也不会重新分配驱动。用户日志里就能看到它沿用旧映射：
   给 `MI_00` 加装 `WUDFWpdMtp` + `WinUsb` 服务 —— 那是"MI_00 = MTP"时期的记录（那时 MTP+RNDIS
   的 MTP 在 0 号接口），而现在这个模式下 MI_00 已经变成 RNDIS 的控制接口。
   结论：**只要描述符或接口布局变了，就必须换序列号**。S70USB 的 `USB_REV` 就是干这个的，
   它把版本标记追加在序列号后面（当前为 `X2000-<wlan0 MAC>-2`）。
2. **主机上要真的存在 RNDIS 驱动**。设备侧的 compatible id `RNDIS` 只负责"告诉 Windows 用哪个驱动"，
   实际驱动是 Windows 自带的 `netrndis.inf`（Remote NDIS Compatible Device）。
   如果主机没有这个 INF（部分 Windows 11 24H2 已不再附带/不迁移），再正确的描述符也绑不上。

### 14.7 现场排查清单（Windows 侧，按顺序做）

1. `winver`：确认是 Windows 10 还是 11、内部版本号（是否 24H2）。
2. 看 `C:\Windows\INF\netrndis.inf` 是否存在（资源管理器要开"显示隐藏的文件/系统文件"，
   或命令行 `dir C:\Windows\INF\netrndis.inf`）。不存在 = 这台主机没有 RNDIS 驱动。
3. 设备管理器 → 那个未识别的 "RNDIS" → 详细信息 → **硬件 ID**，看有没有 `USB\MS_COMP_RNDIS`：
   - 有 → 描述符已经送到主机，只是驱动没挂上（回到第 2 步的驱动可用性）；
   - 没有 → 主机没读描述符（实例缓存问题：换 `USB_REV`/序列号，或把父设备实例彻底卸载）。
4. 手动指定驱动（不用下载任何东西）：右键该设备 → 更新驱动程序 → 浏览我的电脑上的驱动程序 →
   让我从计算机上的可用驱动程序列表中选取 → 网络适配器 → Microsoft → Remote NDIS Compatible Device。
5. 若第 2 步确认没有 `netrndis.inf`，或想彻底摆脱 Windows 的驱动差异，可考虑换传输方式：
   - **CDC-NCM**：Windows 11 自带 NCM 驱动，Windows 10 需装微软的 NCM 驱动包；
   - **RNDIS + NCM 双网卡**：Linux/macOS 走 NCM，Windows 谁有驱动用谁（BeagleBone 等板子就这么做）。
   两者的前提都是内核打开 `CONFIG_USB_CONFIGFS_NCM` / `CONFIG_USB_F_NCM`（当前 `not set`），
   需要重编内核并重烧 `xImage`。

### 14.8 真正的开关：IAD 的 class/subclass/protocol（2026-09-28 定位）

用户主机环境：**Windows 11 25H2（内部版本 26200.8875）**，`C:\Windows\INF\netrndis.inf` 存在，
设备硬件 ID 是 `USB\VID_18D1&PID_4EE1&REV_0100&MI_01` 与 `USB\VID_18D1&PID_4EE1&MI_01`
—— 没有任何"类别派生"的条目，设备一直无驱动。

关键依据（微软官方《USB Device Class Drivers Included in Windows》表格）：

| 类代码 | 驱动 | 适配范围 |
|--------|------|----------|
| Miscellaneous (**EFh**) | `Rndismp.sys` / `Rndismp.inf` | **SubClass 04h + Protocol 01h**，Windows 10/11 |
| CDC (02h) | `UsbNcm.sys` / `UsbNcm.inf` | SubClass 0Dh（NCM），Windows 11 / Server 2022 |

也就是说：现代 Windows 的 RNDIS 驱动 `Rndismp.inf` 认的是 **EF/04/01** 这组类代码，
而不是 Microsoft OS 描述符里的 `RNDIS` 兼容 ID；而对于带 IAD 的复合设备，Windows 恰恰是
**用 IAD 的 `bFunctionClass/SubClass/Protocol`** 去为"这个功能"匹配驱动的。
内核默认把 RNDIS 的 IAD 写成 `E0/01/03`（Wireless Controller 的 RNDIS 组合），没有任何 INF 认它，
所以设备管理器里就只能是"其他设备 → RNDIS"。社区里 Toradex 的结论也是同一句：
把 `functions/rndis.usb0/{class,subclass,protocol}` 设成 Windows 期望的 `EF/04/01` 即可自动挂驱动，
不需要 INF、也不需要 OS 描述符。

修法（`fs_overlay/common/etc/init.d/S70USB`，rndis.usb0 分支）：

```sh
echo "EF" > "$g/functions/rndis.usb0/class"
echo "04" > "$g/functions/rndis.usb0/subclass"
echo "01" > "$g/functions/rndis.usb0/protocol"
```

**坑：这三个属性是"裸十六进制"解析**（内核里的 `sscanf(page, "%02hhx", &val)`），
写成 `0xEF` 会被解析成 `0x00`（只吃掉 `0`），必须写 `EF`；同理 `0x04`→`04`、`0x01`→`01`。
板端验证：写入后回读为 `ef / 04 / 01`，重新 bind 主机即重新枚举。

MS OS 描述符（第 14.2–14.3 节）依旧保留：它不冲突，且在仍然认 `USB\MS_COMP_RNDIS` 的
主机/驱动组合上多一层保险。两条路径现在同时具备，任何一条被主机接受即可用。

### 14.9 结论：Windows 11 上用 CDC-NCM（本轮落地）

把 IAD 改成 EF/04/01 之后，Windows 11 25H2 确实把设备认成了 RNDIS（"网络适配器"里出现
Remote NDIS Compatible Device，匹配 ID = `USB\Class_ef&SubClass_04&Prot_01`），但它选中的是
**RNDIS 6.0 驱动** `rndiscmp.inf`（`usbrndis6.sys`），启动失败：问题代码 0xA、状态 0xC0000001。

板端用内核 dynamic debug 抓到的证据（`file rndis.c +p` / `file composite.c +p`）：

```text
dwc2 13500000.otg: new device is high-speed
dwc2 13500000.otg: new address 30
configfs-gadget gadget: high-speed config #1: c
configfs-gadget gadget: init rndis
configfs-gadget gadget: RNDIS RX/TX early activation ...
configfs-gadget gadget: rndis_open
```

即：主机选好配置、激活了数据接口，**然后一条 RNDIS 控制报文都没发**（没有 `RNDIS_MSG_INIT`，
也没有任何 ep0 类请求）。说明 `usbrndis6.sys` 在发第一个报文之前就判定设备不合格 —— 它期望的是
"杂项类（EF/04/01）"那套 RNDIS 约定，而 Linux 的 RNDIS gadget 是 CDC 那套（控制接口
class 02h/02h/FF + CDC 功能描述符，控制报文走 class 请求）。这不是配置能修的。

这也解释了为什么第 14.2 节的 Microsoft OS 描述符路线在 25H2 上不成立：它依赖 Windows 自带的
**RNDIS 5.x 驱动**（netrndis.inf，"Remote NDIS Compatible Device"）。25H2 上 `netrndis.inf`
文件还在，但已经不是可安装的驱动包，兼容 ID `USB\MS_COMP_RNDIS` 落不到任何驱动上，
于是只剩严格的 RNDIS 6.0 驱动可选。

改用 **CDC-NCM**（USB-IF 标准，类代码 02h/0Dh）：

- Windows 11 自带 `UsbNcm.sys` / `UsbNcm.inf`（微软《USB Device Class Drivers Included in
  Windows》：Communications 02h，Supports SubClass 0Dh，Windows 11 / Server 2022），按类自动匹配；
- Linux 与 macOS 原生支持 NCM，不需要额外驱动；
- Windows 10 及更早需要装微软的 NCM 驱动包，因此 NCM 接口仍写 `WINNCM` 兼容 ID，
  装了那个驱动包的主机会自动挂上。

落地内容：

- 内核 `configs/kernel/x2000_module_base_linux_sfc_nand_defconfig`：`CONFIG_USB_CONFIGFS_NCM=y`
  （自动 select `USB_F_NCM`、`USB_U_ETHER`；`System.map` 里确认 `ncm_bind/ncm_setup/ncm_set_alt` 都在）。
  **坑**：改内核 defconfig 之后必须重新执行 `./build.sh config` 再 `./build.sh kernel`，
  否则沙箱里的 `.config` 还是旧的（第一次就踩了：defconfig 改好、`.config` 里仍是 `is not set`）。
- `fs_overlay/common/etc/init.d/S70USB`：
  - 模式表：`mtp | ncm（默认）| rndis | both | serial`；
  - NCM 固定网卡名 `usb1`、网段 `192.168.9.168/24`；RNDIS 固定 `usb0`、`192.168.8.168/24`，
    两条链路可同时存在且不冲突；
  - 两条链路各自可跑 udhcpd（`etc/udhcpd_ncm.conf` 与 `etc/udhcpd_usb.conf`）；
  - NCM 无需类代码覆盖（内核的 NCM IAD 本身就是 02h/0Dh/00h）；
  - `status` 同时打印两条链路状态、收发计数与两个功能的类代码 / 兼容 ID；
  - `USB_REV` 升到 4（描述符布局又变了，Windows 需要新的设备实例）。
- 交付含义：这一轮除了 `rootfs.ubi` **还要重烧 `xImage`**（内核变了），u-boot 不用动。

### 14.10 NCM 首测暴露的问题：`ifname` 必须是带 `%d` 的模板

现象：Windows 侧驱动绑定成功（"网络适配器 → UsbNcm Host Device" ✓），但该网卡一直
"Media disconnected"、拿不到 IP。

根因在板端：第一版脚本写的是 `echo "usb1" > functions/ncm.usb0/ifname`，而内核的
`gether_set_ifname()`（`drivers/usb/gadget/function/u_ether.c`）要求名字里**恰好有一个 `%d`**：

```c
	/* Require exactly one %d, so binding will not fail with EEXIST. */
	p = strchr(name, '%');
	if (!p || p[1] != 'd' || strchr(p + 2, '%'))
		return -EINVAL;
```

所以那次写入被拒（`-EINVAL`），NCM 网卡保留了默认名（该模式下即 `usb0`），脚本随后的
`ifconfig usb1 ...` 全部落空 —— 而且被 `2>/dev/null` 吞掉了错误、日志里照样打印 "up"，
从主机侧看就完全像一个"驱动挂了"的问题。

修法：

- 每个功能用各自的名字**模板**：`usbncm%d` → `usbncm0`，`usbrndis%d` → `usbrndis0`
  （不用默认 `usb%d`：那样名字会随创建顺序变化，同一个地址会在不同模式下落到不同网卡上）；
- `etc/udhcpd_ncm.conf` / `etc/udhcpd_usb.conf` 的 `interface` 行同步成 `usbncm0` / `usbrndis0`；
- `bring_up_link()` 改成"等网卡出现（最多 10 秒）+ 失败必须报错"，不再静默成功。

影响范围：**纯用户态**，只需重烧 `rootfs.ubi`（内核不用动）。

### 14.11 掉进 SSH 死循环的坑：0 字节主机密钥 + NCM 改链路本地地址

**一、`sshd` 起不来的真正原因（板端实测）**

重烧之后板子 ping 得通但 22 端口是 `Connection refused`。串口上看到：

```text
# ls -l /etc/ssh/
-rw------- 1 root root 0 Mar  1 12:00 ssh_host_rsa_key        <- 0 字节
-rw------- 1 root root 0 Mar  1 12:00 ssh_host_ecdsa_key
-rw------- 1 root root 0 Mar  1 12:00 ssh_host_ed25519_key
# /etc/init.d/S50sshd restart
Unable to load host key "/etc/ssh/ssh_host_rsa_key": invalid format
...
sshd: no hostkeys available -- exiting.
```

即：首次启动时 `ssh-keygen -A` 把密钥文件**建出来了但内容是 0 字节**（无 RTC、随机池刚起来就动、文件系统也刚挂上，
具体原因待查），而 `S50sshd` 当时的判断是"文件存在就跳过生成" —— 于是这个坏状态被永久固化，
每次启动 sshd 都退出，板子彻底失联。

修法（`fs_overlay/common/etc/init.d/S50sshd`）：

- 判断改成"密钥存在**且可用**"：`[ -s "$f" ] && ssh-keygen -l -f "$f"`；不可用的（缺失/0 字节/损坏）先删掉再重新生成；
- 生成后再次校验，失败时打印文件字节数 + `df -h /`，并明确报错（是磁盘满还是随机数问题，一眼可辨）；
- `status` 也报告 `rsa key: usable / MISSING OR UNUSABLE`，并给出修复命令。

**二、NCM 链路改成链路本地静态地址（不再依赖 DHCP）**

需求：Windows 侧不愿每次手动刷新 DHCP/缓存。方案：板子在这条链路上直接用链路本地地址，
主机侧交给系统自动分配（Windows/macOS 的 APIPA，169.254/16），两边都不需要任何配置：

- `NCM_IP=169.254.9.168`、`NCM_NETMASK=255.255.0.0`，默认**不启 udhcpd**；
- `etc/udhcpd_ncm.conf` 保留为**可选**（`UDHCPD_NCM_CONF=/etc/udhcpd_ncm.conf NCM_IP=192.168.9.168 ...` 才启用）；
- RNDIS 链路维持原样（192.168.8.168/24 + udhcpd），因为 RNDIS 只面向老 Windows，那里 DHCP 是现成的。

实测依据：修好命名之后，板端 `usb0`（当时的 NCM 网卡）`RX packets: 26` —— 说明主机的 APIPA 已经在这条链路上
发 ARP/探测包了，链路是通的，唯一缺的就是板子这边的地址（见第 2 节）。所以只要板子给出 169.254.x.x，
Windows 直接就能 ping 通，无需 DHCP、无需静态配置、无需刷新。

### 14.12 SSH 启动脚本整合：S50sshd → S80SSH

第 14.11 节查到 0 字节主机密钥之后，把 SSH 的启动脚本彻底换成我们自己的，
不再沿用 buildroot（以及更早的厂商适配）的 `S50sshd`：

- 文件：`fs_overlay/common/etc/init.d/S80SSH`（buildroot 的 `etc/init.d/S50sshd` 进了 `prune.txt`）；
- 为什么放到 S80：网络（S40/S60）已经就绪，SSH 起来时板子的地址和日志都已定型，
  控制台上看到的就是最终状态；
- 风格与 S60WiFi / S70USB 一致：Part I `Func_*`（一个函数一件事）+ Part III `main`，
  带 doxygen 头注释；没有 Part II `API_*` 层，因为 SSH 没有"多步用户操作"那类需求。
  三个脚本共享的设计准则：**依赖项要验证、能修就修并说明、报告真实结果**：
  - 主机密钥按**可用性**判断（非空 + `ssh-keygen -l` 能解析），不可用的先删再重建，重建后再次校验；
    失败时打印文件字节数与 `df -h /`，并提示是空间问题还是随机数问题；
  - 启动前检查 sshd 依赖的文件与权限：`sshd_config` 不能组/其他可写、`/var/empty` 必须 root 所有、
    `/run/sshd` 要存在 —— 不对就自动修正并打印；
  - 启动用 `sshd -e`（错误打到控制台），失败明确报 `FAILED` 并提示 `S80SSH debug`；
  - `status` 一次给全：进程、22 端口监听（直接读 `/proc/net/tcp`，不依赖 netstat）、密钥可用性、
    配置权限与认证方式、privsep 属主、root 口令状态。
- 动词：`start | stop | restart | status | keys | debug | authorized`
  （`keys` 就是这次人工修密钥的自动化版本；`debug` = 前台 `sshd -e -d`；
  `authorized` 保留"从可写分区安装公钥"的能力，虽然默认已关闭公钥认证）。
- 好处：开机自愈（密钥坏了下一次启动或 `keys` 都能修好）、排错不靠猜（每条依赖都有检查与说明）、
  与另外两个脚本同源同风格。

## 15. 相机模块（soc_camera.ko）的两处修复

本节整理同事 gwp 的改动（`_Workspace_/.playground/camera_gwp20260929.tar.gz` 对比得出），
代码以 `configs/module_driver/` 覆盖层的形式进入本仓库。

### 15.1 先认清对象：这个 .ko 是什么、源码在哪

- 它**不是**内核里的 V4L2 `soc_camera` 框架（那个在 `kernel/.../media/{platform,i2c}/soc_camera/`，与本次无关）；
  它是君正自己的相机/ISP 模块，源码在 `vendor/module_driver/soc/x2000_510/camera/`，
  构建产物是 `build/module_driver/soc/x2000_510/camera/soc_camera.ko`。
- 组成（权威清单 = 该目录 `Makefile` 的 `MODULE_NAME := soc_camera`）：
  `camera.c`、`hal/{camera_gpio,csi,vic,dsys}.c`、`cim/cim.c`、`vic/vic_channel_mem.c`、
  `isp/{isp,isp_sys,mscaler,isp_sensor,isp_tuning,vic_channel_tiziano}.c`、`isp/isp-core/`。
- 参数与加载：`vendor/module_driver/package/soc/x2000_510/camera/camera.mk` 生成 `/module_driver/soc_camera.sh`
  （参数来自产品配置 `MD_X2000_510_CAMERA_VIC*` 等键），开机由 `S11module_driver_default` 统一加载。
- 取证方法：`build/module_driver/soc/x2000_510/camera/.soc_camera.o.cmd`（每个 .o 的完整源/头文件清单）、
  `.soc_camera.ko.cmd`（链接命令）、`modinfo`（vermagic/depends）。

### 15.2 修复一：ISP 掉电位没有被清掉（isp.c）

原厂代码在 ISP 软复位里只把 `CPM_LCR.ISP0_PD / ISP1_PD` 置位，全驱动没有一处把它清 0，
结果 ISP 寄存器块（0x13700000）读回恒为 0，**永远抓不到帧**。
修法：软复位之后清 PD 位并 `udelay(20)`。

### 15.3 修复二：mscaler 的三类问题（mscaler.c，共 7 处）

1. 补两处漏掉的 `mutex_unlock()` —— 否则互斥锁永久泄漏，`mscaler_open/release` 会卡死；
2. get_frame **持锁睡眠**：原代码在 `wait_event_*()` 期间一直持有 `data->lock`，而 `put_frame()`
   （把缓冲还给驱动）需要同一把锁，free_list 一见底就互相卡死（实测 VI 帧率 29.8 → 14.9）。
   修法：睡前解锁、醒来再锁，并检查 `is_stream_on` 后继续；
3. 帧泄漏回收：新增 `mscaler_reclaim_leaked()`，只回收"卡在 `trans` 且不是 DMA 目标"的帧，
   以及"卡在 `user` 超过 1.5 s"的帧（为此在 `hal/camera_sensor.h` 的 `struct frame_data` 里加了
   `user_jiffies` 时间戳做老化判断，避免误回收在飞帧）；触发点是 get_frame 里每 2 s 检查一次
   "既无空闲又无可用且 frame_counter==0"，以及连续 3 次超时；回收时打印 `recovered N leaked frame(s)`。

可观测性：新增 `timeout_streak/recover_cnt/reclaim_cnt/last_reclaim` 计数，并在
`/sys/isp1/mscaler/show_mscaler_info` 输出帧状态汇总（free/usable/user/trans/other + mem_cnt/frame_counter/dma_index）
与 DMA 目标索引。现场判断标准：free/usable 有数、user/trans 不堆积，dmesg 不频繁出现 recovered。

### 15.4 落地方式

- 我们的改动放在 `configs/module_driver/soc/x2000_510/camera/{hal/camera_sensor.h,isp/isp.c,isp/mscaler.c}`，
  由 `build.sh` 的 `sync_sources()` 覆盖进沙箱（与 `configs/kernel`、`configs/uboot` 完全同构），
  `vendor/` 继续逐字节不变 —— 厂商升级时这三个文件就是"我们的 delta"。
- 只重编驱动模块：`./build.sh module_driver`（内部就是 `make apps packages=module_driver`；
  vendor 的 `apps` 目标会顺带把 `.ko` 装进 rootfs target 树），随后 `./build.sh fs` 重打文件系统。

### 15.5 提醒：别人编好的 .ko 不要直接用

同事那份 `soc_camera.ko` 的 vermagic 是 `5.10.186+ ...`（多一个 `+` = 他那边内核树是 dirty 的），
我们是 `5.10.186 ...`；vermagic 不一致 `insmod` 会直接拒绝（`version magic ... should be ...`）。
**永远在目标树里重编**，不要用 `insmod -f` 之类的手段绕。









