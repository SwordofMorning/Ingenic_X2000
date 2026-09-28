
# 确保.config.in 产生
make_sure_update_config_in := $(shell tools/check_config.sh)

# 导入当前的配置
sinclude .config.in

TOPDIR := $(PWD)
BR_DIR := $(TOPDIR)/$(APP_br_dir)
FS_HOST_DIR := $(BR_DIR)/output/host
FS_TARGET_DIR := $(BR_DIR)/output/target
FS_STAGING_DIR := $(BR_DIR)/output/staging

TOOLCHAIN_DIR := $(TOPDIR)/$(APP_toolchain_dir)/bin
UBOOT_TOOLCHAIN_DIR := $(TOPDIR)/$(APP_uboot_toolchain_dir)/bin
KERNEL_DIR := $(TOPDIR)/$(APP_kernel_dir)
MD_DIR := $(TOPDIR)/../module_driver
UBOOT_DIR := $(TOPDIR)/$(APP_uboot_dir)

CROSS_COMPILE=$(FS_HOST_DIR)/usr/bin/$(APP_br_toolchain_name)

config_in_file := $(TOPDIR)/.config.in

THREAD_ARG = -j$(APP_compile_threads)

# make V=1 可以看见Makefile运转的细节
ifneq ($(V),1)
Q = @
MAKE_ARG = --no-print-directory
else
Q =
MAKE_ARG =
endif

MAKE_ARG += BR_DIR=$(BR_DIR)
MAKE_ARG += FS_HOST_DIR=$(FS_HOST_DIR)
MAKE_ARG += FS_TARGET_DIR=$(FS_TARGET_DIR)
MAKE_ARG += FS_STAGING_DIR=$(FS_STAGING_DIR)
MAKE_ARG += V=$V CROSS_COMPILE=$(CROSS_COMPILE)
MAKE_ARG += config_in=$(config_in_file)
MAKE_ARG += $(THREAD_ARG)

project ?= Config.in
config ?= .config.in

menuconfig_project := $(project)
menuconfig_config := $(config)

all1: all
	@echo "no warning" > /dev/null

# 包含软件列表
include cmd_common.mk
include cmd_uboot.mk
include cmd_kernel.mk
include cmd_buildroot.mk
include cmd_module_driver.mk
include cmd_ota_updater.mk
include cmd_overlay_bootfs.mk
include product.mk

define  CP_UBOOT_IMAGE
	$(Q)rm -f ./output/u-boot*.bin
	$(Q)if [ -f $(UBOOT_DIR)/u-boot-with-spl-mbr-gpt.bin ]; then \
		cp -v $(UBOOT_DIR)/u-boot-with-spl-mbr-gpt.bin ./output; \
	else \
		cp -v $(UBOOT_DIR)/spl/u-boot-spl-pad.bin output/; \
	fi
endef

KERNEL_IMAGE_PATH=arch/mips/boot/compressed/xImage

ifdef APP_kernel_uImage
KERNEL_IMAGE_PATH:=arch/mips/boot/uImage
endif

ifdef APP_kernel_uImage_split
KERNEL_IMAGE_PATH:=arch/mips/boot/uImage_split
endif

define  CP_KERNEL_IMAGE
	$(Q)rsync -avL $(KERNEL_DIR)/$(KERNEL_IMAGE_PATH) output/
endef

define  CP_BUILDROOT_IMAGE
	$(Q)cp -v $(BR_DIR)/output/images/* output/
endef

all:
	$(CMD_MAKE_UBOOT)
	$(CMD_MAKE_BUILDROOT)
	$(CMD_MAKE_KERNEL)
	$(CMD_MAKE_APPS)
	$(CMD_MAKE_BUILDROOT)
	$(Q)mkdir -p output/
	$(CP_BUILDROOT_IMAGE)
	$(CP_KERNEL_IMAGE)
	$(CP_UBOOT_IMAGE)
	$(CMD_MAKE_OVERLAY_BOOTFS)
	$(CMD_MAKE_OTA_IMG)

clean:
	$(CMD_CLEAN_UBOOT)
	$(CMD_CLEAN_APPS)
	$(CMD_CLEAN_KERNEL)
	$(CMD_CLEAN_BUILDROOT)
	$(CMD_CLEAN_OVERLAY_BOOTFS)
	$(Q)rm -rf output/

apps:
	$(foreach package,$(packages),$(call CMD_MAKE,$(package))$(CMD_SEPARATOR))

clean_apps:
	$(foreach package,$(packages),$(call CMD_CLEAN,$(package))$(CMD_SEPARATOR))

buildroot:
	$(CMD_MAKE_BUILDROOT)
	$(CP_BUILDROOT_IMAGE)

clean_buildroot:
	$(CMD_CLEAN_BUILDROOT)

kernel:
	$(CMD_MAKE_KERNEL)
	$(CP_KERNEL_IMAGE)

clean_kernel:
	$(CMD_CLEAN_KERNEL)

uboot:
	$(CMD_MAKE_UBOOT)
	$(CP_UBOOT_IMAGE)

clean_uboot:
	$(CMD_CLEAN_UBOOT)

ota_img:
	$(CMD_MAKE_OTA_IMG)

overlay_bootfs:
	$(CMD_MAKE_OVERLAY_BOOTFS)

clean_overlay_bootfs:
	$(CMD_CLEAN_OVERLAY_BOOTFS)

set_kernel_configs:
	$(CMD_MAKE_SET_KERNEL_CONFIGS)

set_buildroot_configs:
	$(CMD_MAKE_SET_BUILDROOT_CONFIGS)

menuconfig:
	./tools/menuconfig project=$(menuconfig_project) defconfig=$(menuconfig_config)

# 生成 configs/目录下 *_defconfig 的规则
defconfigs := $(notdir $(wildcard configs/*_defconfig))

ifneq ($strip($(defconfigs)),)
$(defconfigs):
	$(Q)echo "..writing .config.in"
	$(Q)cp configs/$(@) .config.in
	$(Q)tools/check_config.sh
	$(Q)make set_kernel_configs
	$(Q)make set_buildroot_configs
	$(Q)echo "..writing include/config.h"

.PONHY: $(defconfigs)
endif

.PONHY:all clean buildroot kernel uboot apps set_kernel_configs set_buildroot_configs menuconfig
.PONHY:clean_apps clean_uboot clean_kernel clean_buildroot