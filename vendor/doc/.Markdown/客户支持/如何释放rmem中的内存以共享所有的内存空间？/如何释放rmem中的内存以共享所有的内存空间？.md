# 如何释放rmem中的内存以共享所有的内存空间？

​	rmem通常是linux系统为特定的模块预留的内存空间，常用的是camera、lcd。但是如果客户产品没有这些功能需求，可以释放rmem以求得更大的可用系统内存空间。那么如何释放rmem中的内存以共享所有的内存空间？

1, 确定配置文件（以Darwin v2.0板子的配置为例）

```
sxyzhang@T430:~/my/work/linux/x2000_sz/build$ cat ./configs/x2000_darwin_v20_nand_defconfig | grep uboot
APP_uboot_toolchain_dir=../tools/toolchains/mips-gcc720-glibc229
APP_uboot_dir=../bootloader/uboot-x2000
APP_uboot_config=x2000_base_xImage_sfc_nand
```

可知，当前uboot编译目录为：../bootloader/uboot-x2000

所用的uboot配置文件为：x2000_base_xImage_sfc_nand

2, 删除保留内存设置

```
sxyzhang@T430:~/my/work/linux/x2000_sz/build$ cd ../bootloader/uboot-x2000
sxyzhang@T430:~/my/work/linux/x2000_sz/bootloader/uboot-x2000$ vim boards.cfg
```

在boads.cfg中找到x2000_base_xImage_sfc_nand对应的一行，删除RMEM_MB=XXX

3, 重新编译uboot并烧录生成的uboot镜像

```
sxyzhang@T430:~/my/work/linux/x2000_sz/bootloader/uboot-x2000$ cd ../../build
sxyzhang@T430:~/my/work/linux/x2000_sz/build$ make uboot
```

4, 确认是否释放rmem成功（在开发板的串口端）

![2](如何释放rmem中的内存以共享所有的内存空间？.assets/2.png)

可见，已经没有了rmem的内存段显示来。至此，释放rmem的操作已完成。