# 如何确定当前编译的uboot、kernel目录？

我们拿到一份代码之后按照文档在build目录下执行make XX_defconfig， 那么最终编译的是bootlader、kernel下的哪个uboot和kernel呢？

<img src="如何确定当前编译的uboot、kernel目录？.assets/Uboot_目录区分.png" alt="Uboot_目录区分" style="zoom:150%;" />

<img src="如何确定当前编译的uboot、kernel目录？.assets/Kernel_目录选择.png" alt="Kernel_目录选择" style="zoom:150%;" />

此时，应该打开上面命令中的XX_defconfig文件来查看，有详细的uboot、kernel、buildroot等的编译路径及配置文件。以x1600_nand_defconfig举例说明：

查看编译中uboot相关配置：cat build/configs/x1600_nand_defconfig | grep uboot

![19](如何确定当前编译的uboot、kernel目录？.assets/19.png)

查看编译中kernel相关配置：cat build/configs/x1600_nand_defconfig | grep kernel

![20](如何确定当前编译的uboot、kernel目录？.assets/20.png)

更多信息则查看：cat build/configs/x1600_nand_defconfig