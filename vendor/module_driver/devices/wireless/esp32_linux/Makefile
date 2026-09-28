WLAN_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(DRIVERS_DIR)/tools/common_module.mk
CONFIG_SUPPORT_ESP_SERIAL = y
CONFIG_ENABLE_MONITOR_PROCESS = n

# Whether to enter download mode
CONFIG_ENTER_DOWNLOAD = n

# Architecture
ARCH ?= mips
# Toolchain Path
CROSS_COMPILE ?= mips-linux-gnu-

PWD := $(shell pwd)

#Default interface is sdio
MODULE_NAME = esp32_sdio

#Targets passed overrrides default value
ifeq ($(target), sdio)
	MODULE_NAME = esp32_sdio
endif

ifeq ($(target), spi)
	MODULE_NAME = esp32_spi
endif

ifeq ($(CONFIG_ENTER_DOWNLOAD), y)
	EXTRA_CFLAGS += -DCONFIG_ENTER_DOWNLOAD
endif

ifeq ($(CONFIG_SUPPORT_ESP_SERIAL), y)
	EXTRA_CFLAGS += -DCONFIG_SUPPORT_ESP_SERIAL
	module_objects += esp_serial.o esp_rb.o
endif

ifeq ($(CONFIG_ENABLE_MONITOR_PROCESS), y)
	EXTRA_CFLAGS += -DCONFIG_ENABLE_MONITOR_PROCESS
endif

ifeq ($(MODULE_NAME), esp32_sdio)
	EXTRA_CFLAGS += -I$(PWD)/sdio
	module_objects += sdio/esp_sdio.o sdio/esp_sdio_api.o
endif

ifeq ($(MODULE_NAME), esp32_spi)
	EXTRA_CFLAGS += -I$(PWD)/spi
	module_objects += spi/esp_spi.o
endif

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers $(SOC_DIR)/utils/Module.symvers
EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -I$(DRIVERS_DIR) -I$(DRIVERS_DIR)/include/
EXTRA_CFLAGS += -I$(SOC_DIR)/include/
EXTRA_CFLAGS += -include $(DRIVERS_DIR)/config.h

obj-m := $(MODULE_NAME).o
$(MODULE_NAME)-y := esp_bt.o main.o $(module_objects)