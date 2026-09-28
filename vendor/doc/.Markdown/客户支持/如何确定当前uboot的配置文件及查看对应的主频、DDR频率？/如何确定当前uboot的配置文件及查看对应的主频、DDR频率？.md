  如何确定当前uboot的配置文件及查看对应的主频、DDR频率？

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

![4](如何确定当前uboot的配置文件及查看对应的主频、DDR频率？.assets/4.png)

```
sxyzhang@T430:~/my/work/linux/x2000_sz/bootloader/uboot-x2000$ vim include/configs/x2000_base.h
```

![6](如何确定当前uboot的配置文件及查看对应的主频、DDR频率？.assets/6.png)

3, 查看对应的主频、DDR频率

```
sxyzhang@T430:~/my/work/linux/x2000_sz/bootloader/uboot-x2000$ vim include/configs/x2000_base_common.h
```

![7](如何确定当前uboot的配置文件及查看对应的主频、DDR频率？.assets/7.png)

4, 自定义配置

PLL锁相环频率配置，可根据需要进行配置

```
#define CONFIG_SYS_APLL_FREQ            1200000000      /*If APLL not use mast be set 0*/
#define CONFIG_SYS_MPLL_FREQ            1500000000      /*If MPLL not use mast be set 0*/
#define CONFIG_SYS_EPLL_FREQ            300000000       /*If MPLL not use mast be set 0*/
```

CPU、DDR的PLL选择，建议使用默认配置

```
#define CONFIG_CPU_SEL_PLL              APLL
#define CONFIG_DDR_SEL_PLL              MPLL
```

CPU、DDR频率配置，其中CPU的频率需要与选择的PLL频率成倍数关系，同时DDR的频率也需要与选择的PLL频率成倍数关系。

```
#define CONFIG_SYS_CPU_FREQ             1200000000
#define CONFIG_SYS_MEM_FREQ             750000000
```

5, 查看当前烧录镜像的CPU及DDR频率方法一

查看系统启动log，有CPU及DDR频率展示

![8](如何确定当前uboot的配置文件及查看对应的主频、DDR频率？.assets/8.png)

6, 查看当前烧录镜像的CPU及DDR频率方法二

参考：如何查看当前系统的各个时钟设置及状态？