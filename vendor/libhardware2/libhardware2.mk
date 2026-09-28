include .config.in

#
# 定义默认目标
#
all: all_targets

#
# 编译选项
#
CFLAGS = -Os -fPIC -Wall -Werror
CFLAGS += -Iinclude/
CFLAGS += -include config.h
CFLAGS += -Wno-pointer-sign
CFLAGS += -Wno-pointer-to-int-cast

obj_dir = .objs/

check_lib = $(shell tools/check_lib.sh $(CROSS_COMPILE)gcc $1)

ifeq ($(APP_libhardware2_alsa),y)
APP_libhardware2_alsa := $(call check_lib, libasound.so)
endif

# ----------------------
# 编译 libhardware2.so
# ----------------------
LDFLAGS =
LDFLAGS-$(APP_libhardware2_alsa) += -lasound
LDFLAGS-$(APP_libhardware2_wifi) += -lpthread
LDFLAGS-$(APP_libhardware2_avpu) += -L src/lib/avpu -lavpu

module_name = output/libhardware2.so

src-$(APP_libhardware2_fb) += src/lib/fb/fb.c
src-$(APP_libhardware2_fb_layer_mixer) += src/lib/fb_layer_mixer/fb_layer_mixer.c
src-$(APP_libhardware2_v4l2_jpeg_encode) += src/lib/vpu/v4l2_jpeg_encode.c
src-$(APP_libhardware2_v4l2_jpeg_decode) += src/lib/vpu/v4l2_jpeg_decode.c
src-$(APP_libhardware2_v4l2_h264_encode) += src/lib/vpu/v4l2_h264_encode.c
src-$(APP_libhardware2_v4l2_h264_decode) += src/lib/vpu/v4l2_h264_decode.c
src-$(APP_libhardware2_avpu)             += src/lib/avpu/avpu_codec.c
src-$(APP_libhardware2_avpu_jpeg_encode) += src/lib/avpu/video_jpeg_encode.c
src-$(APP_libhardware2_avpu_h264_encode) += src/lib/avpu/video_h264_encode.c
src-$(APP_libhardware2_hw_jpeg_encode) += src/lib/jpeg/jpegenc.c
src-$(APP_libhardware2_hw_jpeg_decode) += src/lib/jpeg/jpegdec.c
src-$(APP_libhardware2_camera) += src/lib/camera/camera.c
src-$(APP_libhardware2_adc) += src/lib/adc/adc.c
src-$(APP_libhardware2_i2c) += src/lib/i2c/i2c.c
src-$(APP_libhardware2_pwm) += src/lib/pwm/pwm.c
src-$(APP_libhardware2_pwm_battery) += src/lib/pwm_battery/pwm_battery.c
src-$(APP_libhardware2_efuse) += src/lib/efuse/efuse.c
src-$(APP_libhardware2_watchdog) += src/lib/watchdog/watchdog.c
src-$(APP_libhardware2_keyboard) += src/lib/keyboard/keyboard.c
src-$(APP_libhardware2_spi) += src/lib/spi/spi.c
src-$(APP_libhardware2_sslv) += src/lib/sslv/sslv.c
src-$(APP_libhardware2_can) += src/lib/can/can.c
src-$(APP_libhardware2_gpio) += src/lib/gpio/gpio.c
src-$(APP_libhardware2_gpio_sys) += src/lib/gpio/gpio_sys.c
src-$(APP_libhardware2_ps2) += src/lib/ps2/ps2.c
src-$(APP_libhardware2_mscaler) += src/lib/mscaler/mscaler.c
src-$(APP_libhardware2_dtrng) += src/lib/dtrng/dtrng.c
src-$(APP_libhardware2_mcu) += src/lib/mcu/mcu.c
src-$(APP_libhardware2_wifi) += src/lib/wifi/wifi.c
src-$(APP_libhardware2_v4l2_camera) += src/lib/v4l2/v4l2_camera.c

src-$(APP_libhardware2_gpio_counter) += src/lib/gpio_counter/gpio_counter.c
src-$(APP_libhardware2_alsa) += src/lib/alsa/alsa.c
src-$(APP_libhardware2_rmem) += src/lib/rmem/rmem.c
src-$(APP_libhardware2_rotator) += src/lib/rotator/rotator.c
src-$(APP_libhardware2_hash) += src/lib/hash/hash.c
src-$(APP_libhardware2_aes) += src/lib/aes/aes.c
src-$(APP_libhardware2_sc) += src/lib/ingenic_sc/ingenic_sc.c
src-$(APP_libhardware2_rsa) += src/lib/rsa/rsa.c
src-$(APP_libhardware2_nemc) += src/lib/nemc/nemc.c
src-$(APP_libhardware2_dbox) += src/lib/dbox/dbox.c
src-$(APP_libhardware2_ipu) += src/lib/ipu/ipu.c
src-$(APP_libhardware2_hw_timer) += src/lib/hw_timer/hw_timer.c
include tools/build_elf.mk

# ---------------------------
# the end
# ---------------------------
all_targets: $(module_targets)
	@echo "  compiled $(all_modules)"

libs = $(filter %.so,$(all_modules))

ifneq ($(libs),)
define install_libs
	$(Q)cp -f $(libs) $(FS_TARGET_DIR)/usr/lib/
	$(Q)cp -f $(libs) $(FS_STAGING_DIR)/usr/lib/
	$(Q)cp -rf include/libhardware2 $(FS_STAGING_DIR)/usr/include/
	$(if $(APP_libhardware2_avpu), $(Q)cp -f src/lib/avpu/libavpu.so $(FS_TARGET_DIR)/usr/lib/)
	$(if $(APP_libhardware2_avpu), $(Q)cp -f src/lib/avpu/libavpu.so $(FS_STAGING_DIR)/usr/lib/)
	@echo "  installed $(libs) include/libhardware2/"
endef
define clean_install_libs
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(libs)))
	$(Q)rm -f $(addprefix $(FS_STAGING_DIR)/usr/lib/, $(notdir $(libs)))
	$(if $(APP_libhardware2_avpu), $(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir libavpu.so)))
	$(if $(APP_libhardware2_avpu), $(Q)rm -f $(addprefix $(FS_STAGING_DIR)/usr/lib/, $(notdir libavpu.so)))
	$(Q)rm -rf $(FS_STAGING_DIR)/usr/include/libhardware2
	@echo "  removed $(libs) include/libhardware2/"
endef
endif

install:
	$(install_libs)

clean_install:
	$(clean_install_libs)

clean:
	$(Q)rm -rf output/
	$(Q)rm -rf $(module_clean_files)

.PHONY: all clean all_targets clean_install install
