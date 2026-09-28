# X2000 添加 UART 打印

## 1.uboot 中添加串口 (若不需要uboot串口打印可略过该步骤)

1.查看 bootloader/uboot-x2000/drivers/gpio/jz_gpio/x2000_v12_gpio.c 文件中uart_gpio_func 结构体

**注意：该结构体中存在的 GPIO 端口才可用于 uboot 的串口打印，若需要使用 uart_gpio_func 中没有的 GPIO 端口进行 uboot 的串口打印，请联系君正添加**

![9](img/9.png)

如图，该文件中只存在 UART0_PD，UART1_PC，UART2_PD，UART3_PC，则只有这些端口可以用于 uboot 打印



2.打开需要添加 UART 的板级配置(这里以 x2000_halley5_nand_defconfig 为例)

![1](img/1.png)

3.打开`工程/编译` 配置

![2](img/2.png)

4.查看 uboot 默认配置

![3](img/3.png)

5.在文件 bootloader/uboot-x2000/boards.cfg 中找到 uboot 的默认配置

![4](img/4.png)

**在后面定义你需要添加打印的串口 ，例如是 UART3 就添加 SYS_UART_INDEX=3**



## 2.kernel 中添加串口

1.查看 kernel 默认配置

![6](img/6.png)

2.进入 kernel 目录打开 kernel 默认配置

>   cd kernel/kernel-x2000
>
>   make x2000_halley5_module_base_linux_sfc_nand_defconfig
>
>   make menuconfig

查看设备树文件名

-> Machine selection

​    -> SOC Type Selection

![7](img/7.png)

![8](img/8.png)



3.找到设备树文件 x2000_halley5_module_base.dts

文件路径 kernel/kernel-x2000/arch/mips/boot/dts/ingenic/x2000_halley5_module_base.dts

```c
&uart2 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&uart2_pd>;
};

&uart3 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&uart3_pc>;
};
```

>   status = "okay" 表示使能该串口
>
>   status = "disable" 表示失能该串口
>
>   pinctrl-0 = < > 中填写需要使用的串口端口
