include $(DRIVERS_DIR)/tools/common_module.mk

KMODULE_NAME = ssv6x5x

ifeq ($(SSV_DRV_PATH),)
SSV_DRV_PATH := $(DRIVERS_DIR)/devices/wireless/ssv6x5x_linux_v60.20
endif

KBUILD_TOP := $(SSV_DRV_PATH)
ifeq ($(MAKELEVEL),0)
KBUILD_TOP := .
endif

ifeq ($(INSTALL_PATH),)
INSTALL_PATH := $(SSV_DRV_PATH)
endif

include $(KBUILD_TOP)/config_common.mak
include $(KBUILD_TOP)/config.mak

MOD_DEF_H = include/ssv_mod_conf.h

# Generate version strings
# GEN_VER := $(shell cd $(KBUILD_TOP); ./ver_info.pl include/ssv_version.h)
# Generate include/ssv_conf_parser.h
# GEN_CONF_PARSER := $(shell cd $(KBUILD_TOP); env ccflags="$(ccflags-y)" ./parser-conf.sh include/ssv_conf_parser.h)
# Generate $(KMODULE_NAME)-wifi.cfg
#BKP_CFG := $(shell cp $(KBUILD_TOP)/$(PLATFORMS)-wifi.cfg $(KBUILD_TOP)/image/$(KMODULE_NAME)-wifi.cfg)

EXTRA_CFLAGS := -I$(KBUILD_TOP) -I$(KBUILD_TOP)/include
USER_EXTRA_CFLAGS += -I$(SOC_DIR)/include/
DEF_PARSER_H = $(KBUILD_TOP)/include/ssv_conf_parser.h

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers $(SOC_DIR)/utils/Module.symvers $(SOC_DIR)/msc/Module.symvers
EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -I$(DRIVERS_DIR) -I$(DRIVERS_DIR)/include/
EXTRA_CFLAGS += -I$(SOC_DIR)/include/
EXTRA_CFLAGS += -include $(DRIVERS_DIR)/config.h

$(shell env ccflags="$(ccflags-y)" $(KBUILD_TOP)/parser-conf.sh $(DEF_PARSER_H))

#------------------------------
# ssvdevice/
KERN_SRCS := ssvdevice/ssvdevice.c
KERN_SRCS += ssvdevice/ssv_cmd.c

# hci/
KERN_SRCS += hci/ssv_hci.c

# smac/
KERN_SRCS += smac/init.c
KERN_SRCS += smac/dev.c
KERN_SRCS += smac/bdev.c
KERN_SRCS += smac/regd.c
KERN_SRCS += smac/wow.c
KERN_SRCS += smac/hw_scan.c

KERN_SRCS += smac/lib.c
KERN_SRCS += smac/ap.c

ifeq ($(findstring -DCONFIG_SSV6XXX_DEBUGFS, $(ccflags-y)), -DCONFIG_SSV6XXX_DEBUGFS)
KERN_SRCS += smac/ssv6xxx_debugfs.c
endif
KERN_SRCS += smac/efuse.c
KERN_SRCS += smac/ssv_skb.c
ifeq ($(findstring -DCONFIG_SSV_CTL, $(ccflags-y)), -DCONFIG_SSV_CTL)
KERN_SRCS += smac/ssv_wifi_ctl.c
ifeq ($(findstring -DCONFIG_SSV_SMARTLINK, $(ccflags-y)), -DCONFIG_SSV_SMARTLINK)
KERN_SRCS += smac/kssvsmart.c
endif
endif

KERN_SRCS += smac/hal/hal.c
ifeq ($(findstring -DSSV_SUPPORT_SSV6006, $(ccflags-y)), -DSSV_SUPPORT_SSV6006)
KERN_SRCS += smac/hal/ssv6006c/ssv6006_common.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006C_mac.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006_phy.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006_turismoC.c
endif
ifeq ($(findstring -DSSV_SUPPORT_SSV6020, $(ccflags-y)), -DSSV_SUPPORT_SSV6020)
KERN_SRCS += smac/hal/ssv6020/ssv6020_common.c
KERN_SRCS += smac/hal/ssv6020/ssv6020_mac.c
KERN_SRCS += smac/hal/ssv6020/ssv6020_phy.c
KERN_SRCS += smac/hal/ssv6020/ssv6020_turismoE.c
endif

# hwif/hal/
KERN_SRCS += hwif/hal/hwif_hal.c
ifeq ($(findstring -DSSV_SUPPORT_SSV6006, $(ccflags-y)), -DSSV_SUPPORT_SSV6006)
KERN_SRCS += hwif/hal/ssv6006c/ssv6006C_hwif.c
endif
ifeq ($(findstring -DSSV_SUPPORT_SSV6020, $(ccflags-y)), -DSSV_SUPPORT_SSV6020)
KERN_SRCS += hwif/hal/ssv6020/ssv6020_hwif.c
endif

# hwif/sdio/
KERN_SRCS += hwif/sdio/sdio.c


#------------------------------

KERN_SRCS += $(KMODULE_NAME)-generic-wlan.c

$(KMODULE_NAME)-y += $(KERN_SRCS_S:.S=.o)
$(KMODULE_NAME)-y += $(KERN_SRCS:.c=.o)

obj-$(CONFIG_SSV6X5X) += $(KMODULE_NAME).o

obj-m = $(KMODULE_NAME).o
$(KMODULE_NAME)-y = $(KERN_SRCS:.c=.o)

