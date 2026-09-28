PWD := $(shell pwd)

CONFIG_AIC8800_BTLPM_SUPPORT ?= m
CONFIG_AIC8800_WLAN_SUPPORT ?= m
CONFIG_AIC_WLAN_SUPPORT ?= m

obj-$(CONFIG_AIC8800_BTLPM_SUPPORT) += aic8800_btlpm/
obj-$(CONFIG_AIC8800_WLAN_SUPPORT) += aic8800_fdrv/
obj-$(CONFIG_AIC_WLAN_SUPPORT) += aic8800_bsp/

# Platform support list
CONFIG_PLATFORM_ROCKCHIP ?= n
CONFIG_PLATFORM_ALLWINNER ?= n
CONFIG_PLATFORM_INGENIC_TXX ?= y
CONFIG_PLATFORM_AMLOGIC ?= n
CONFIG_PLATFORM_UBUNTU ?= n



ifeq ($(CONFIG_PLATFORM_INGENIC_TXX), y)
ccflags-$(CONFIG_PLATFORM_INGENIC_TXX) += -DCONFIG_INGENIC_TXX
KDIR := $(ANKER_CAM_KERNEL_DIR)
ARCH ?= mips
CROSS_COMPILE ?= mips-linux-gnu-
endif

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers\
						$(DRIVERS_DIR)/soc/x2600_510/msc/Module.symvers

EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -I$(DRIVERS_DIR) -I$(DRIVERS_DIR)/include/
EXTRA_CFLAGS += -I$(SOC_DIR)/include/
EXTRA_CFLAGS += -include $(DRIVERS_DIR)/config.h


all: modules
modules:
	make -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	# $(CROSS_COMPILE)strip -g aic8800d80.ko

uninstall:
	rm -rfv $(MODDESTDIR)/aic8800_bsp.ko
	rm -rfv $(MODDESTDIR)/aic8800_fdrv.ko
	rm -rfv $(MODDESTDIR)/aic8800_btlpm.ko
	/sbin/depmod -a ${KERNEL_DIR}

clean:
	cd aic8800_bsp/;make clean;cd ..
	cd aic8800_fdrv/;make clean;cd ..
	cd aic8800_btlpm/;make clean;cd ..
	rm -rf modules.order Module.symvers .tmp_versions/

