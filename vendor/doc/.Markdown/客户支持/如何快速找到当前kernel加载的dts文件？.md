# 		如何快速找到当前kernel加载的dts文件？

有时候需要修改kernel的dts设备树，那么就需要定位当前kernel加载的dts文件。

本文以x2600e + halley7开发板的编译主配置：x2600e_halley7_v1.0_nand_5.10_factory_defconfig为例，展示如何从当前主配置定位到当前使用的kernel dts文件。

查找当前加载的kernel目录及配置文件：

```c
sxyzhang@T430:~/x2670_sz/build$ cat configs/x2600e_halley7_v1.0_nand_5.10_factory_defconfig | grep kernel
APP_kernel_dir=../kernel/kernel
APP_kernel_config=x2600_module_base_linux_sfc_nand_defconfig
```

可见，当前加载的kernel路径为：../kernel/kernel

当前kernel加载的配置文件为：x2600_module_base_linux_sfc_nand_defconfig

```c
sxyzhang@T430:~/x2670_sz/build$ cd ../kernel/kernel
sxyzhang@T430:~/x2670_sz/kernel/kernel$ cat arch/mips/configs/x2600_module_base_linux_sfc_nand_defconfig| grep dts
CONFIG_DT_X2600_MODULE_BASE_DTS_FILE="x2600_module_base.dts"
sxyzhang@T430:~/x2670_sz/kernel/kernel$ find ./ -name x2600_module_base.dts
./module_drivers/dts/x2600_module_base.dts
```

可见，当前加载的dts文件为：kernel/kernel/module_drivers/dts/x2600_module_base.dts

**注意**：不同的编译主配置文件加载的dts可能会不同，需要根据自己的主配置进行调整。但是定位dts的方法是一样的。