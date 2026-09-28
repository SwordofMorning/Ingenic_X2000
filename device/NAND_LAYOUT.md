# NAND 分区布局（Darwin_X2000_V2.1.1 / X2000H / ATO25D1GA 128 MiB）

本文档记录 SPI-NAND 分区"有哪些来源、谁说了算、怎么核对"，避免下次改分区时翻源码。

## 1. 芯片容量

- 板载 `ATO25D1GA`：**1 Gbit = 128 MiB**（驱动参数 `flashsize = 2*1024*64*1024`，见
  `vendor/kernel/kernel/drivers/mtd/devices/ingenic_sfc_v1/nand_device/ato_nand.c`）。
  厂商《X2000 存储支持列表》中该型号列为 1Gb / 3.3V / MP。
- 页 2 KiB + OOB 64 B，块 128 KiB，Quad 模式（`need_quad = 1`）。

## 2. 三处"分区表"来源与当前取值

| 来源 | 位置 | 取值 | 是否适用于本板 |
|------|------|------|----------------|
| u-boot 默认 mtdparts | `vendor/bootloader/uboot-x2000/include/configs/x2000_base_common.h:282` | `mtdparts=nand:1M(boot),8M(kernel),40M(rootfs),-(data)` | **适用**（合计 49 MiB + 剩余 data，128 MiB 放得下） |
| 内核板级 dts `fixed-partitions` | `vendor/kernel/kernel/module_drivers/dts/x2000_module_base.dts`（`nandflash@1` 节点） | `uboot 1M + kernel 8M + rootfs 247M`（合计 256 MiB） | **不适用**（按 256 MiB 芯片写的，超出本芯片容量） |
| cloner 烧录参数（SPL 参数块） | flash 偏移 `0x5800`（`CONFIG_SPIFLASH_PART_OFFSET`），由烧录工具写入 | 由烧录工具界面决定 | 实际生效的一份，需上板核对 |

## 3. 上板核对方法

```sh
cat /proc/mtd                 # 内核看到的分区表（最权威的运行期视图）
dmesg | grep -i -E "nand|mtd|sfc"
cat /proc/cmdline             # 看是否带 mtdparts=
```

对照上面第 2 节判断实际生效的是 u-boot 的 `mtdparts` 还是 dts 的 `fixed-partitions`。
若发现生效的是 dts 那份（rootfs 只有 ~40 MiB 可用之外的异常），再把
`x2000_module_base.dts` 的 `nandflash@1` 分区改成与 128 MiB 匹配的取值，
并同步调整 `configs/kernel/` 与 `configs/uboot/` 的覆盖文件。

## 4. 当前产品决定

- 采用**厂商默认布局**（不引入君正 OTA / A/B 双分区），即
  `uboot 1M + kernel 8M + rootfs 40M + data 剩余`。
- 因此交付物只需要 3 个烧录文件：`u-boot-spl-pad.bin`、`xImage`、`rootfs.squashfs`。
- 若将来要启用 A/B OTA，需要重新分配（估算见方案文档 `10_packaging_plan_review.md` §1.2：
  `uboot 1M + kernel 8M×2 + rootfs 40M×2 + ota 1M ≈ 98 MiB`），并重新生成烧录参数。
