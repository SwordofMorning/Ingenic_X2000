# Linux工程编译说明

## 1. 目录说明

```c
├── bootloader                                         /*各个平台的引导代码*/
├── build                                                     /*工程编译脚本*/
├── buildroot                                            /*文件系统*/
├── doc                                                       /*说明文档*/
├── kernel                                                  /*linux内核代码*/
├── libhardware2                                    /*驱动对应的应用接口代码*/
├── libutils2                                               /*应用小工具*/
├── module_driver                                 /*模块驱动代码*/
├── ota_updater                                      /*ota升级*/
├── tools
│   ├── burntools                                    /*烧录工具*/
│   ├── iconfigtool                                  /*配置工具*/
│   └── toolchains                                  /*交叉编译工具*/
└── wireless                                              /*无线网卡驱动*/
```

## 2. 工程编译流程

```shell
工程编译流程以x2000_nand_defconfig配置为例
cd build                                            # 切换工程编译目录
make x2000_nand_defconfig # 编译工程配置
make                                                 # 编译工程

执行make后在build/output目录下生成镜像文件u-boot-spl-pad.bin、xImage、rootfs.squashfs。如果编译是以mmc为存储介质的配置时，引导文件是在uboot目录下的u-boot-with-spl-mbr-gpt.bin文件。
将对应的镜像文件烧录开发板即可。

make clean                                       # 清除工程编译生成的所有文件
```

> make x2000_nand_defconfig 编译配置时kernel和buildroot目录下面的.config配置会立即更新，上一次的.config配置会保存在.config.save文件中。

## 3. 模块编译命令

> 执行工程编译流程后可执行以下命令
>
> 命令执行目录：build/ 目录下

### 3.1. 单独编译uboot

```shell
make uboot
作用：单独编译系统引导文件，并将生成引导文件u-boot-spl-pad.bin拷贝到output目录下（当编译以mmc为存储介质配置时，生成引导文件u-boot-with-spl-mbr-gpt.bin在uboot目录下）
make clean_uboot
作用：清除编译uboot生成的所有文件
```

### 3.2. 单独编译kernel

```shell
make kernel
作用：单独编译内核镜像文件，并将生成的内核镜像文件拷贝到output目录下
make clean_kernel
作用：清除编译内核生成的所有文件
```

### 3.3.单独编译buildroot

```shell
make buildroot
作用：单独编译文件系统，并将生成的文件系统拷贝到output目录下
make clean_buildroot
作用：清除编译文件系统生成的所有文件
```

### 3.4.单独编译app

```shell
make apps
作用：编译模块驱动module_driver以及库函数libhardware2、libutils2、libisp等
make clean_apps
作用：清除编译模块驱动以及库函数生成的所有文件

make apps、make app_module_driver、make app_libhardware2、make app_libisp、make app_libutils2等、后都需要执行make buildroot 命令将编译出来的文件打包到文件系统镜像中，并烧录到开发板
```

> 编译app应用程序以编译app_module_driver和app_libhardware2为例

#### 3.4.1.单独编译app_module_driver

```shell
make app_module_driver
作用：编译模块驱动并将模块安装脚本和ko文件拷贝到module_driver/output目录下。
make clean_app_module_driver
作用：清除编译模块驱动生成的所有文件
```

#### 3.4.2.单独编译app_libhardware2

```shell
make app_libhardware2
作用：编译libhardware2目录下的函数并将生成的命令和库拷贝到libhardware2/output目录下
make clean_app_libhardware2
作用：清除编译libhardware2生成的所有文件
```

### 3.5. 单独编译ota_img

```shell
make ota_img
作用：编译OTA升级文件系统
```



## 4. 修改配置文件

>   修改配置文件使用可视化配置工具IConfigTool

### 4.1 解压并打开配置工具

>   IConfigTool配置在工具在"tools/iconfigtool"目录下，解压后直接运行
>
>   如果IConfigTool出现闪退时，删除工具lib/目录下libQtCore.so.4与libQtGui.so.4文件

```shell
./IConfigTool
```

![](img/1.png)

Config.in是生成配置界面文件

>   build/Config.in

Config是需要修改的配置文件

>   build/configs/x2000_nand_deconfig

### 4.2 保存配置文件

```
修改模块功能配置文件时，请参考片上外设模块驱动文档。
修改完成后执行以下配置：
1．点击File选项
2．选择save进行保存
3．重新执行make x2000_nand_defconfig
4．执行make
```

