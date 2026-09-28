#
# sprdbt_tty.ko
#
# Kbuild: for kernel building external module
#
# Note:
# - Please refer to modules/sample/Kbuild to find out what should be
#   done is this Kbuild file
#

PWD := $(shell pwd)

ARCH ?= mips
CROSS_COMPILE ?= mips-linux-gnu-

BSP_BOARD_UNISOC_WCN_SOCKET :=sdio
src := $(abspath $(lastword $(MAKEFILE_LIST)/../))
UNISOC_BSP_INCLUDE := $(abspath $(lastword $(MAKEFILE_LIST)/../../unisocwcn/include))

ifeq ($(filter $(strip $(BSP_BOARD_UNISOC_WCN_SOCKET)),sipc sdio pcie sipc2),)
$(error no BSP_BOARD_UNISOC_WCN_SOCKET, pls fix it !!!)
endif

TTY_DEVICE := tty-$(strip $(BSP_BOARD_UNISOC_WCN_SOCKET))

#
# Source List
#
KO_MODULE_NAME := fcs950uab_xbt
KO_MODULE_PATH := $(src)/$(TTY_DEVICE)
KO_MODULE_SRC  :=

KO_MODULE_SRC += \
	$(wildcard $(KO_MODULE_PATH)/*.c) \
	$(wildcard $(KO_MODULE_PATH)/alignment/*.c)

#
# Build Options
#
ccflags-y += -DDEBUG
ccflags-y += -I$(UNISOC_BSP_INCLUDE)
#
# Final Objects
#
obj-m := $(KO_MODULE_NAME).o
# Comment it if the only object file has the same name with module
$(KO_MODULE_NAME)-y := $(patsubst $(src)/%.c,%.o,$(KO_MODULE_SRC))

KBUILD_EXTRA_SYMBOLS := $(abspath $(lastword $(MAKEFILE_LIST)/../../unisocwcn/Module.symvers))

all: driver

driver:
	@echo build driver
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	cd tty-sdio ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	rm -fr Module.symvers ; rm -fr Module.markers ; rm -fr modules.order
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -fr .tmp_versions
