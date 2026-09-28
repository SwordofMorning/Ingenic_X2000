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
CFLAGS += -include config.h
LDFLAGS = -lutils2 -limpf -limp -lalog -lrt -lpthread
# -lswscale -lavcodec -lavformat -lavutil -lssl -lcurl -lcrypto -lnghttp2 -lcares -lpthread

obj_dir = .objs/

# ----------------------
# 编译 cmds
# ----------------------
module_name = libcommon.a
src-y += src/cmds/common.c
src-y += src/cmds/video_common.c
include tools/build_elf.mk

module_name = output/demo_base_interface
src-$(APP_libimpf_base_interface_cmd) += src/cmds/demo_base_interface.c
src-$(APP_libimpf_base_interface_cmd) += libcommon.a
include tools/build_elf.mk

module_name = output/demo_dec_jpeg
src-$(APP_libimpf_dec_jpeg_cmd) += src/cmds/demo_dec_jpeg.c
src-$(APP_libimpf_dec_jpeg_cmd) += libcommon.a
include tools/build_elf.mk

module_name = output/demo_enc_h264_jpeg
src-$(APP_libimpf_enc_h264_jpeg_cmd) += src/cmds/demo_enc_h264_jpeg.c
src-$(APP_libimpf_enc_h264_jpeg_cmd) += libcommon.a
include tools/build_elf.mk

module_name = output/demo_fs_bgra_display
src-$(APP_libimpf_fs_bgra_display_cmd) += src/cmds/demo_fs_bgra_display.c
src-$(APP_libimpf_fs_bgra_display_cmd) += libcommon.a
include tools/build_elf.mk

module_name = output/demo_fs_nv12
src-$(APP_libimpf_fs_nv12_cmd) += src/cmds/demo_fs_nv12.c
src-$(APP_libimpf_fs_nv12_cmd) += libcommon.a
include tools/build_elf.mk

module_name = output/demo_fs_raw
src-$(APP_libimpf_fs_raw_cmd) += src/cmds/demo_fs_raw.c
src-$(APP_libimpf_fs_raw_cmd) += libcommon.a
include tools/build_elf.mk

module_name = output/demo_isp
src-$(APP_libimpf_demo_isp_cmd) += src/cmds/demo_isp.c
src-$(APP_libimpf_demo_isp_cmd) += libcommon.a
include tools/build_elf.mk

module_name = output/demo_osd_enc_jpeg
src-$(APP_libimpf_demo_osd_enc_jpeg_cmd) += src/cmds/demo_osd_enc_jpeg.c
src-$(APP_libimpf_demo_osd_enc_jpeg_cmd) += libcommon.a
include tools/build_elf.mk

module_name = output/demo_host_uvc
src-$(APP_libimpf_demo_host_uvc_cmd) += src/cmds/demo_host_uvc.c
src-$(APP_libimpf_demo_host_uvc_cmd) += src/cmds/image_convert.c
src-$(APP_libimpf_demo_host_uvc_cmd) += libcommon.a
include tools/build_elf.mk

module_name = output/demo_device_uvc
src-$(APP_libimpf_demo_device_uvc_cmd) += src/cmds/demo_device_uvc.c
src-$(APP_libimpf_demo_device_uvc_cmd) += src/cmds/uvc-gadget.c
src-$(APP_libimpf_demo_device_uvc_cmd) += libcommon.a
include tools/build_elf.mk
# ---------------------------
# the end
# ---------------------------
all_targets: $(module_targets)
	@echo "  compiled $(all_modules)"

cmds = $(filter-out %.so %.a,$(all_modules))
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

clean_install:
	$(clean_install_cmds)

clean:
	$(Q)rm -rf output/
	$(Q)rm -rf $(module_clean_files)

.PHONY: all clean all_targets clean_install install
