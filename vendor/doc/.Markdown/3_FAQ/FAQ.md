# FAQ

##### Q1：如何编译Linux工程？

A1：请查阅《1_Linux工程编译说明.pdf》

##### Q2：如何单独编译uboot、kernel、buildroot？

A2：请查阅《1_Linux工程编译说明.pdf》

##### Q3：如何烧录编译好的镜像？

A3：请查阅《2_Linux工程烧录介绍.pdf》

##### Q4：kernel目录下的kernel-x1000、kernel-x1021、kernel-x2000有什么区别？

A4：soc-x1000、soc-x1520、soc-x1830使用的是kernel-x1000；soc-x1021使用的是kernel-x1021；soc-x2000使用的是kernel-x2000。

同理，soc-x1000、soc-x1520、soc-x1830使用的是uboot-x1000；soc-x1021使用的是uboot-x1021；soc-x2000使用的是uboot-x2000。

##### Q5：如何配置kernel添加驱动？

A5：在已经编译过 kernel 的情况下，进入工程下的 kernel/kernel-xXXXX 目录(XXXX代指不同的soc，如kernel-x1000)，执行 make menuconfig 。根据需要选择、移除对应的驱动，然后保存退出。重新编译并烧录到开发板。

> 未编译过 kernel 的情况需提前编译，具体操作请查阅《1_Linux工程编译说明.pdf》。

##### Q6：如何生成自己的kernel配置？

A6：用户在修改了 kernel 配置后，可以在kernel/kernel-xXXXX 目录下（XXXX代指不同的soc，如kernel-x1000），执行 *cp  .config arch/mips/configs/your_defconfig* 命令，生成自己的 kernel 配置。

##### Q7：如何配置buildroot添加arecord/aplay应用？

A7：在已经编译过 buildroot 的情况下，进入工程下的 buildroot/buildroot 目录，执行 *make menuconfig* 。根据下图路径，找到并选择 aplay/arecord 应用，保存退出。重新编译并烧录到开发板，通过 ls 命令可以在开发板 /usr/bin/ 目录下找到aplay/arecord。

> 未编译过 buildroot 的情况需提前编译，具体操作请查阅《1_Linux工程编译说明.pdf》。

<img src="img/8.png" style="zoom:67%;" />

![](img/9.png)



##### Q8：如何生成自己的buildroot配置？

A8：用户在修改了 buildroot 配置后，可以在buildroot/buildroot 目录下，执行 *cp .config ../../build/configs/buildroot/your_defconfig* 命令，生成自己的 buildroot 配置。

##### Q9：如何调整分区表大小？

A9：通过烧录工具修改SFC面板中分区信息，根据实际情况修改偏移和大小。

<img src="img/10.png" style="zoom: 80%;" />

<img src="img/11.png" style="zoom: 80%;" />



##### Q10：烧录失败，发现镜像文件太大该如何处理？

A10：对于这种情况一般有如下两种解决方法：1、调整分区表大小以适应镜像文件大小；2、裁剪镜像大小，一般裁剪kernel或buildroot；

##### Q11：修改分区表大小后，仍然显示选择的文件超出分区大小？

A11：除了保证镜像文件大小不超出分区大小之外，还需要保证烧录工具POLICY中的offset偏移大小与SFC中的分区信息的偏移大小保持一致。

<img src="img/6.png" style="zoom: 67%;" />

<img src="img/7.png" style="zoom: 67%;" />

##### Q12：上电后，串口没有打印或打印出现乱码是什么情况？

A12：本工程使用的波特率是3000000或115200。修改波特率后无效，可能是串口没有配置正确，请查阅《串口配置说明文档.pdf》。

##### Q13：上电之后，kernel 起不来，该如何解决？

<img src="img/14.png" style="zoom: 67%;" />

<img src="img/2.png" style="zoom: 65%;" />

A13：一般 kernel 引导失败，有几种情况：

情况1、系统只打印 uboot 信息。出现这种情况可能是用户把 uImage 当成 xImage 烧录到开发板，替换正确的 xImage 烧录到开发板即可；

