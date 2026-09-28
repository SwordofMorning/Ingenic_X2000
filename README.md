# X2000 SDK 仓库（Ingenic Darwin 5.10 / X2000H / SPI-NAND）

本仓库把君正 X2000 的 BSP 从"压缩包 + 手动解压 + 手动配置"变成一个**可以直接 clone、一条命令出镜像**的仓库。
目标板：**Darwin_X2000_V2.1.1（芯片 X2000H，512 MB LPDDR3，1 Gbit SPI-NAND）**。

参考同门仓库：`_Workspace_/Hi3559V200_SDK_V2.0.1.0`（Hi3556V200），本仓库沿用它的约定：
源码树只读、编译在 `build/` 沙箱、显式 `fs_overlay/`、固定交付树、二进制走 Git LFS。

---

## 1. 五分钟上手

```bash
# 0) 前置：Ubuntu 18.04/20.04（本仓库在 18.04 上验证），python2 可用
#    工具链不进 git，需要把两个压缩包放到 toolchains/（见第 3 节）

./build.sh toolchain check      # 校验工具链（sha256 + gcc 版本 + 编译冒烟测试）
./build.sh toolchain setup      # 解压到 toolchains/<name>/（相对路径，不写 /opt）
./build.sh all                  # 全量构建：u-boot + buildroot + kernel + apps + rootfs + 镜像
./build.sh release              # 汇总可烧录包到 build/release/darwin_v211/
ls -lh build/release/darwin_v211/
```

产物与烧录步骤见 `device/README.md`；NAND 分区口径见 `device/NAND_LAYOUT.md`。

## 2. 常用命令

| 命令 | 作用 |
|------|------|
| `./build.sh env` | 打印产品 / 配置四元组 / 工具链 / 沙箱路径 |
| `./build.sh check` | 一键自检：产品配置、厂商树、工具链、宿主依赖、git 是否干净 |
| `./build.sh sync` | 把 `vendor/` 复制到 `build/`，叠加我们的 `configs/`，建立工具链软链 |
| `./build.sh config` | 执行产品 defconfig（纯文本配置，等价于 IConfigTool 的文本路径） |
| `./build.sh all` | 全量构建（4 个阶段，见下） |
| `./build.sh uboot\|kernel\|buildroot\|apps` | 单模块编译 |
| `./build.sh fs` | 只重做文件系统：合并 `fs_overlay/` + 重新打包 rootfs |
| `./build.sh release` | 汇总交付包（分区名 + md5sum.txt + notes.txt） |
| `./build.sh clean` | 删除 `build/` 沙箱（**不动** `vendor/` 与 `toolchains/`） |

`./build.sh all` 的四个阶段：

```text
phase 1/4  u-boot  →  buildroot（产出 host 工具链与 staging sysroot）
phase 2/4  预编译 staging 依赖（third_party/speexdsp，厂商包顺序里它排在 libmedia 之后）
phase 3/4  厂商 make all（kernel → apps → rootfs → 镜像收集 → overlay_bootfs → ota）
phase 4/4  合并 fs_overlay/ 并重新打包 rootfs
```

## 3. 工具链（不进 git）

| 包 | 大小 | 用途 |
|----|------|------|
| `mips-gcc720-glibc229.tar.xz` | 228 MB | u-boot（全部 x2000 配置） |
| `mips-gcc930-glibc228.tar.xz` | 1.5 GB | 5.10 内核与 rootfs（buildroot 外部工具链） |

放在仓库根的 `toolchains/` 下即可；`./build.sh toolchain setup` 会解压成
`toolchains/<name>/`，`sync` 阶段在沙箱内建立**相对软链**（`build/tools/toolchains/<name>`），
厂商写死的两处相对路径（`../tools/toolchains/...`、`$(TOPDIR)/../../tools/toolchains/...`）因此原样生效，
全程不写 `/opt`、不需要 root。`MANIFEST.sha256` 记录 sha256 与文件数，`toolchain check` 会核对。

## 4. 目录结构

```text
build.sh             唯一构建入口
products/            产品真源（darwin_v211.conf：配置四元组 / 介质 / 工具链 / overlay / 交付名）
configs/             我们对厂商配置的覆盖（kernel HIGHMEM、uboot LPJ、build/Config.in）
fs_overlay/          产品根文件系统层（common + <product>，见其 README）
device/              烧录说明与 NAND 分区口径
doc/.Markdown/       厂商文档（Markdown 版，1886 个文件；PDF 未导入）
docs/                我们自己的文档（含厂商文档索引）
scripts/             prune_vendor.sh（可复现地生成 vendor/）+ 后续工具
toolchains/          [不进 git] 交叉工具链压缩包与解压结果
vendor/              [厂商基线，2.3 GB] 只读，勿改；见 vendor/meta/
build/               [不进 git] 构建沙箱（vendor 副本 + 我们的覆盖 + 全部产物）
```

## 5. 厂商基线的边界（重要）

- `vendor/` 只做**整组件**裁剪（清单在 `vendor/meta/prune_manifest.tsv`），组件内部**逐字节与厂商快照一致**；
  唯一例外是 `doc/` 只导入 `.Markdown` 子树。
- `vendor/meta/` 记录了可复现性证据：`repo-manifest.xml`（上游 repo 清单）、`project_lock.tsv`
  （53 个子项目的 HEAD）、`files.sha256`（全树校验）、`prune_manifest.tsv`（保留/裁掉清单）。
- 我们对厂商文件的改动**全部集中在** `configs/`（内核配置、u-boot 板表、build/Config.in），
  每处都能与 `vendor/` 逐行 diff。
- 厂商快照时间：**2024-04-07**（tag `vendor/20240407`），随原厂补丁 `sdk_patch` v1.0.0 一并导入。

## 6. 配置体系

```text
products/darwin_v211.conf           ← 产品真源（改这一个文件就能换 defconfig / 介质 / 工具链 / 覆盖层）
        │
        ├─ APP_CONFIG      = x2000_darwin_v20_5.10_nand_factory_defconfig   (厂商主配置：APP_*/MD_* 数百项)
        ├─ KERNEL_CONFIG   = x2000_module_base_linux_sfc_nand_defconfig     (+ configs/kernel 覆盖)
        ├─ UBOOT_CONFIG    = x2000_base_xImage_sfc_nand                     (+ configs/uboot 覆盖 LPJ)
        └─ ROOTFS_CONFIG   = buildroot_x2000_510_wifi_common_defconfig
```

## 7. 当前状态与已知事项

- 已打通：`toolchain check/setup` → `all` → `release`，产出 `u-boot-spl-pad.bin` / `xImage` / `rootfs.squashfs`。
- 已按 X2000H 修正：**`CONFIG_HIGHMEM=y`**（否则 512 MB 只能用 128 MB）、u-boot **`LPJ="11935744"`**。
- 无线（WiFi/BT）已包含在配置内：CYW43438（bcmdhd SDIO）+ `wireless/bcm` 脚本与固件 + `wpa_supplicant`/`bluez`。
- OTA：保留源码但**未启用**（`OTA=n`），交付物不含 OTA 包。
- 尚未上板验证：`device/README.md` 第 4 节列了上电自查命令，第一次上板后请把 `cat /proc/mtd`、
  `cat /proc/meminfo` 的结果回填到 `device/NAND_LAYOUT.md`。
