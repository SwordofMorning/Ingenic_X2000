PWD := $(shell pwd)

COUSTOM_PLATFORM := ingenic

ARCH ?= mips
CROSS_COMPILE ?= mips-linux-gnu-


UNISOC_WCN_CHIP_ID := uwe5622
UNISOC_WCN_HW_TYPE := sdio

UNISOC_BSP_INCLUDE := $(abspath $(lastword $(MAKEFILE_LIST)/../../unisocwcn/include))
$(info UNISOC_BSP_INCLUDE = $(UNISOC_BSP_INCLUDE))
src := $(abspath $(lastword $(MAKEFILE_LIST)/../))
$(info src = $(src))

ifneq ($(UNISOC_BSP_INCLUDE),)
ccflags-y += -I$(UNISOC_BSP_INCLUDE)
endif

ifeq ($(filter $(strip $(UNISOC_WCN_HW_TYPE)), sdio usb pcie),)
$(error no UNISOC_WCN_HW_TYPE, pls fix it !!!)
endif

ifeq ($(UNISOC_WCN_HW_TYPE), sdio)
WLAN_DEVICE := sc2355
endif

ifeq ($(UNISOC_WCN_HW_TYPE), usb)
WLAN_DEVICE := sc2355
endif

ifeq ($(UNISOC_WCN_HW_TYPE), pcie)
# TODO
WLAN_DEVICE := marlin3_pcie
endif

#
# Module name
#
KO_MODULE_NAME := fcs950uab_wifi
src := $(abspath $(lastword $(MAKEFILE_LIST)/../))
$(info src = $(src))
KO_MODULE_PATH := $(src)/$(WLAN_DEVICE)
$(info "KO_MODULE_PATH: $(KO_MODULE_PATH)")
KO_MODULE_SRC += $(shell find $(KO_MODULE_PATH) -name "*.c")

#
# Build Options
#
# ------ coustom platform config start ------
ifeq ($(COUSTOM_PLATFORM), unisoc)
ccflags-y += -DCONFIG_UNISOC_BOARD
endif

ifeq ($(COUSTOM_PLATFORM), ingenic)
ccflags-y += -DCONFIG_THIRD_PARTY_BOARD
endif

ifeq ($(COUSTOM_PLATFORM), qcom)
ccflags-y += -DCONFIG_THIRD_PARTY_BOARD
ccflags-y += -DCONFIG_QCOM_BOARD
ccflags-y += -DCONFIG_QUE_EC300F
endif

ifeq ($(COUSTOM_PLATFORM), rk)
ccflags-y += -DCONFIG_THIRD_PARTY_BOARD
ccflags-y += -DCONFIG_RK_BOARD
endif

ifeq ($(COUSTOM_PLATFORM), mtk)
ccflags-y += -DCONFIG_THIRD_PARTY_BOARD
ccflags-y += -DCONFIG_MTK_BOARD
endif
# ------ coustom platform config end ------

# ------ wcn chip id config start ------
ifeq ($(UNISOC_WCN_CHIP_ID), umw2652)
ccflags-y += -DCONFIG_UMW2652
endif

ifeq ($(UNISOC_WCN_CHIP_ID), umw2653)
ccflags-y += -DCONFIG_UMW2653
endif

ifeq ($(UNISOC_WCN_CHIP_ID), uwe5621)
ccflags-y += -DCONFIG_UMW2653
endif

ifeq ($(UNISOC_WCN_CHIP_ID), uwe5622)
ccflags-y += -DCONFIG_UMW2652
endif

ifeq ($(UNISOC_WCN_CHIP_ID), uwe5623)
ccflags-y += -DCONFIG_UMW2653
endif
# ------ wcn chip id config end ------