情况2、与情况1相同，系统只打印 uboot 信息。出现这种情况可能是烧录工具中 kernel 的偏移没有设置正确，参考 Q15 解决；

情况3、系统打印 invalid compressed format（err=2），尝试 *make clean_kernel*，再重新编译并烧录 kernel 。

##### Q14：上电后，为什么看不到驱动的打印信息？

A14：这是因为默认情况下，quiet参数是打开的，可以使用 dmesg 查看 kernel 打印信息。在开发阶段也可以通过去除工程中bootloader/uboot-xXXXX/boards.cfg（XXXX代指对应的soc）对应板级的宏ARG_QUIET 达到查看所有 kernel 打印信息的目的。

##### Q15：上电后不断重启，打印No filesystem could mount root, tried:  squashfs，该如何处理？

<img src="img/3.png" style="zoom: 50%;" />

A15：请确保烧录工具中rootfs的Manage_mode(管理模式)是MTD_MODE且分区的位置是否正确（ /dev/mtdblock_bbt_ro2 对应烧录工具分区信息的第3项），如下图所示：

<img src="img/12.png" style="zoom: 67%;" />



<img src="img/13.png" style="zoom:67%;" />

##### Q16：如何探测 i2c 设备的地址？

A16：使用 *cmd_i2c detect <bus num>* 命令，具体参考《Linux辅助开发.pdf》；

##### Q17：如何添加IConfig tools配置选项？

A17：请参考《IConfig配置选项添加说明文档.pdf》。

##### Q18：如何添加一个用户自己的模块驱动？

A18：请参考《模块驱动添加流程.pdf》。

##### Q19：如何挂载NFS网络文件系统？

A19：请参考《NFS文件系统挂载.pdf》。

##### Q20：如何给板子联网？

A20：无线联网方式请参考bt&wifi使用说明文档目录下的《XXX bt&wifi使用说明文档.pdf》（XXX代指不同的产商的无线模块）;

以太网联网方式可以用 ifconfig 命令进行联网，具体操作这里不作阐述。

##### Q21：如何使用ota功能？

A21：请参考《ota升级操作说明文档.pdf》。

##### Q22：如何在Linux工程中使用spi模块驱动进行通信？

A22：方式1、使用 cmd_spi 命令，具体参考《Linux辅助开发.pdf》；

​		  方式2、参考 *devices/lcd/x1830/lcd_st7701/lcd_st7701_data.c* 和*devices/Ethernet/ax88796c_spi/ax88796c_spi_dev.c* 中 spi_register_device 的用法。

##### Q23：如何使用ov9732摄像头采集图像？

A23、请参考《Linux工程中ov9732摄像头使用流程》。

##### Q24：如何添加一款camera sensor到模块驱动工程中？

A24：请参考Camera使用以及添加说明文档中的《Camera_Sensor xXXXX使用以及添加说明文档.pdf》（XXXX代指对应的soc）。

##### Q25：x1520加载摄像头模块，系统打印  camera: failed to alloc mem: 5529600 信息，该如何处理？

A25：在串口或ADB终端输入：cat /proc/cmdline，查看cmdline信息中是否缺少rmem信息。如果缺少rmem信息，需要在bootloader/uboot-xxxx/boards.cfg对应的板级添加RMEM_xM宏,x表示数值大小。具体rmem的数值大小根据实际情况填写，如x1520建议填写RMEM_6M。

```
x1520_new_xImage_nand mips xburst x1520_new ingenic x1520x 1520_new:SPL_SFC_NAND,MTD_SFCNAND,SPL_OS_OTA_BOOT,SPL_SERIAL_SUPPORT,RMEM_6M
```

##### Q26：如何禁止应用或驱动开机自启？

