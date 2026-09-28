include .config.in

#
# 定义默认目标
#
all: all_targets

#
# 编译选项
#
CFLAGS = -Os
CFLAGS += -Iinclude/
CFLAGS += -include config.h -Wall -Werror
CFLAGS += -Wno-pointer-sign
CFLAGS += -Wno-pointer-to-int-cast

LDFLAGS = -Loutput/ -lhardware2 -lpthread

ifeq ($(APP_libhardware2_avpu),y)
LDFLAGS += -L src/lib/avpu -lavpu
endif

obj_dir = .objs/

check_lib = $(shell tools/check_lib.sh $(CROSS_COMPILE)gcc $1)

ifeq ($(APP_libhardware2_alsa_cmd),y)
APP_libhardware2_alsa_cmd := $(call check_lib, libasound.so)
endif

# ----------------------
# 编译 cmds
# ----------------------

module_name = output/cmd_fb
src-$(APP_libhardware2_fb_cmd) += src/cmds/fb_main.c
include tools/build_elf.mk

module_name = output/cmd_fb_scale
src-$(APP_libhardware2_fb_scale_cmd) += src/cmds/fb_scale_main.c
include tools/build_elf.mk

module_name = output/cmd_camera
src-$(APP_libhardware2_camera) += src/cmds/camera_main.c
include tools/build_elf.mk

module_name = output/cmd_camera_jpeg_encode
LDFLAGS-$(APP_libhardware2_camera_jpeg_encode) = -lutils2
src-$(APP_libhardware2_camera_jpeg_encode) += src/cmds/camera_jpeg_encode.c
include tools/build_elf.mk

module_name = output/cmd_camera_h264_encode
LDFLAGS-$(APP_libhardware2_camera_h264_encode) = -lutils2
src-$(APP_libhardware2_camera_h264_encode) += src/cmds/camera_h264_encode.c
include tools/build_elf.mk

module_name = output/cmd_camera_avpu_jpeg_encode
LDFLAGS-$(APP_libhardware2_camera_avpu_jpeg_encode) = -lutils2
src-$(APP_libhardware2_camera_avpu_jpeg_encode) += src/cmds/camera_jpeg_avpu_encode.c
include tools/build_elf.mk

module_name = output/cmd_camera_avpu_h264_encode
LDFLAGS-$(APP_libhardware2_camera_avpu_h264_encode) = -lutils2
src-$(APP_libhardware2_camera_avpu_h264_encode) += src/cmds/camera_h264_avpu_encode.c
include tools/build_elf.mk

module_name = output/cmd_camera_nv12_preview
src-$(APP_libhardware2_camera_nv12_preview) += src/cmds/camera_nv12_preview.c
include tools/build_elf.mk

module_name = output/cmd_camera_software_preview
LDFLAGS-$(APP_libhardware2_camera_software_preview) = -lutils2
src-$(APP_libhardware2_camera_software_preview) += src/cmds/camera_software_preview.c
include tools/build_elf.mk

module_name = output/cmd_rector_play
src-$(APP_libhardware2_fb_rector_play) += src/cmds/rector_play.c
include tools/build_elf.mk

module_name = output/cmd_adc
src-$(APP_libhardware2_adc_cmd) += src/cmds/adc_main.c
include tools/build_elf.mk

module_name = output/cmd_i2c
src-$(APP_libhardware2_i2c_cmd) += src/cmds/i2c_main.c
include tools/build_elf.mk

module_name = output/cmd_rtc
src-$(APP_libhardware2_rtc_cmd) += src/cmds/rtc_main.c
include tools/build_elf.mk

module_name = output/cmd_pwm
src-$(APP_libhardware2_pwm_cmd) += src/cmds/pwm_main.c
include tools/build_elf.mk

module_name = output/cmd_pwm_audio
src-$(APP_libhardware2_pwm_audio_cmd) += src/cmds/pwm_audio_main.c
include tools/build_elf.mk

module_name = output/cmd_pwm_battery
src-$(APP_libhardware2_pwm_battery_cmd) += src/cmds/pwm_battery_main.c
include tools/build_elf.mk

module_name = output/cmd_efuse
src-$(APP_libhardware2_efuse_cmd) += src/cmds/efuse_main.c
include tools/build_elf.mk

