include $(DRIVERS_DIR)/tools/common_module.mk

EXTRA_CFLAGS +=-Wno-error=date-time
MODULE_NAME:= cst8xx_touch

module-y += ts_cst8xx_data.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)