ifeq ($(strip $(WLAN_DEVICE)), sc2355)
# support thirdpary OTT
ccflags-y += -DOTT_UWE
ccflags-y += -DSC2355_FTR -DSC2355_TX_FILTER -DSC2355_QOS_MAP
ccflags-y += -DIBSS_SUPPORT -DIBSS_RSN_SUPPORT
ccflags-y += -DNAN_SUPPORT
ccflags-y += -DRTT_SUPPORT
ccflags-y += -DSC2355_RX_NO_LOOP
ccflags-y += -DACS_SUPPORT -DSC2355_HW_CSUM
ccflags-y += -DWMMAC_WFA_CERTIFICATION
ccflags-y += -DCOMPAT_SAMPILE_CODE
ccflags-y += -DRND_MAC_SUPPORT
ccflags-y += -DSOFTAP_HOOK
ccflags-y += -DATCMD_ASSERT
ccflags-y += -DTCPACK_DELAY_SUPPORT
#  ifeq ($(strip $(TARGET_BUILD_VARIANT)),userdebug)
#   ccflags-y += -DCONFIG_SPRD_WCN_DEBUG
#  endif
ccflags-y += -DSPLIT_STACK
#ccflags-y += -DDRV_RESET_SELF
endif

ifeq ($(strip $(WLAN_DEVICE)), marlin3_pcie)
ccflags-y += -DSC2355_FTR -DSC2355_TX_FILTER -DSC2355_QOS_MAP
ccflags-y += -DIBSS_SUPPORT -DIBSS_RSN_SUPPORT
ccflags-y += -DNAN_SUPPORT
ccflags-y += -DRTT_SUPPORT
#ccflags-y += -DSC2355_RX_NO_LOOP
ccflags-y += -DACS_SUPPORT -DSC2355_HW_CSUM
ccflags-y += -DWMMAC_WFA_CERTIFICATION
ccflags-y += -DCOMPAT_SAMPILE_CODE
ccflags-y += -DRND_MAC_SUPPORT
ccflags-y += -DSOFTAP_HOOK
ccflags-y += -DATCMD_ASSERT
ccflags-y += -DTCPACK_DELAY_SUPPORT
#ccflags-y += -DENABLE_PAM_WIFI
	ifeq ($(strip $(TARGET_BUILD_VARIANT)), userdebug)
	ccflags-y += -DCONFIG_SPRD_WCN_DEBUG
	endif
ccflags-y += -DSPLIT_STACK
endif

#ccflags-y += -Wno-packed-bitfield-compat

#
# Final Objects
#
obj-m := $(KO_MODULE_NAME).o

# Comment it if the only object file has the same name with module
ifeq ($(strip $(WLAN_DEVICE)), marlin3_pcie)
KO_SRC_PATH := marlin3_pcie
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,main.o cfg80211.o txrx.o \
						cmdevt.o npi.o msg.o work.o vendor.o tcp_ack.o mm.o \
						reorder.o core_sc2355.o tx_msg_sc2355.o rx_msg_sc2355.o \
						if_sc2355.o qos.o edma_test.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,defrag.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,ibss.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,nan.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,tracer.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,rf_marlin3.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,rtt.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,api_version.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,rnd_mac_addr.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,softap_hook.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,debug.o)
$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,txrx_buf_mm.o)
# support for pam_wifi
#$(KO_MODULE_NAME)-y += $(addprefix $(KO_SRC_PATH)/,pam_wifi.o)
else
$(KO_MODULE_NAME)-y := $(patsubst $(src)/%.c, %.o, $(KO_MODULE_SRC))
endif

KBUILD_EXTRA_SYMBOLS := $(abspath $(lastword $(MAKEFILE_LIST)/../../unisocwcn/Module.symvers))
$(info "KBUILD_EXTRA_SYMBOLS: $(KBUILD_EXTRA_SYMBOLS)")

all: driver

driver:
	@echo build driver
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	cd sc2355 ; rm -fr */*.mod.c */*.mod */*.o */.*.cmd */*.ko
	cd sc2355 ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	rm -fr Module.symvers ; rm -fr Module.markers ; rm -fr modules.order
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
