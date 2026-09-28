# 如何确定当前uboot的编译配置文件及目录？

1, 确定当前uboot的编译路径和配置文件（以Darwin v2.0板子的配置为例）

```
sxyzhang@T430:~/my/work/linux/x2000_sz/build$ cat ./configs/x2000_darwin_v20_nand_defconfig | grep uboot
APP_uboot_toolchain_dir=../tools/toolchains/mips-gcc720-glibc229
APP_uboot_dir=../bootloader/uboot-x2000
APP_uboot_config=x2000_base_xImage_sfc_nand
```

可知，当前uboot编译目录为：../bootloader/uboot-x2000

所用的uboot配置文件为：x2000_base_xImage_sfc_nand

2, 确定当前的配置文件

```
sxyzhang@T430:~/my/work/linux/x2000_sz/build$ cd ../bootloader/uboot-x2000
sxyzhang@T430:~/my/work/linux/x2000_sz/bootloader/uboot-x2000$ vim include/config.h
```

![9](如何确定当前uboot的编译配置文件及目录？.assets/9.png)

```
sxyzhang@T430:~/my/work/linux/x2000_sz/bootloader/uboot-x2000$ vim include/configs/x2000_base.h
```

![10](如何确定当前uboot的编译配置文件及目录？.assets/10.png)

