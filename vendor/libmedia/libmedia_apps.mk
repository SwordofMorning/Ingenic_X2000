include .config.in

# 此文件中处理需要依赖的库,添加到 LDFLAGS
sinclude libmedia_LDFLAGS.mk

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
CFLAGS += -include config.h

ifdef APP_libmedia_static
LDFLAGS += -Wl,--gc-sections
module_depends = output/libmedia.a

ifdef APP_libmedia_ffmpeg
module_depends += output/libmedia_ffmpeg.a
endif

endif

LDFLAGS += -Wl,-rpath=./ -Loutput/ -lmedia -ldl

obj_dir = .objs/


# ---------------------------
# test app sources
# ---------------------------

module_name = output/audio_test
LDFLAGS-$(APP_libmedia_test_audio) = -lmedia_ffmpeg
src-$(APP_libmedia_test_audio) += test/main_audio_test.c
include tools/build_elf.mk

module_name = output/recorder_test
LDFLAGS-$(APP_libmedia_test_recorder) = -lmedia_ffmpeg
src-$(APP_libmedia_test_recorder) += test/main_recorder_test.c
include tools/build_elf.mk

module_name = output/previewer_test
LDFLAGS-$(APP_libmedia_ffmpeg) = -lmedia_ffmpeg
src-$(APP_libmedia_test_previewer) += test/main_previewer_test.c
include tools/build_elf.mk

module_name = output/secs_previewer_test
src-$(APP_libmedia_test_secs_previewer) += test/main_secs_previewer_test.c
include tools/build_elf.mk

module_name = output/ingenic_camera_app
LDFLAGS-$(APP_libmedia_test_camera_app) = -lswscale -lturbojpeg -lmedia_ffmpeg
src-$(APP_libmedia_test_camera_app) += test/ingenic_camera_app/main.c
include tools/build_elf.mk

module_name = output/player_test
LDFLAGS-$(APP_libmedia_test_player) = -lutils2 -lmedia_ffmpeg
src-$(APP_libmedia_test_player) += test/main_player_test.c
include tools/build_elf.mk

module_name = output/font_test
src-$(APP_libmedia_test_font) += test/main_font_test.c
include tools/build_elf.mk

module_name = output/jpeg_test
src-$(APP_libmedia_test_jpeg_decode) += test/main_turbo_jpeg_decode_test.c
include tools/build_elf.mk

module_name = output/rotater_test
src-$(APP_libmedia_test_rotate) += test/main_rotate_test.c
include tools/build_elf.mk

module_name = output/overlayer_test
src-$(APP_libmedia_test_overlayer) += test/main_overlay_test.c
include tools/build_elf.mk

module_name = output/scaler_test
# fix compile error, modify by yhy on 041524
LDFLAGS-$(APP_libmedia_test_scale) = -lmedia_ffmpeg
src-$(APP_libmedia_test_scale) += test/main_scale_test.c
include tools/build_elf.mk

module_name = output/convert_test
src-$(APP_libmedia_test_convert) +=test/main_convert_test.c
include tools/build_elf.mk

module_name = output/uvc_reader
src-$(APP_libmedia_test_uvc_reader) += test/main_uvc_reader_test.c
include tools/build_elf.mk

module_name = output/uvc_jpeg_to_nv12
src-$(APP_libmedia_test_uvc_jpeg_to_nv12) += test/main_uvc_jpeg_to_nv12_test.c
include tools/build_elf.mk

module_name = output/uvc_h264_to_mp4
LDFLAGS-$(APP_libmedia_test_uvc_h264_to_mp4) = -lmedia_ffmpeg
src-$(APP_libmedia_test_uvc_h264_to_mp4) += test/main_uvc_h264_to_mp4_test.c
include tools/build_elf.mk

module_name = output/uvc_h264_preview
src-$(APP_libmedia_test_uvc_h264_preview) += test/main_uvc_h264_preview_test.c
include tools/build_elf.mk

module_name = output/uvc_h264_2d_rotator_preview
src-$(APP_libmedia_test_uvc_h264_rotator_preview) += test/main_uvc_h264_2d_rotator_preview_test.c
include tools/build_elf.mk

module_name = output/uvc_jpegd_preview
src-$(APP_libmedia_test_uvc_jpeg_preview) += test/main_uvc_jpegd_2600_preview_test.c
include tools/build_elf.mk

module_name = output/uvc_jpegd_2d_rotator_preview
src-$(APP_libmedia_test_uvc_jpeg_rotator_preview) += test/main_uvc_jpegd_2d_rotator_preview_test.c
include tools/build_elf.mk

module_name = output/hw_h264_decoder_test
LDFLAGS-$(APP_libmedia_test_hw_h264_decoder) = -lmedia_ffmpeg
src-$(APP_libmedia_test_hw_h264_decoder) += test/main_hw_h264_decoder_test.c
include tools/build_elf.mk

module_name = output/aac_encode_test
LDFLAGS-$(APP_libmedia_test_aac_encode) = -lmedia_ffmpeg
src-$(APP_libmedia_test_aac_encode) += test/main_aac_encode.c
include tools/build_elf.mk

module_name = output/aac_decode_test
LDFLAGS-$(APP_libmedia_test_aac_encode) = -lmedia_ffmpeg
src-$(APP_libmedia_test_aac_decode) += test/main_aac_decode.c
include tools/build_elf.mk

module_name = output/aec_test
src-$(APP_libmedia_test_speex_aec) += test/main_aec_test.c
include tools/build_elf.mk

module_name = output/aec_cc_test
src-$(APP_libmedia_test_speex_cc_aec) += test/main_aec_cc_test.c
include tools/build_elf.mk

# ---------------------------
# the end
# ---------------------------
all_targets: $(module_targets)
	@echo "  compiled $(all_modules)"

apps = $(filter-out %.so,$(all_modules))

ifneq ($(apps),)
define install_apps
	$(Q)cp -f $(apps) $(FS_TARGET_DIR)/usr/bin/
	@echo "  installed $(apps)"
endef
define clean_install_apps
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/bin/, $(notdir $(apps)))
	@echo "  removed $(apps)"
endef
endif

is_install_uvc_preview_res:=n
ifeq ($(APP_libmedia_test_uvc_jpeg_rotator_preview), y)
	is_install_uvc_preview_res=y
endif
ifeq ($(APP_libmedia_test_uvc_jpeg_preview), y)
	is_install_uvc_preview_res=y
endif

ifeq ($(is_install_uvc_preview_res), y)
define install_uvc_preview_res
	$(Q)cp -rf test/uvc_logo_res $(FS_TARGET_DIR)/usr/
endef
define clean_install_uvc_preview_res
	$(Q)rm -drf $(FS_TARGET_DIR)/usr/uvc_logo_res
endef
endif

install:
	$(install_apps)
	$(install_uvc_preview_res)

clean_install:
	$(clean_install_apps)
	$(clean_install_uvc_preview_res)

clean:
	$(Q)rm -rf $(module_clean_files) $(all_modules)

.PHONY: all clean all_targets clean_install install
