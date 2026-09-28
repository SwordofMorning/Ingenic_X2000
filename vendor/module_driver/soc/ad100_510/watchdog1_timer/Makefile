include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_watchdog1_timer

module-y += watchdog1_timer.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)