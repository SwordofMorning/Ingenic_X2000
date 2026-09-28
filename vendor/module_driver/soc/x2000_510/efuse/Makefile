include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_efuse

module-$(MD_X2000_510_VERIFY_MODE_HAMMING) += hamming_mode/jz_efuse.o
module-$(MD_X2000_510_VERIFY_MODE_NONE) += raw_mode/jz_efuse.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)