include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)/aic

MODULE_NAME := x2580_icodec_sound_card

module-y += x2580_icodec_snd_card.o

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
