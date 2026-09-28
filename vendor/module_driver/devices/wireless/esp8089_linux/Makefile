

include $(DRIVERS_DIR)/tools/common_module.mk

ARCH ?= mips
CROSS_COMPILE ?= mips-linux-gnu-

ccflags-y += -DHAS_FW
ccflags-y += -Wno-implicit-fallthrough

PDIR := $(shell pwd)

MODULE_NAME = esp8089

ESP8089_DIR += -I$(PDIR)/src
module_objects += src/esp_wlan_power_control.o
module_objects += src/esp_debug.o
module_objects += src/sdio_sif_esp.o
module_objects += src/esp_io.o
module_objects += src/esp_file.o
module_objects += src/esp_main.o
module_objects += src/esp_sip.o
module_objects += src/esp_ext.o
module_objects += src/esp_ctrl.o
module_objects += src/esp_mac80211.o
module_objects += src/esp_debug.o
module_objects += src/esp_utils.o

EXTRA_CFLAGS += -I$(src)/include -I$(src)/
KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers $(SOC_DIR)/msc/Module.symvers
EXTRA_CFLAGS += -I$(DRIVERS_DIR) -I$(DRIVERS_DIR)/include/
EXTRA_CFLAGS += -I$(SOC_DIR)/include/
EXTRA_CFLAGS += -include $(DRIVERS_DIR)/config.h

esp8089-objs := $(module_objects)
obj-m += $(MODULE_NAME).o