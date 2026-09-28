PWD := $(shell pwd)

#
# Module name
#
COUSTOM_PLATFORM := ingenic

ARCH ?= mips
CROSS_COMPILE ?= mips-linux-gnu-

EXTRA_CFLAGS += -Wno-unused-variable
EXTRA_CFLAGS += -D__linux__

UNISOC_WCN_CHIP_ID := uwe5622
UNISOC_WCN_HW_TYPE := sdio
KO_MODULE_NAME := fcs950uab_$(UNISOC_WCN_CHIP_ID)_$(UNISOC_WCN_HW_TYPE)

#
# Build Options
#
# ------ coustom platform config start ------
ifeq ($(COUSTOM_PLATFORM), unisoc)
MARLIN_CFLAGS += -DCONFIG_UNISOC_BOARD
endif

ifeq ($(COUSTOM_PLATFORM), ingenic)
# MARLIN_CFLAGS += -DCONFIG_DEBUG
MARLIN_CFLAGS += -DCONFIG_THIRD_PARTY_BOARD
endif

ifeq ($(COUSTOM_PLATFORM), qcom)
MARLIN_CFLAGS += -DCONFIG_THIRD_PARTY_BOARD
MARLIN_CFLAGS += -DCONFIG_QCOM_BOARD
endif

ifeq ($(COUSTOM_PLATFORM), rk)
MARLIN_CFLAGS += -DCONFIG_THIRD_PARTY_BOARD
MARLIN_CFLAGS += -DCONFIG_RK_BOARD
endif

ifeq ($(COUSTOM_PLATFORM), mtk)
MARLIN_CFLAGS += -DCONFIG_THIRD_PARTY_BOARD
MARLIN_CFLAGS += -DCONFIG_MTK_BOARD
endif
# ------ coustom platform config end ------

# ------ wcn chip id config start ------
ifeq ($(UNISOC_WCN_CHIP_ID), umw2652)
# CONFIG_WCN_GNSS := y
# add macro for build source files
MARLIN_CFLAGS += -DCONFIG_UMW2652
# MARLIN_CFLAGS += -DCONFIG_WCN_GNSS
endif

ifeq ($(UNISOC_WCN_CHIP_ID), umw2653)
CONFIG_UMW2653 = y
MARLIN_CFLAGS += -DCONFIG_UMW2653
endif

ifeq ($(UNISOC_WCN_CHIP_ID), uwe5621)
CONFIG_UMW2653 = y
MARLIN_CFLAGS += -DCONFIG_UMW2653
endif

ifeq ($(UNISOC_WCN_CHIP_ID), uwe5622)
CONFIG_UMW2652 = y
MARLIN_CFLAGS += -DCONFIG_UMW2652
endif

ifeq ($(UNISOC_WCN_CHIP_ID), uwe5623)
CONFIG_UMW2653 = y
MARLIN_CFLAGS += -DCONFIG_UMW2653
endif
# ------ wcn chip id config end ------

# ------ wcn chip id config start ------
ifeq ($(UNISOC_WCN_HW_TYPE), sdio)
CONFIG_WCN_SDIO = y
CONFIG_WCN_SLP = y
CONFIG_WCN_SWD = y
CONFIG_WCN_UTILS ?= y
# add macro for build source files
MARLIN_CFLAGS += -DCONFIG_SDIO_BLKSIZE_512
MARLIN_CFLAGS += -DCONFIG_WCN_SDIO
MARLIN_CFLAGS += -DCONFIG_WCN_SWD
MARLIN_CFLAGS += -DCONFIG_MEM_PD
MARLIN_CFLAGS += -DCONFIG_WCN_SLP
endif

ifeq ($(UNISOC_WCN_HW_TYPE), usb)
CONFIG_WCN_USB = y
CONFIG_WCN_SWD = y
CONFIG_WCN_UTILS = y
# add macro for build source files
MARLIN_CFLAGS += -DCONFIG_WCN_USB
MARLIN_CFLAGS += -DCONFIG_WCN_SWD
#MARLIN_CFLAGS += -DCONFIG_WCN_USB_CH25_USE_WORKQUEUE
#MARLIN_CFLAGS += -DCONFIG_WCN_USB_ALL_IN_ONE_TASKLET
MARLIN_CFLAGS += -DCONFIG_WCN_USB_USE_THREAD
MARLIN_CFLAGS += -DCONFIG_BUF_LIST_CHECK
endif

ifeq ($(UNISOC_WCN_HW_TYPE), pcie)
# TODO
CONFIG_WCN_PCIE = y
# add macro for build source files
MARLIN_CFLAGS += -DCONFIG_WCN_PCIE
endif
# ------ wcn chip id config end ------

# ------ wcn common macro start ------
# CONFIG_WCN_BSP_DRIVER_BUILDIN ?= y
CONFIG_SC23XX = y
CONFIG_WCN_BOOT = y
# add macro for build source files
# ifeq ($(TARGET_BUILD_VARIANT), user)
# MARLIN_CFLAGS += -DCONFIG_WCN_USER
# endif
# MARLIN_CFLAGS += -DCONFIG_WCN_RDCDBG
# MARLIN_CFLAGS += -DCONFIG_WCN_PARSE_DTS
# ------ wcn common macro end ------

