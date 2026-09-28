# 烧录说明（Darwin_X2000_V2.1.1 / X2000H / SPI-NAND）

## 0. 交付物

`./build.sh release` 生成 `build/release/darwin_v211/`：

| 文件 | 分区 | 说明 |
|------|------|------|
| `u-boot-spl-pad.bin` (24 KB) | uboot | SPL + U-Boot（SFC NAND 启动）。**启动参数在本文件里**，换 rootfs 类型必须一起重烧 |
| `xImage` (~5 MB) | kernel | 5.10.186 内核（君正私有格式，**不能**用 uImage 代替） |
| `rootfs.ubi` (~42 MB) | rootfs | **可写根文件系统**（UBIFS in UBI，卷名 `rootfs`，autoresize）。当前产品用这个 |
| `rootfs.ubifs` (~41 MB) | — | UBI 内层镜像，仅在自己 ubinize / 手工写入时需要 |
| `rootfs.squashfs` (~30 MB) | rootfs | 只读回退方案（需配套旧的 `rootfstype=squashfs ro` 启动参数） |
| `md5sum.txt` / `notes.txt` | — | 校验与构建记录 |

> 启动参数由 u-boot 生成（见 `configs/uboot/x2000_base_common.h` 覆盖）：
> `ubi.mtd=rootfs root=ubi0:rootfs rootwait rootfstype=ubifs rw`。
> 因此 rootfs 分区需要 ≥64 MB（镜像 42 MB + 运行期写入余量）。

## 1. 烧录工具（不在 SDK 内）

```bash
# Ubuntu 版（厂商 FTP；该账号是厂商文档页公开的下载账号）
wget ftp://szingenic:hq7Wy0gws@ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz
tar xzf cloner-latest-ubuntu.tar.gz && cd cloner-* && sudo ./cloner
```

Windows 版把 `-ubuntu.tar.gz` 换成 `-windows.zip`（需先装驱动）。

## 2. cloner 操作步骤

1. **INFO → 选择板级**：x2000 + nand。
   ⚠️ **DDR 选项必须选 X2000H（512 MB）**。X2000 / X2000E / X2000H 是 pin 兼容的，
   差异只体现在烧录工具的 DDR 参数上；选错会导致内存只认到 128 MB。
2. **SFC 面板 → 设定分区**：偏移 / 大小 / 名称，与 `device/NAND_LAYOUT.md` 第 2 节一致
   （默认 `uboot 1M` / `kernel 8M` / `rootfs 40M` / `data` 其余）。
3. **选择镜像文件**：uboot → `u-boot-spl-pad.bin`；kernel → `xImage`；**rootfs → `rootfs.ubi`**。
4. **OPS → 选存储器类型**：nand flash 选 `SFC_NAND`（nor 选 `SFC_NOR`）；
   rootfs 分区的 `Manage_mode` 必须是 **`MTD_MODE`**（裸分区写入）——UBI/UBIFS、squashfs 都要求这样，
   否则内核挂载会报 `No filesystem could mount root`。
5. **开始烧录**：点击"开始"后，**按住开发板 BOOT 键不放 → 再按 Reset（或重新上电）**。
6. 首次/全量烧录建议勾选"全部擦除"；擦除阶段耗时较久属正常。

## 3. 串口

- 波特率 **3000000 8N1**（无流控），串口芯片 **CH343**（驱动在
  `vendor/doc/.Markdown/开发使用说明/CH343串口驱动安装/`，或 `make && sudo make load`）。
- 本板 log 口为 **uart2_pd**（厂商 `testlist` 记录：`darwin_x2000_v2.1.1: x2000h + nand, uart2_pd`）。
- 开发阶段若看不到内核打印：`dmesg`；或去掉 u-boot 的 `ARG_QUIET`（见厂商 FAQ）。

## 4. 上电后自查

```sh
cat /proc/meminfo | head -2      # X2000H 应看到 ~500 MB（已开启 CONFIG_HIGHMEM）
cat /proc/mtd                    # NAND 分区表（见 device/NAND_LAYOUT.md）
uname -a                         # 应显示 5.10.186
lsmod | head                     # 驱动模块（bcmdhd 等）
ifconfig -a                      # wlan0 是否出现（WiFi 见 docs/）
cat /etc/init.d/S4*wifi*         # 无线启动脚本
```

## 5. 已知注意事项

- 本仓库使用**厂商默认分区**，未启用君正 A/B OTA（`OTA=n`，见 `products/darwin_v211.conf`）。
- 出厂镜像里还有 `userdata.ubifs`（data 分区镜像）；当前构建产出 `rootfs.squashfs`（只读回退）
  与 `rootfs.ubifs` + `rootfs.ubi`（**烧录用后者**），`userdata.ubifs` 尚未生成，
  若需要可写数据分区的镜像，再按厂商 `ubi` 相关配置生成（`APP_br_ubi_*`）。
- 若烧录后只能看到 u-boot 打印：八成是把 `uImage` 当 `xImage` 烧了，或 kernel 偏移设错。
- 只改了 rootfs 时**只烧 `rootfs.ubi` 即可**（u-boot 与内核未变）；全量重烧见第 0 节的三个文件。

## 6. 登录板端（出厂账号 root / cdjp123）

- 出厂账号：**用户名 `root`，口令 `cdjp123`**。真源是 `products/darwin_v211.conf` 的
  `ROOT_PASSWORD`；改完重新 `./build.sh fs` 并重烧 `rootfs.ubi` 即可。
  镜像里存的是 SHA-512 哈希（`/etc/shadow`，`0600 root:root`），**串口与 SSH 共用同一套账号**。
- 串口：见第 3 节（3000000 8N1）。串口是唯一的"救砖"入口，建议先接好再烧录。
  注意当前 `etc/inittab` 的串口是 `respawn:-/bin/sh`，**不要求登录**（直接就是 root shell）。
- 网络：`wlan0` 用 `S60WiFi connect <ssid> [pwd]` 联网（见 `docs/bringup_v211_fixes.md` 第 11 节），
  或 USB RNDIS（`usb0`）直连。板子能 ping 通说明网络层没问题。
- SSH：镜像内是 buildroot 的 **OpenSSH**（`sshd` + `ssh-keygen`），开机由 `S50sshd` 启动；
  认证方式为**口令**（`PubkeyAuthentication no`，板上不需要放任何密钥）：

  ```sh
  ssh root@<板子IP>        # 口令 cdjp123
  ```

- 排错：`/etc/init.d/S50sshd status` 会报告 sshd 是否在跑、主机密钥数量、root 口令是否已设置；
  仍失败就 `/usr/sbin/sshd -e -d` 前台运行看具体报错。
  第四轮（SSH 起不来）的排查过程见 `docs/bringup_v211_fixes.md` 第 12 节，口令实现见第 13 节。

