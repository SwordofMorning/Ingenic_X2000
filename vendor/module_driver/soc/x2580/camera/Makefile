include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)/camera/isp/tiziano-core/include -mmxu3
ccflags-y += -I$(SOC_DIR)/camera/hal
ccflags-y += -I$(SOC_DIR)/camera/

ccflags-y += -Wno-implicit-fallthrough

MODULE_NAME := soc_camera

module-y += camera.o

module-y += isp/tiziano-core/x2580-tiziano-core.a

module-y += isp/mscaler.o
module-y += isp/isp_tuning.o
module-y += isp/vic_channel_tiziano.o
module-y += isp/isp_sensor.o
module-y += isp/x2580_isp_common.o
module-y += isp/isp.o

module-y += hal/camera_gpio.o
module-y += hal/dsys.o
module-y += hal/csi.o
module-y += hal/vic.o

module-y += vic/vic_channel_mem.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
						$(DRIVERS_DIR)/drivers/rmem_manager/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)