# ------ wcn common inclue files start ------
INCLUDE_CFLAGS += -I$(src)/platform/
# INCLUDE_CFLAGS += -I$(src)/platform/rf/
INCLUDE_CFLAGS += -I$(src)/include/
# ------ wcn common inclue files end ------

#
# Final Objects
#
ifeq ($(CONFIG_WCN_BSP_DRIVER_BUILDIN), y)
# Support bsp driver build-in
obj-y := $(KO_MODULE_NAME).o
else
# Support bsp driver build module
obj-m += $(KO_MODULE_NAME).o
endif

$(KO_MODULE_NAME)-$(CONFIG_SC23XX) := wcn_bus.o

ifdef CONFIG_WCN_BOOT
# marlin3 files
$(KO_MODULE_NAME)-y += $(addprefix platform/, fcs_wlan_power_control.o)
$(KO_MODULE_NAME)-$(CONFIG_WCN_BOOT) += $(addprefix platform/, wcn_boot.o \
			wcn_dump.o wcn_op.o)
# wcn buf log loopcheck ...
$(KO_MODULE_NAME)-$(CONFIG_WCN_BOOT) += $(addprefix platform/, bufring.o wcn_txrx.o wcn_misc.o \
			wcn_parn_parser.o wcn_log.o wcn_procfs.o loopcheck.o reset.o \
			reset_test.o rdc_debug.o sysfs.o)
$(KO_MODULE_NAME)-$(CONFIG_WCN_PCIE) +=$(addprefix platform/,pcie_boot.o)
$(KO_MODULE_NAME)-$(CONFIG_WCN_SWD) += $(addprefix platform/,wcn_swd_dap.o)
$(KO_MODULE_NAME)-$(CONFIG_WCN_USB) += $(addprefix platform/,usb_boot.o)
ifeq ($(CONFIG_UMW2653), y)
$(KO_MODULE_NAME)-$(CONFIG_WCN_SDIO) += $(addprefix platform/,mem_pd_mgr_new.o)
else
$(KO_MODULE_NAME)-$(CONFIG_WCN_SDIO) += $(addprefix platform/,mem_pd_mgr.o)
endif
$(KO_MODULE_NAME)-$(CONFIG_WCN_UTILS) += $(addprefix platform/,wcn_debugfs.o)
$(KO_MODULE_NAME)-$(CONFIG_WCN_GNSS) +=$(addprefix platform/gnss/,gnss_common_ctl.o \
			gnss_pmnotify_ctl.o gnss_dbg.o gnss_dump.o)
endif  # CONFIG_WCN_BOOT

ifdef CONFIG_WCN_SDIO
$(KO_MODULE_NAME)-$(CONFIG_WCN_SDIO) += $(addprefix sdio/,sdiohal_main.o sdiohal_common.o \
			sdiohal_tx.o sdiohal_rx.o sdiohal_if.o)

$(KO_MODULE_NAME)-$(CONFIG_DEBUG_FS) += $(addprefix sdio/,sdiohal_ctl.o)
endif  # CONFIG_WCN_SDIO

ifdef CONFIG_WCN_PCIE
$(KO_MODULE_NAME)-$(CONFIG_WCN_PCIE) += $(addprefix pcie/,pcie.o edma_engine.o pcie_dbg.o \
			ioctl.o mchn.o edma_test.o pcie_attach_bus.o pcie_pm.o)
endif  # CONFIG_WCN_PCIE

ifdef CONFIG_WCN_SLP
$(KO_MODULE_NAME)-$(CONFIG_WCN_SLP) += $(addprefix sleep/,sdio_int.o slp_mgr.o slp_sdio.o \
			slp_test.o)
endif  # CONFIG_WCN_SLP

ifdef CONFIG_WCN_USB
$(KO_MODULE_NAME)-$(CONFIG_WCN_USB) += $(addprefix usb/,wcn_usb.o wcn_usb_rx_tx.o \
			wcn_usb_store.o wcn_usb_io.o wcn_usb_test.o wcn_usb_download.o)
endif  # CONFIG_WCN_USB

ccflags-y += $(MARLIN_CFLAGS) \
			$(INCLUDE_CFLAGS)

$(info "ccflags-y: $(ccflags-y)")

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers\
						$(DRIVERS_DIR)/soc/x2600_510/msc/Module.symvers

EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -I$(DRIVERS_DIR) -I$(DRIVERS_DIR)/include/
EXTRA_CFLAGS += -I$(SOC_DIR)/include/
EXTRA_CFLAGS += -include $(DRIVERS_DIR)/config.h

all: driver

driver:
	@echo build driver
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	$(CROSS_COMPILE)strip -g $(KO_MODULE_NAME).ko

install:
	@echo install targets
	mkdir -p $(ANDROID_PRODUCT_OUT)/system/lib/modules
	cp -fv $(PWD)/$(KO_MODULE_NAME).ko $(ANDROID_PRODUCT_OUT)/system/lib/modules/$(KO_MODULE_NAME).ko
	make clean

clean:
	cd platform ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd sdio ; rm -fr */*.mod.c */*.mod */*.o */.*.cmd */*.ko
	cd sdio ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd sleep ; rm -fr */*.mod.c */*.mod */*.o */.*.cmd */*.ko
	cd sleep ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	rm -fr Module.symvers ; rm -fr Module.markers ; rm -fr modules.order
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -fr .tmp_versions
