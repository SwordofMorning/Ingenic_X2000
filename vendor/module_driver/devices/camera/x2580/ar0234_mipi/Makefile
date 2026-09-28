include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)

MODULE_NAME0 := sensor0_ar0234_mipi
MODULE_NAME1 := sensor1_ar0234_mipi

module0-y += ar0234_mipi.o
module1-y += ar0234_mipi.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
                 $(SOC_DIR)/camera-double/Module.symvers

obj-m  = $(MODULE_NAME0).o
obj-m += $(MODULE_NAME1).o

$(MODULE_NAME0)-y = $(module0-y)
$(MODULE_NAME1)-y = $(module1-y)
