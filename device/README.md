# 烧录说明（Darwin_X2000_V2.1.1 / X2000H / SPI-NAND）

## 0. 交付物

`./build.sh release` 生成 `build/release/darwin_v211/`：

| 文件 | 分区 | 说明 |
|------|------|------|
| `u-boot-spl-pad.bin` (24 KB) | uboot | SPL + U-Boot（SFC NAND 启动） |
| `xImage` (~5 MB) | kernel | 5.10.186 内核（君正私有格式，**不能**用 uImage 代替） |
| `rootfs.squashfs` (~48 MB) | rootfs | 只读根文件系统 |
| `md5sum.txt` | — | 与文件同名同序，烧录后可用 `md5sum -c` 核对 |
| `notes.txt` | — | 构建时间 / 配置来源 / git 版本 |

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
3. **选择镜像文件**：分别指向 `u-boot-spl-pad.bin` / `xImage` / `rootfs.squashfs`。
4. **OPS → 选存储器类型**：nand flash 选 `SFC_NAND`（nor 选 `SFC_NOR`）；
   rootfs 的 `Manage_mode` 必须是 **`MTD_MODE`**，否则内核挂载会报
   `No filesystem could mount root, tried: squashfs`。
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
- 出厂镜像里还有 `userdata.ubifs`（data 分区镜像）；当前构建只产出 `rootfs.squashfs`，
  若需要可写数据分区的镜像，再按厂商 `ubi` 相关配置生成（`APP_br_ubi_*`）。
- 若烧录后只能看到 u-boot 打印：八成是把 `uImage` 当 `xImage` 烧了，或 kernel 偏移设错。
