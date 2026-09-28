# Kernel 内添加设备树(.dts)文件

以下内容以 x2000_halley5 为参照

## 1.创建 .dts 文件

进入 kernel/kernel-x2000/arch/mips/boot/dts/ingenic 目录创建 .dts 文件并填写相关参数 (拷贝x2000_halley5

_module_base.dts)

![1](img/1.png)



## 2.打开 kernel 中 menuconfig 填写需要使用的 .dts 文件

1.拷贝 x2000_halley5 板级配置 x2000_halley5_nand_defconfig 为 x2000_example_nand_defconfig

打开相应的板级配置

![5](img/5.png)

2.打开`工程/编译` 配置

![6](img/6.png)

3.查看 kernel 默认配置

![7](img/7.png)

4.打开 menuconfig 更改配置文件

>   cd kernel/kernel-x2000
>
>   make x2000_halley5_module_base_linux_sfc_nand_defconfig
>
>   make menuconfig

目标位于

-> Machine selection

​    -> SOC Type Selection

![2](img/2.png)

5.将红色区域内文件名改成我们创建的文件名，然后保存退出

![3](img/3.png)

6.将 .config 配置保存

>   cp   .config   arch/mips/configs/x2000_example_linux_sfc_nand_deconfig



## 3.使用配置文件

1.修改 kernel 默认配置为我们保存的配置

![4](img/4.png)

2.保存退出

