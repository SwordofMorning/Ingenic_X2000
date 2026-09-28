# fs_overlay — 产品根文件系统层

这一层是"我们的文件"进入 rootfs 的唯一入口，等价于 Hi3556V200 仓库里的 `fs_overlay/`。

## 合并顺序（build.sh 的 phase 4/4 或 `./build.sh fs`）

```text
buildroot/output/target                     ← buildroot 原生 + 厂商 rootfs_config 注入（S11module_driver_default、挂载脚本…）
  ⊕ fs_overlay/common/                      ← 公共层（所有产品共用）
  ⊕ fs_overlay/darwin_v211/                 ← 产品层（覆盖前者）
  → 重新执行 buildroot，重新生成 rootfs.squashfs
```

要点：

- **在厂商 rootfs 装配之后覆盖**，所以我们的同名文件一定生效；厂商文件零修改。
- 两个目录**按需创建**（仓库里默认不建空目录，避免空目录被拷进 rootfs 的噪音）。首次放入文件时
  `mkdir -p fs_overlay/darwin_v211/etc/init.d` 即可。
- 目录结构就是 rootfs 的目录结构，例如：
  - `fs_overlay/darwin_v211/etc/init.d/S99myapp`
  - `fs_overlay/darwin_v211/usr/bin/myapp`
  - `fs_overlay/common/etc/fs-version`
- 只放**文本与小文件**；二进制可执行文件请走 Git LFS（根 `.gitattributes` 已配置 `*.bin/*.so/...`）。
- 与厂商既有机制的分工：厂商的 `rootfs_config/file/<feature>/` 由 IConfigTool/Kconfig 开关驱动（我们不修改）；
  属于"我们产品自己的文件"一律放这里，便于一眼看清我们改了什么。
- 特例：**出厂口令不在这一层**。root 口令由 `products/<product>.conf` 的 `ROOT_PASSWORD` 声明，
  打包时算成 SHA-512 哈希写进 `etc/shadow` 的 root 行（串口与 SSH 共用；见
  `docs/bringup_v211_fixes.md` 第 13 节）。这样换产品/换口令只改一个配置项，
  不必在 overlay 里维护哈希与盐值。
