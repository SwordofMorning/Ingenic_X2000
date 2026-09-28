export packages
export Q
export obj_dir

export root.COMMON_FLAGS
export root.CFLAGS
export root.CXXFLAGS
export root.ASMFLAGS
export root.LDFLAGS
export root.TARGET_LDFLAGS
export root.TARGET_LIBS

export root.COMMON_FLAGS-y
export root.CFLAGS-y
export root.CXXFLAGS-y
export root.ASMFLAGS-y
export root.LDFLAGS-y
export root.TARGET_LDFLAGS-y
export root.TARGET_LIBS-y

export CROSS_COMPILE

export CC
export CXX
export LD
export AR
export OBJCOPY
export OBJDUMP

export CFLAGS
export CXXFLAGS
export ASMFLAGS
export LDFLAGS

# make V=1 可以看见Makefile运转的细节
ifneq ($(V),1)
Q = @
MAKE_ARGS = --no-print-directory
else
Q =
MAKE_ARGS =
endif

# 如果当前的目标是 xxx_defconfig 那么不打印inlcude 信息
ifneq ($(filter %_defconfig, $(MAKECMDGOALS)),)
no_including_info = y
endif

ifneq ($(filter clean%, $(MAKECMDGOALS)),)
root.private.target = clean_ALL
else
root.private.target = ALL
endif

project ?= Config.in
config ?= .config.in

menuconfig_project := $(project)
menuconfig_config := $(config)

# .o 文件目录
obj_dir := .objs/

# 检查 .config.in include/config.h 是否有更新
make_sure_config_update:=$(shell tools/check_config.sh $(config_in))

include tools/msg.mk

# 引入配置文件
sinclude .config.in

# 默认目标
all: link_all

clean: clean_all

# 生成 configs/目录下 *_defconfig 的规则
defconfigs := $(notdir $(wildcard configs/*_defconfig))

ifneq ($(strip $(defconfigs)),)
$(defconfigs):
	$(Q)echo "..writing .config.in"
	$(Q)cp configs/$(@) .config.in
	$(Q)tools/check_config.sh
	$(Q)echo "..writing include/config.h"

.PONHY: $(defconfigs)
endif

# 工程
root.COMMON_FLAGS += -Iinclude/ -include config.h

include options/Makefile

# 定义需要加入编译的Makefile
package-y += soc/
package-$(APP_libmcu_lib) += lib/
package-$(APP_libmcu_device) += devices/
package-$(APP_libmcu_driver) += drivers/
package-y += newlib/
package-y += third_party/
package-y += vendor/

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# 递归引入需要加入编译的Makefile
$(foreach file,$(package-y),$(eval package:=$(file))$(eval include tools/include_package.mk))

# 最终得到如下的编译参数
# root.CFLAGS 会在所有的Makefile中加入到CFLAGS
# 各自Makefile中添加的 CFLAGS 会在各自的Makefile起效
root.COMMON_FLAGS += $(root.COMMON_FLAGS-y)
root.CFLAGS += $(root.CFLAGS-y)
root.CXXFLAGS += $(root.CXXFLAGS-y)
root.ASMFLAGS += $(root.ASMFLAGS-y)
root.LDFLAGS += $(root.LDFLAGS-y)
root.TARGET_LDFLAGS += $(root.TARGET_LDFLAGS-y)
root.TARGET_LIBS += $(root.TARGET_LIBS-y)

CFLAGS := $(root.COMMON_FLAGS) $(root.CFLAGS)
CXXFLAGS := $(root.COMMON_FLAGS) $(root.CXXFLAGS)
ASMFLAGS := $(root.COMMON_FLAGS) $(root.ASMFLAGS) -D__ASSEMBLY__
LDFLAGS := $(root.LDFLAGS)
TARGET_LDFLAGS := $(root.TARGET_LDFLAGS)
TARGET_LIBS := $(root.TARGET_LIBS)

CXXFLAGS := $(filter-out -Wstrict-prototypes -std=gnu99,$(CXXFLAGS))

TARGET_NAME := libmcu-bare

package_objs := $(patsubst %,%built-in.o,$(package-y))

ifneq ($(strip $(package-y)),)
$(package-y):
	$(Q)+make $(MAKE_ARGS) -f tools/build_package.mk D=$@ package_name=$@built-in.o $(root.private.target)
	$(Q)+make $(MAKE_ARGS) -f tools/build_package.mk D=$@ package_name=$@built-in.o clean_DEP
endif

link_all: $(package-y)
	@echo $(MSG_link) $(TARGET_NAME).elf
	$(Q)$(LD) $(TARGET_LDFLAGS) $(package_objs) $(TARGET_LIBS) -Map $(TARGET_NAME).map -o $(TARGET_NAME).elf
	@echo $(MSG_binary) $(TARGET_NAME).bin
	$(Q)$(OBJCOPY) -O binary $(TARGET_NAME).elf $(TARGET_NAME).bin
	$(Q)$(OBJDUMP) -d $(TARGET_NAME).elf > $(TARGET_NAME).S

clean_all: $(package-y)
	@echo $(MSG_clean) $(TARGET_NAME).elf $(TARGET_NAME).bin
	$(Q)rm -f $(TARGET_NAME).elf $(TARGET_NAME).bin $(TARGET_NAME).S

menuconfig:
	./tools/menuconfig project=$(menuconfig_project) defconfig=$(menuconfig_config)

.PHONY: all clean link_all clean_all $(package-y) menuconfig
