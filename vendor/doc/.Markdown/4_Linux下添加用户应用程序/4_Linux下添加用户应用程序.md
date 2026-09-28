# 				4_Linux下添加用户应用程序

## 一 背景介绍

很多客户拿到深圳君正的代码及开发板之后，编译烧录，简单调试，性能评估，最终决定在此基础之上进行开发自己的产品。那么避免不了的就需要在深圳君正的代码基础上添加自己的应用程序，加入到配置选项、编译、运行调用等环节。 

本文以PD_X2600E_VAST_V2.0开发板为例详述此过程，编译配置为：x2600e_vast_v20_nand_5.10_factory_defconfig。

本文涉及的用户应用程序的添加方法适用于深圳君正linux平台下的所有主控芯片，针对不同的主控芯片，编译配置文件、烧录配置文件等都要对应的修改。

## 二 软件实现

一般建议客户将自己的应用开发程序放在根目录的demos/下，此处以demos/hello_world/为例，详述软件实现过程

### 2.1 添加本应用程序的tools目录

tools目录涉及统一的编译规则，建议将其他demos工程的tools直接拷贝到自己的应用目录下。

```c
sxyzhang@T430:~/my/work/linux/x2670_sz/demos$ mkdir hello_world
sxyzhang@T430:~/my/work/linux/x2670_sz/demos$ mkdir hello_world/tools   
sxyzhang@T430:~/my/work/linux/x2670_sz/demos$ cp faceapp/tools/* hello_world/tools/
sxyzhang@T430:~/my/work/linux/x2670_sz/demos$ ll hello_world/tools/
总用量 140
drwxrwxr-x 2 sxyzhang sxyzhang   4096 12月 21 17:17 ./
drwxrwxr-x 3 sxyzhang sxyzhang   4096 12月 21 17:17 ../
-rw-rw-r-- 1 sxyzhang sxyzhang   3484 12月 21 17:17 build_elf.mk       # 添加统⼀的编译规则（使⽤⻅下⽂）
-rwxrwxr-x 1 sxyzhang sxyzhang    816 12月 21 17:17 check_config.sh*
-rwxrwxr-x 1 sxyzhang sxyzhang 125423 12月 21 17:17 configparser*
```

### 2.2 添加本应用程序的Config.in文件

添加IconfigTool的配置选项，demos/hello_world/Config.in 该如何编写详细⻅《IConfig配置选项添加说明⽂档》

```c
sxyzhang@T430:~/my/work/linux/x2670_sz/demos$ touch hello_world/Config.in
```

```c
sxyzhang@T430:~/my/work/linux/x2670_sz/demos$ cat hello_world/Config.in
config APP_HELLO_WORLD
    bool "HelloWorld (你好世界)"
    default n
```

具体请根据代码需求编写。

### 2.3 添加本应用程序的Makefile

其中hello_world.mk 为用户应用程序自己规则文件。

```c
sxyzhang@T430:~/my/work/linux/x2670_sz/demos$ touch hello_world/Makefile
```

```makefile
sxyzhang@T430:~/my/work/linux/x2670_sz/demos$ cat hello_world/Makefile 
# 检查 .config.in include/config.h 是否有更新
make_sure_config_update:=$(shell tools/check_config.sh $(config_in) 2> /dev/null)

# 将规则定义的配置选项引用进来，如 APP_HELLO_WORLD
sinclude .config.in

# 由上层makefile传递进来
export V
export Q
export CROSS_COMPILE
# 没有压缩的文件系统路径
export FS_TARGET_DIR
# 存放构建过程中的中间文件，如交叉编译工具链和根文件系统的一些工具等
export FS_STAGING_DIR

# make V=1 可以看见Makefile运转的细节
ifneq ($(V),1)
Q = @
MAKE_ARG = --no-print-directory
else
Q =
MAKE_ARG =
endif

# hello_world.mk 为用户应用程序自己规则文件
all:
	$(Q)+make $(MAKE_ARG) -f hello_world.mk all

clean:
	$(Q)+make $(MAKE_ARG) -f hello_world.mk clean

clean_install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(if $(FS_STAGING_DIR),,$(error must set FS_STAGING_DIR))
	$(Q)+make $(MAKE_ARG) -f hello_world.mk clean_install

install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(if $(FS_STAGING_DIR),,$(error must set FS_STAGING_DIR))
	$(Q)+make $(MAKE_ARG) -f hello_world.mk install

.PHONY: all clean install
```

### 2.4 添加本应用程序的具体编译规则

上面提到hello_world.mk 为用户应用程序自己规则文件，具体内容如下，客户可根据自己的代码情况删减链接库、编译参数、编译源文件。

