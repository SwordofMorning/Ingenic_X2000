include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)/aic

MODULE_NAME := x2600_510_icodec_sound_card

module-y += x2600_510_icodec_snd_card.o

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
