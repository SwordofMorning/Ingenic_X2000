PWD := $(shell pwd)

CONFIG_AIC8800_BSP_SUPPORT ?= m
CONFIG_SDIO_SUPPORT := y
CONFIG_SDIO_PWRCTRL := y
CONFIG_AIC_FW_PATH = "/lib/firmware/aic_fw"
# CONFIG_AIC_FW_PATH = "/data/aic_firmware"
export CONFIG_AIC_FW_PATH
ccflags-y += -DCONFIG_AIC_FW_PATH=\"$(CONFIG_AIC_FW_PATH)\"

MODULE_NAME := aic8800_bsp
ifeq ($(CONFIG_SDIO_SUPPORT), y)
ccflags-y += -DAICWF_SDIO_SUPPORT
ccflags-$(CONFIG_SDIO_PWRCTRL) += -DCONFIG_SDIO_PWRCTRL
endif

CONFIG_GPIO_WAKEUP = n
CONFIG_LINK_DET_5G = y
CONFIG_MCU_MESSAGE = n
CONFIG_FIRMWARE_ARRAY = n
# Need to set fw path in BOARD_KERNEL_CMDLINE
CONFIG_USE_FW_REQUEST = n
CONFIG_FDRV_NO_REG_SDIO = n
CONFIG_VRF_DCDC_MODE = y
CONFIG_OOB = n
CONFIG_PREALLOC_TXQ = y
CONFIG_ONE_TXQ = n

ccflags-$(CONFIG_GPIO_WAKEUP) += -DCONFIG_GPIO_WAKEUP
ccflags-$(CONFIG_LINK_DET_5G) += -DCONFIG_LINK_DET_5G
ccflags-$(CONFIG_MCU_MESSAGE) += -DCONFIG_MCU_MESSAGE
ccflags-$(CONFIG_FIRMWARE_ARRAY) += -DCONFIG_FIRMWARE_ARRAY
ccflags-$(CONFIG_USE_FW_REQUEST) += -DCONFIG_USE_FW_REQUEST
ccflags-$(CONFIG_FDRV_NO_REG_SDIO) += -DCONFIG_FDRV_NO_REG_SDIO
ccflags-$(CONFIG_VRF_DCDC_MODE) += -DCONFIG_VRF_DCDC_MODE
ccflags-$(CONFIG_OOB) += -DCONFIG_OOB
ccflags-$(CONFIG_PREALLOC_TXQ) += -DCONFIG_PREALLOC_TXQ
ccflags-$(CONFIG_ONE_TXQ)  += -DCONFIG_ONE_TXQ

# obj-m := $(MODULE_NAME).o
ifeq ($(ANKER_INDOOR_CAM_ENV_PRODUCT_NAME),T8425)
    ccflags-y += -Dt8425
endif

obj-$(CONFIG_AIC8800_BSP_SUPPORT) := $(MODULE_NAME).o

$(MODULE_NAME)-y := \
	aic_wlan_power_control.o\
	aic8800dc_compat.o \
	aic8800d80_compat.o \
	aic_bsp_main.o \
	aic_bsp_driver.o \
	aicsdio.o \
	aicsdio_txrxif.o \
	md5.o

$(MODULE_NAME)-$(CONFIG_PREALLOC_TXQ)     += aicwf_txq_prealloc.o

ifeq ($(CONFIG_FIRMWARE_ARRAY),y)
$(MODULE_NAME)-y +=	aicwf_firmware_array.o
endif

# Platform support list
CONFIG_PLATFORM_ALLWINNER ?= n
CONFIG_PLATFORM_INGENIC_TXX ?= y
CONFIG_PLATFORM_AMLOGIC ?= n
CONFIG_PLATFORM_UBUNTU ?= n

ifeq ($(CONFIG_PLATFORM_ALLWINNER), y)
ccflags-$(CONFIG_PLATFORM_ALLWINNER) += -DCONFIG_PLATFORM_ALLWINNER
#KDIR  ?= /home/yaya/E/Allwinner/A133/a133-sdk/android/longan/out/kernel/build/
#ARCH ?= arm64
#CROSS_COMPILE ?= /home/yaya/E/Allwinner/A133/a133-sdk/android/longan/out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
KDIR  ?= /home/yaya/E/Allwinner/r818/Android10/lichee/kernel/linux-4.9/
ARCH ?= arm64
CROSS_COMPILE ?= /home/yaya/E/Allwinner/r818/Android10/android/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
endif

ifeq ($(CONFIG_PLATFORM_INGENIC_TXX), y)
ccflags-$(CONFIG_PLATFORM_INGENIC_TXX) += -DCONFIG_INGENIC_TXX
# KDIR := $(ANKER_INDOOR_CAM_ENV_KERNEL_DIR)
PWD  ?= $(shell pwd)
ARCH ?= mips
CROSS_COMPILE ?= mips-linux-gnu-
endif

ifeq ($(CONFIG_PLATFORM_AMLOGIC), y)
ccflags-$(CONFIG_PLATFORM_AMLOGIC) += -DCONFIG_PLATFORM_AMLOGIC
KDIR  ?= /home/aiden/D1/SDK/Amlogic/905x3_a9/android9.0/out/target/product/u202/obj/KERNEL_OBJ/
ARCH ?= arm
CROSS_COMPILE ?= /home/aiden/D1/SDK/Amlogic/905x3_a9/android9.0/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-
endif

ifeq ($(CONFIG_PLATFORM_UBUNTU), y)
ccflags-$(CONFIG_PLATFORM_UBUNTU) += -DCONFIG_PLATFORM_UBUNTU
KDIR  ?= /lib/modules/$(shell uname -r)/build
PWD   ?= $(shell pwd)
KVER ?= $(shell uname -r)
MODDESTDIR ?= /lib/modules/$(KVER)/kernel/drivers/net/wireless/
ARCH ?= x86_64
CROSS_COMPILE ?=
endif

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers\
						$(DRIVERS_DIR)/soc/x2600_510/msc/Module.symvers

EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -I$(DRIVERS_DIR) -I$(DRIVERS_DIR)/include/
EXTRA_CFLAGS += -I$(SOC_DIR)/include/
EXTRA_CFLAGS += -include $(DRIVERS_DIR)/config.h



all: modules
modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	# $(CROSS_COMPILE)strip -g $(MODULE_NAME).ko

install:
	mkdir -p $(MODDESTDIR)
	install -p -m 644 $(MODULE_NAME).ko  $(MODDESTDIR)
	/sbin/depmod -a ${KERNEL_DIR}

uninstall:
	rm -rfv $(MODDESTDIR)/$(MODULE_NAME).ko
	/sbin/depmod -a ${KERNEL_DIR}

clean:
	rm -rf *.o *.ko *.o.* *.mod.* modules.* Module.* .a* .o* .*.o.* *.mod .tmp* .cache.mk built-in.a