```makefile
sinclude .config.in

#
# 定义默认目标
#
all: all_targets

#
# 客户可以在此添加编译参数
#
CFLAGS = -Os -Wall
CFLAGS += -Iinclude/
CFLAGS += -Wno-pointer-sign
CFLAGS += -Wno-pointer-to-int-cast

obj_dir = .objs/

# ----------------------
# 编译 hello_world，客户可以在此处添加编译链接的库及源文件
# ----------------------
LDFLAGS =
LDFLAGS += -Wl,-rpath=./
LDFLAGS += -lutils2 -lhardware2
LDFLAGS += -lm -Wl,--gc-sections

src-y += main.c

module_name = output/hello_world
include tools/build_elf.mk

# ---------------------------
# the end
# ---------------------------

all_targets: $(module_targets)
	@echo "  compiled $(all_modules)"

app = $(filter-out %.so,$(all_modules))

ifneq ($(app),)
define install_app
	$(Q)cp -rf $(app) $(FS_TARGET_DIR)/usr/bin/

	@echo "  installed $(app)"
endef
define clean_install_app
	$(Q)rm -rf $(addprefix $(FS_TARGET_DIR)/usr/bin/, $(notdir $(app)))

	@echo "  removed $(app)"
endef
endif

install:
	$(install_app)

clean_install:
	$(clean_install_app)

clean:
	$(Q)rm -rf output/
	$(Q)rm -rf $(FS_TARGET_DIR)/usr/bin/$(app)

.PHONY: all clean all_targets clean_install install
```

### 2.5 添加本应用程序的main.c

```c
sxyzhang@T430:~/my/work/linux/x2670_sz/demos$ touch hello_world/main.c
```

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	while(1)
	{
		printf("##\n");
		usleep(5000);
		printf("@@\n");
        usleep(5000);
	}
}
```

### 2.6 在build/product.mk添加编译

在根⽬录的build下的product.mk末尾中添加如下，添加makefile规则，规则名称为
demos/hello_world ：

```c
sxyzhang@T430:~/my/work/linux/x2670_sz$ vim build/product.mk
```

```makefile
package_name-$(APP_HELLO_WORLD) = demos/hello_world
include tools/package_common.mk
```

### 2.7 在build/Config.in添加IconfigTool的配置选项

上述 APP_HELLO_WORLD 由配置选项Config.in进⾏定义，IConfigTool进⾏配置⽣成，在build⽬录下
需要包含对应⼦⼯程的Config.in的⽂件，修改如下：

```c
sxyzhang@T430:~/my/work/linux/x2670_sz$ vim build/Config.in
```

```c
menu "demos"
sub_package ../demos/faceapp/Config.in
sub_package ../demos/evol/porting/Config.in
sub_package ../demos/ilockapp/Config.in
sub_package ../demos/scancode/Config.in
sub_package ../demos/ingenic_alg_lib/Config.in
sub_package ../demos/tuya/Config.in
# 新添加的⽂件
sub_package ../demos/hello_world/Config.in
endmenu
```

## 三 配置选项

至此，代码添加流程已完成。具体用法如下：

![1](4_Linux下添加用户应用程序.assets/1.png)

![2](4_Linux下添加用户应用程序.assets/2.png)

![3](4_Linux下添加用户应用程序.assets/3.png)

## 四 编译和烧录

```c
sxyzhang@T430:~/my/work/linux/x2670_sz/build$ make x2600e_vast_v20_nand_5.10_factory_defconfig
sxyzhang@T430:~/my/work/linux/x2670_sz/build$ make
```

编译后生成如下可供烧录的文件：

```c
sxyzhang@T430:~/my/work/linux/x2670_sz/build$ ll output/
总用量 18120
drwxrwxr-x 2 sxyzhang sxyzhang     4096 12月 21 19:53 ./
drwxrwxr-x 6 sxyzhang sxyzhang     4096 12月 21 19:50 ../
-rw-r--r-- 1 sxyzhang sxyzhang 13570048 12月 21 19:53 rootfs.squashfs
-rw-rw-r-- 1 sxyzhang sxyzhang    24576 12月 21 19:53 u-boot-spl-pad.bin
-rw-rw-r-- 1 sxyzhang sxyzhang  4948032 12月 21 19:52 xImage
```

<img src="4_Linux下添加用户应用程序.assets/4.png" alt="4" style="zoom:150%;" />

<img src="4_Linux下添加用户应用程序.assets/5.png" alt="5" style="zoom: 150%;" />

## 五 运行调用

烧录之后进入串口终端，执行如下命令，执行情况如下，可见与2.5章节的main.c相对应。

```c
# hello_world 
##
@@
##
@@
##
@@
##
```

## 六 不用IconfigTool来配置时的编写方式

上文第一～五章节展示的是在IconfigTool中选配客户应用程序的实现方法。客户也可以在代码中默认加载自己的应用程序，不用选择配置。那么实现方法就简单了，只需做如下操作：

按照上述2.1 添加本应⽤程序的tools⽬录

按照上述2.3 添加本应⽤程序的Makefile

按照上述2.4 添加本应⽤程序的具体编译规则

按照上述2.5 添加本应⽤程序的main.c

在build/product.mk添加编译：

在根⽬录的build下的product.mk末尾中添加如下，添加makefile规则，规则名称为
demos/hello_world ：

```c
sxyzhang@T430:~/my/work/linux/x2670_sz$ vim build/product.mk
```

```makefile
package_name-y = demos/hello_world
include tools/package_common.mk
```

如此就不需要在IConfigTool中选择是否配置，默认就编译进代码中。编译和烧录及使用方法同上，在此不再赘述。



