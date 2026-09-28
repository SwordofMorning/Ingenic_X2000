# X2000_添加客户自定义配置文件

以下内容以 Darwin_x2000_V2.0 开发板为参照

## 1 创建整体自定义配置文件(defconfig)

​        比如客户想创建自己的主配置，比如为 x2000_example_factory_defconfig， 那就可以拷贝  Darwin_x2000_V2.0 板级主配置  x2000_darwin_factory_defconfig 为  x2000_example_factory_defconfig

```c
/build$ cp configs/x2000_darwin_factory_defconfig configs/x2000_example_factory_defconfig -arf
```

## 2 修改uboot 配置

**确定当前使用的uboot配置**

使用IConfigTool工具选择上面创建的主配置文件



![2023-04-17_16-19](Darwin_X2000_添加客户自定义配置文件.assets/01.png)



点击Open, 打开`工程/编译` 配置

![6](Darwin_X2000_添加客户自定义配置文件.assets/02.png)



![2023-05-22_16-27](Darwin_X2000_添加客户自定义配置文件.assets/03.png)

或者通过以下命令：

```c
build$ grep -nr "uboot" configs/x2000_example_factory_defconfig 
3:APP_uboot_toolchain_dir=../tools/toolchains/mips-gcc720-glibc229
4:APP_uboot_dir=../bootloader/uboot-x2000
5:APP_uboot_config=x2000_base_xImage_sfc_nand
```

可知，当前的uboot配置为：x2000_base_xImage_sfc_nand

```c
bootloader/uboot-x2000$ grep -nr "x2000_base_xImage_sfc_nand" boards.cfg
679:x2000_base_xImage_sfc_nand	 mips        xburst2	x2000_base	 ingenic	x2000_v12	x2000_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_BOOT,RMEM_MB=16,SPL_PARAMS_FIXER
```



在文件boards.cfg  添加自己的uboot配置为 x2000_base_xImage_example_sfc_nand

```c
x2000_base_xImage_example_sfc_nand	 mips        xburst2	x2000_base	 ingenic	x2000_v12	x2000_base:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_BOOT,RMEM_MB=16,SPL_PARAMS_FIXER
```



## 3 修改 kernel 配置

**确定当前使用的kernel配置**

使用IConfigTool工具选择上面创建的主配置文件



![2023-04-17_16-19](Darwin_X2000_添加客户自定义配置文件.assets/01.png)



点击Open, 打开`工程/编译` 配置

![6](Darwin_X2000_添加客户自定义配置文件.assets/02.png)



![2023-05-22_16-36](Darwin_X2000_添加客户自定义配置文件.assets/04.png)

或者通过以下命令：

```c
build$ grep -nr "kernel" configs/x2000_example_factory_defconfig 
6:APP_kernel_dir=../kernel/kernel-x2000
7:APP_kernel_config=x2000_module_base_linux_sfc_nand_defconfig
481:App_ota_updater_kernel_img_path=../build/output/xImage
```

可知，当前的kernel 配置为：x2000_module_base_linux_sfc_nand_defconfig

创建客户自己的kernel配置，比如为：x2000_module_base_example_linux_sfc_nand_defconfig

```c
kernel/kernel-x2000$ cp arch/mips/configs/x2000_module_base_linux_sfc_nand_defconfig arch/mips/configs/x2000_module_base_example_linux_sfc_nand_defconfig -arf
```



## 4 修改 kernel 加载的 dts 文件

**确定当前使用 .dts 文件**

首先确定当前使用的.dts文件

通过如下命令：

```c
kernel/kernel-x2000$ grep -nr ".dts" arch/mips/configs/x2000_module_base_example_linux_sfc_nand_defconfig
90:CONFIG_DT_X2000_MODULE_BASE_DTS_FILE="x2000_module_base.dts"
```

可知当前dts文件为：x2000_module_base.dts



![2023-04-14_16-59](Darwin_X2000_添加客户自定义配置文件.assets/05.png)

**创建客户自己的.dts文件**

进入 kernel/kernel-x2000/arch/mips/boot/dts/ingenic 目录创建 .dts 文件，比如命名为：x2000_example.dts

```c
cp kernel/kernel-x2000/arch/mips/boot/dts/ingenic/x2000_module_base.dts kernel/kernel-x2000/arch/mips/boot/dts/ingenic/x2000_example.dts
```



 **kenel 使用创建的 x2000_example.dts 文件**



打开 menuconfig 更改配置文件

>   cd kernel/kernel-x2000
>
>   make x2000_module_base_example_linux_sfc_nand_defconfig
>
>   make menuconfig

目标位于

-> Machine selection

​    -> SOC Type Selection

![2](Darwin_X2000_添加客户自定义配置文件.assets/06.png)

将红色区域内文件名改成我们创建的文件名，然后保存退出

![3](Darwin_X2000_添加客户自定义配置文件.assets/07.png)

将 .config 配置保存，重新拷贝到我们创建的文件

>   kernel/kernel-x2000$ cp .config arch/mips/configs/x2000_module_base_example_sfc_nand_defconfig -arf



## 5 修改buildroot配置

**确定当前使用的buildroot配置**

使用IConfigTool工具选择上面创建的主配置文件



![2023-04-17_16-19](Darwin_X2000_添加客户自定义配置文件.assets/01.png)



点击Open, 打开`工程/编译` 配置

![6](Darwin_X2000_添加客户自定义配置文件.assets/02.png)

![2023-05-22_16-49](Darwin_X2000_添加客户自定义配置文件.assets/08.png)



或者通过以下命令：

```c
build$ grep -nr "buildroot" configs/x2000_example_factory_defconfig 
8:APP_br_dir=../buildroot/buildroot
9:APP_br_config_file=configs/buildroot/buildroot_x2000_darwin_debug
451:# APP_br_root_login_keep_buildroot is not set
```

可知，当前的buildroot 配置为：buildroot_x2000_darwin_debug

创建客户自己的buildroot配置，比如为：buildroot_x2000_example_darwin_debug

```c
build$ cp configs/buildroot/buildroot_x2000_darwin_debug configs/buildroot/buildroot_x2000_example_darwin_debug -arf
```



## 6 更新主配置文件

**重新选择客户创建的uboot,kernel ,buildroot配置**

使用IConfigTool工具选择上面创建的主配置文件



![2023-04-17_16-19](Darwin_X2000_添加客户自定义配置文件.assets/01.png)



点击Open, 打开`工程/编译` 配置

![6](Darwin_X2000_添加客户自定义配置文件.assets/02.png)

![2023-05-22_16-55](Darwin_X2000_添加客户自定义配置文件.assets/09.png)



![2023-05-22_17-01](Darwin_X2000_添加客户自定义配置文件.assets/10.png)



点击 Yes保存。这样主配置文件x2000_example_factory_defconfig里面使用的uboot,kernel,buildroot配置就都是客户自己添加的配置了，整个客户自定义的配置修改完成。

## 7 整体编译

**编译命令如下：**

build$ make x2000_example_factory_defconfig

build$ make