module_name = output/cmd_watchdog
src-$(APP_libhardware2_watchdog_cmd) += src/cmds/watchdog_main.c
include tools/build_elf.mk

module_name = output/cmd_keyboard
src-$(APP_libhardware2_keyboard_cmd) += src/cmds/keyboard_main.c
include tools/build_elf.mk

module_name = output/cmd_keyboard_test
src-$(APP_libhardware2_keyboard_cmd_test) += src/cmds/keyboard_test_main.c
include tools/build_elf.mk

module_name = output/cmd_spi
src-$(APP_libhardware2_spi_cmd) += src/cmds/spi_main.c
include tools/build_elf.mk

module_name = output/cmd_sslv
src-$(APP_libhardware2_sslv_cmd) += src/cmds/sslv_main.c
include tools/build_elf.mk

module_name = output/cmd_can
src-$(APP_libhardware2_can_cmd) += src/cmds/can_main.c
include tools/build_elf.mk

module_name = output/cmd_gpio
src-$(APP_libhardware2_gpio_cmd) += src/cmds/gpio_main.c
include tools/build_elf.mk

module_name = output/cmd_mscaler
src-$(APP_libhardware2_mscaler_cmd) += src/cmds/mscaler_main.c
include tools/build_elf.mk

module_name = output/cmd_dtrng
src-$(APP_libhardware2_dtrng_cmd) += src/cmds/dtrng_main.c
include tools/build_elf.mk

module_name = output/cmd_mcu
src-$(APP_libhardware2_mcu_cmd) += src/cmds/mcu_main.c
include tools/build_elf.mk

module_name = output/cmd_wifi
src-$(APP_libhardware2_wifi_cmd) += src/cmds/wifi_main.c
include tools/build_elf.mk

module_name = output/cmd_gpio_counter
src-$(APP_libhardware2_gpio_counter_cmd) += src/cmds/gpio_counter_main.c
include tools/build_elf.mk

module_name = output/cmd_jpeg_display
src-$(APP_libhardware2_jpeg_display) += src/cmds/jpeg_display_main.c
LDFLAGS-$(APP_libhardware2_jpeg_display) += -ljpeg -lutils2
include tools/build_elf.mk

module_name = output/cmd_inputdev_listen
src-$(APP_libhardware2_input_dev_listen) += src/cmds/input_dev_listen.c
include tools/build_elf.mk

module_name = output/cmd_alsa
src-$(APP_libhardware2_alsa_cmd) += src/cmds/alsa_main.c
include tools/build_elf.mk

module_name = output/cmd_ps2
src-$(APP_libhardware2_ps2_custom_keyboard) += src/cmds/ps2_main.c
include tools/build_elf.mk

module_name = output/cmd_rotator
src-$(APP_libhardware2_rotator_cmd) += src/cmds/rotator_main.c
include tools/build_elf.mk

module_name = output/cmd_hash
src-$(APP_libhardware2_hash) += src/cmds/hash_main.c
include tools/build_elf.mk

module_name = output/cmd_aes
src-$(APP_libhardware2_aes) += src/cmds/aes_main.c
include tools/build_elf.mk

module_name = output/cmd_sc
src-$(APP_libhardware2_sc_cmd) += src/cmds/ingenic_sc_main.c
include tools/build_elf.mk

module_name = output/cmd_rsa
src-$(APP_libhardware2_rsa) += src/cmds/rsa_main.c
include tools/build_elf.mk

module_name = output/cmd_v4l2_camera
src-$(APP_libhardware2_v4l2_camera_cmd) += src/cmds/v4l2_camera_main.c
include tools/build_elf.mk

module_name = output/cmd_mdio
src-$(APP_libhardware2_mdio_cmd) += src/cmds/mdio_main.c
include tools/build_elf.mk

module_name = output/cmd_jpeg_decode
src-$(APP_libhardware2_jpeg_decode_cmd) += src/cmds/jpeg_decode_main.c
include tools/build_elf.mk

module_name = output/cmd_jpegd_decode
src-$(APP_libhardware2_jpegd_decode_cmd) += src/cmds/jpegd_decode_main.c
include tools/build_elf.mk

