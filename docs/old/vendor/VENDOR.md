# 厂商基线登记（vendor baseline）

本文件回答同一个问题的三个侧面：**这份 vendor/ 是从哪来的、怎么生成的、怎么核对**。

## 1. 来源

| 压缩包 | 大小 | 说明 |
|--------|------|------|
| `ingenic_sdk_240407.tar.bz2` | 10,863,429,883 B (10.1 GiB) | 主 SDK：repo 工具管理的 53 个子项目快照，解压日期 2024-04-07 |
| `ingenic_x2000_sdk_patch_v1.0.0.tar.bz2` | 70,310,177 B | 原厂补丁 v1.0.0（2024-04-17）：离线下载缓存 + 板级配置等 |
| `ingenic_mips_toolchain.tar.bz2` | 193,018,287 B | gcc720 工具链压缩副本（与本仓库外置工具链包内容一致） |

三个包保存在仓库外的 `original_bsp/`（`original_bsp/` 已被 `.gitignore` 排除，仓库不重复保存）。
sha256 记录在 `meta/vendor_tarballs.sha256`。

**上游获取方式**（供将来取新版本参考）：君正 Gerrit + repo 工具，需要厂商账号授权，见
`doc/.Markdown/客户支持/Linux平台源码获取.md`。因此本仓库的复现性依赖上面这三个包，而不是上游。

## 2. 导入范围与规则

规则（详见 `meta/prune_manifest.tsv`）：**只按"整组件"裁剪，组件内部逐字节保持厂商原样**；
唯一例外是 `doc/`，只导入 `.Markdown` 子树（PDF 不导入）。

保留 18 项：`build`、`bootloader/uboot-x2000`、`kernel/kernel`(5.10.186)、
`buildroot/buildroot`、`buildroot/buildroot_patch`、`module_driver`、`wireless/bcm`、
`third_party`、`lib2d`、`libhardware2`、`libimpf`、`libisp`、`libmedia`、`libutils2`、`libmcu`、
`factory_test`、`ota_updater`、`doc/.Markdown`。

裁掉：其它 SoC 内核（x1000/x1021/x2000）、其它 u-boot、`demos`、其它无线模组、
`tools/iconfigtool`、`tools/toolchains`（改为外置工具链包）、`doc/` 的 PDF、`.repo`。

复现命令：

```bash
scripts/prune_vendor.sh --verify-only     # 只打印保留/裁掉清单与体积
scripts/prune_vendor.sh                   # 重新生成 vendor/ 与 vendor/meta/
```

## 3. 目录内 meta/ 的用途

| 文件 | 内容 | 用途 |
|------|------|------|
| `prune_manifest.tsv` | 每个组件的 keep/drop 与体积 | 一眼看清导入边界 |
| `repo-manifest.xml` | 上游 repo 清单原文 | 将来从 Gerrit 重取时的锚点 |
| `project_lock.tsv` | 53 个子项目的 path + HEAD + 日期 + 主题 | 精确锁定厂商版本 |
| `files.sha256` | 导入树全量文件校验和 | 判断"我们改了哪些厂商文件"、核对导入完整性 |
| `vendor_tarballs.sha256` | 三个原厂压缩包的校验和 | 厂商基线的可追溯性 |

## 4. 版本与标签

- Git tag `vendor/20240407`：厂商主包快照（2024-04-07）导入提交。
- 原厂补丁在下一个提交：`Feat: apply Ingenic SDK patch v1.0.0 (...)`。
- 已知差异：厂商 V2.1.1 出厂包（2025-02-27）比本快照新，其独有内容（如内核
  `CONFIG_XIMAGE_LDADDR`、`Kernel_uImage_menu` 等符号）在本快照中尚不存在；
  我们只吸收了与硬件相关的两项：`CONFIG_HIGHMEM=y` 与 u-boot `LPJ="11935744"`。

## 5. 与厂商"出厂包"的关系

V2.1.1 出厂包（`darwin_x2000_v2.1.1_20250221.tar.gz`）不进入本仓库，保存在
`_Workspace_/.playground/ingenic_x2000/vendor_docs/`，其中包含厂商自用的 4 个配置与
预编译镜像，可作为日后比对参考（不作为构建输入）。