A26：用户可以通过修改 buildroot/buildroot/output/target/etc/init.d/ 目录下的开机启动脚本权限禁止开机启动，如：*chmod -x buildroot/buildroot/output/target/etc/init.d/S11module_driver_default* 可以关闭模块驱动的自动加载，重新 *make buildroot* 并烧录即可。反之，用户也可以通过在buildroot/buildroot/output/target/etc/init.d/ 目录添加启动脚本实现开机自启的目的。

##### Q27：在nor flash的板子上如何挂载userdata分区？

A27：具体操作可以参考《userdata分区挂载说明文档.pdf》。

##### Q28：如何在只读文件系统中进行写操作？

A28：只读文件系统不支持写操作，用户可以通过挂载userdata分区进行写操作。具体操作可以参考《userdata分区挂载说明文档.pdf》。

##### Q29：如何在开发板调试用户自己的模块驱动？

A29：在调试阶段，用户可以通过 adb 将编译生成的模块驱动 push到开发板的 /usr/data/ 或 /tmp 目录下进行调试， *adb push your_module.ko /usr/data/* 或  *adb push your_module.ko /tmp/* 。

##### Q30：使用adb push模块到开发板/usr/data/目录时打印错误信息：*adb: error: failed to copy 'your_module.ko' to '/usr/data/your_module.ko': remote Read-only file system*，该如何处理？

A30：出现这个原因可能是userdata分区挂载失败，请参考《userdata分区挂载说明文档.pdf》。

##### Q31：如何在开发板调试用户自己的应用？

A31：在调试阶段，用户可以通过 adb 将编译生成的应用 push到开发板的 /usr/data/ 或 /tmp 目录下进行调试， *adb push your_app /usr/data/* 或  *adb push your_app /tmp/* 。

如果是修改了 libhardware2 库或者生成自己动态库的用户同样可以将动态库 push 到开发板的  /usr/data/ 或 /tmp 目录，此时需要指定动态库路径，这里提供两种方式:

方式1、用户在串口或adb终端直接设置 LD_LIBRARY_PATH 环境变量， *export LD_LIBRARY_PATH=/usr/data/* 或 *export LD_LIBRARY_PATH=/tmp*；

方式2、在编译应用时加入编译选项 *-Wl,-rpath=/usr/data/* 或 *-Wl,-rpath=/tmp/*

##### Q32：编译内核打印类似如下错误：scripts/extract-cert.c:21:25: fatal error: openssl/bio.h: No such file or directory，该如何处理？

A32：出现这个的原因是缺少依赖库，使用如下命令解决 sudo apt-get install libssl-dev

##### Q33：使用LCD模块和rotator模块时打印错误信息如下，该如何处理？

```
[   25.926667] Call Trace:
[   25.926834] [<800b505c>] dma_alloc_attrs+0x34/0xf8
[   25.927159] [<c0ceb468>] rmem_alloc_aligned+0x190/0x268 [rmem_manager]
[   25.927596] [<c0ceb744>] rmem_ioctl+0xac/0x178 [rmem_manager]
[   25.927984] [<80159b90>] sys_ioctl+0x130/0x8a0
[   25.928283] [<8002a2b8>] syscall_common+0x34/0x58
```

A33：出现这个问题可能是 uboot 配置的文件中的 RMEM_MB 参数设置较小，请检查所使用项目中的 bootloader/uboot-对应soc型号/boards.cfg 文件的对应 uboot 配置项的 RMEM_MB 参数大小，根据实际采用的硬件，进行适当调整。项目具体对应的 uboot 配置项可通过 IConfigTool 的工程/编译配置下的 uboot 默认配置 进行查看。

计算方式：正常显示需要的内存大小 = 所用显示屏分辨率 ×（IConfigTool中开启的缓冲数）× 所用色彩模式下1个像素点占用内存大小

example：硬件采用LCD_FW050，分辨率为720*1280，在 IConfigTool 中开启 fb0 和 fb1，且缓冲数都设置为2，色彩模式为ARGB，LCD正常显示需使用的内存大小为720×1280×(2+2)×4 = 14745600 byte = 14.0625 MB，此时的预留内存必须大于14MB。