module_name = output/cmd_jpegd_decode_preview
src-$(APP_libhardware2_jpegd_decode_preview_cmd) += src/cmds/jpegd_decode_preview_main.c
include tools/build_elf.mk

module_name = output/cmd_jpege_encode
src-$(APP_libhardware2_jpege_encode_cmd) += src/cmds/jpege_encode_main.c
include tools/build_elf.mk

module_name = output/cmd_h264_decode
LDFLAGS-$(APP_libhardware2_h264_decode_cmd) = -lutils2
src-$(APP_libhardware2_h264_decode_cmd) += src/cmds/h264_decode_main.c
include tools/build_elf.mk

module_name = output/cmd_h264_direct_decode
LDFLAGS-$(APP_libhardware2_h264_direct_decode_cmd) = -lutils2
src-$(APP_libhardware2_h264_direct_decode_cmd) += src/cmds/h264_direct_decode_main.c
include tools/build_elf.mk

module_name = output/cmd_nemc
src-$(APP_libhardware2_nemc_cmd) += src/cmds/nemc_main.c
include tools/build_elf.mk

module_name = output/cmd_dbox
src-$(APP_libhardware2_dbox_cmd) += src/cmds/dbox_main.c
include tools/build_elf.mk

module_name = output/cmd_ipu_osd
src-$(APP_libhardware2_ipu_osd_cmd) += src/cmds/ipu_osd_main.c
include tools/build_elf.mk

module_name = output/cmd_ipu_csc
src-$(APP_libhardware2_ipu_csc_cmd) += src/cmds/ipu_csc_main.c
include tools/build_elf.mk

module_name = output/cmd_adc_set_voltage
src-$(APP_libhardware2_adc_set_voltage_cmd) += src/cmds/adc_set_voltage.c
include tools/build_elf.mk

module_name = output/cmd_hw_timer
src-$(APP_libhardware2_hw_timer_cmd) += src/cmds/hw_timer_main.c
include tools/build_elf.mk

module_name = output/demo_tpc_tx_shift_data
src-$(APP_libhardware2_tpc_tx_shift_data) += src/cmds/demo_tpc/demo_tpc_tx_shift_data.c
include tools/build_elf.mk

module_name = output/cmd_nemc_mcu_test
src-$(APP_libhardware2_nemc_mcu_test_cmd) += src/cmds/nemc_mcu_test_main.c
include tools/build_elf.mk

module_name = output/cmd_rmem_extra
src-$(APP_libhardware2_rmem_extra_cmd) += src/cmds/rmem/rmem_extra_main.c
include tools/build_elf.mk

module_name = output/demo_get_mcu_spi_data
src-$(APP_libhardware2_demo_get_mcu_spi_data) += src/cmds/demo_get_mcu_spi_data.c
include tools/build_elf.mk

# ---------------------------
# the end
# ---------------------------
all_targets: $(module_targets)
	@echo "  compiled $(all_modules)"

cmds = $(filter-out %.so,$(all_modules))

ifneq ($(cmds),)
define install_cmds
	$(Q)cp -f $(cmds) $(FS_TARGET_DIR)/usr/bin/
	@echo "  installed $(cmds)"
endef
define clean_install_cmds
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/bin/, $(notdir $(cmds)))
	@echo "  removed $(cmds)"
endef
endif

install:
	$(install_cmds)
ifeq ($(APP_libhardware2_rmem_start_extra_rtos_mem),y)
	$(Q)cp -f src/cmds/rmem/rmem_extra_from_rtos.sh $(FS_TARGET_DIR)/usr/bin/
	$(Q)cp -f src/cmds/rmem/S99rmem_extra_from_rtos $(FS_TARGET_DIR)/etc/init.d/
endif

clean_install:
	$(clean_install_cmds)
ifeq ($(APP_libhardware2_rmem_start_extra_rtos_mem),y)
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/rmem_extra_from_rtos.sh
	$(Q)rm -f $(FS_TARGET_DIR)/etc/init.d/S99rmem_extra_from_rtos
endif

clean:
	$(Q)rm -rf output/
	$(Q)rm -rf $(module_clean_files)

.PHONY: all clean all_targets clean_install install
