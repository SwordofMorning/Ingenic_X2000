WLAN_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

include $(DRIVERS_DIR)/tools/common_module.mk
include $(WLAN_DIR)/env_config.mk

MAIN_PATH :=
OAL_PATH := oal/
OAM_PATH := oam/
INGENIC_PATH := ingenic/
WAL_PATH := wal/
HCC_PATH := hcc/
SEC_PATH := libsec/
SDIO_PATH := sdio/

WLAN_CFLAGS += -std=gnu99 -Wno-declaration-after-statement -fsigned-char -freg-struct-return -Wtrampolines -Wfloat-equal -fvisibility=hidden

ifeq ($(CFG_3518EV300), y)
WLAN_CFLAGS += -Wdate-time
endif

WLAN_CFLAGS += -I$(DRIVERS_DIR) -I$(DRIVERS_DIR)/include/
WLAN_CFLAGS += -I$(SOC_DIR)/include/
WLAN_CFLAGS += -include $(DRIVERS_DIR)/config.h

WLAN_CFLAGS += -I$(WLAN_DIR)
WLAN_CFLAGS += -I$(WLAN_DIR)hcc
WLAN_CFLAGS += -I$(WLAN_DIR)ipcm
WLAN_CFLAGS += -I$(WLAN_DIR)include
WLAN_CFLAGS += -I$(WLAN_DIR)libsec
WLAN_CFLAGS += -I$(WLAN_DIR)sdio
WLAN_CFLAGS += -I$(WLAN_DIR)oal
WLAN_CFLAGS += -I$(WLAN_DIR)oam
WLAN_CFLAGS += -I$(WLAN_DIR)uart
WLAN_CFLAGS += -I$(WLAN_DIR)ingenic

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
						$(DRIVERS_DIR)/drivers/rmem_manager/Module.symvers \
						$(SOC_DIR)/msc/Module.symvers

main-objs += main.o
main-objs := $(addprefix $(MAIN_PATH),$(main-objs))

oal-objs += oal_util.o oal_netbuf.o
oal-objs := $(addprefix $(OAL_PATH),$(oal-objs))

oam-objs  += oam_main.o oam_log.o
oam-objs := $(addprefix $(OAM_PATH),$(oam-objs))

ingenic-objs  += hi_power.o
ingenic-objs := $(addprefix $(INGENIC_PATH),$(ingenic-objs))

wal-objs += wal_net.o wal_netlink.o
wal-objs := $(addprefix $(WAL_PATH),$(wal-objs))

hcc-objs := hcc_host.o hcc_task.o hcc_adapt.o
hcc-objs := $(addprefix $(HCC_PATH),$(hcc-objs))

sdio-objs := sdio_host.o
sdio-objs := $(addprefix $(SDIO_PATH),$(sdio-objs))

sec-objs += memcpy_s.o memmove_s.o memset_s.o strcat_s.o strcpy_s.o strncat_s.o strncpy_s.o strtok_s.o \
            snprintf_s.o vsnprintf_s.o secureprintoutput_a.o
sec-objs := $(addprefix $(SEC_PATH),$(sec-objs))

WLAN_OBJS += $(main-objs) $(oal-objs) $(ingenic-objs) $(oam-objs) $(wal-objs) $(hcc-objs) $(sec-objs) $(sdio-objs)

EXTRA_CFLAGS += $(WLAN_CFLAGS)
PWD := $(shell pwd)
USER_MODULE_NAME := hichannel
MODULE_NAME := $(USER_MODULE_NAME)
$(MODULE_NAME)-y += $(WLAN_OBJS)
obj-m = $(MODULE_NAME).o
