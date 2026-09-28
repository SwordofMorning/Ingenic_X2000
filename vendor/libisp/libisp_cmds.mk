include .config.in

#
# 定义默认目标
#
all: all_targets

#
# 编译选项
#
CFLAGS = -Os
ifeq ($(APP_libisp_x2000), y)
CFLAGS += -Iinclude/libisp/ -include isp_tuning.h
endif
ifeq ($(APP_libisp_x2580), y)
CFLAGS += -Iinclude/libisp/ -include isp_tuning_x2580.h
endif
CFLAGS += -Iinclude/libisp -include isp.h
CFLAGS += -include config.h -Wall -Werror
CFLAGS += -Wno-pointer-sign
CFLAGS += -Wno-pointer-to-int-cast

LDFLAGS = -Loutput/ -lisp

obj_dir = .objs/

# ----------------------
# 编译 cmds
# ----------------------

module_name = output/cmd_isp
src-$(APP_libisp_isp_cmd) += src/cmds/isp_main.c
include tools/build_elf.mk

ifeq ($(APP_libisp_x2000), y)
module_name = output/demo_isp
src-$(APP_libisp_demo_isp_cmd) += src/demo/demo_isp.c
include tools/build_elf.mk

module_name = output/demo_isp_jpeg_encode
src-$(APP_libisp_demo_isp_jpeg_encode_cmd) += src/demo/demo_isp_jpeg_encode.c
LDFLAGS-$(APP_libisp_demo_isp_jpeg_encode_cmd) := -L../libhardware2/output/ -lhardware2
include tools/build_elf.mk

module_name = output/demo_isp_h264_encode
src-$(APP_libisp_demo_isp_h264_encode_cmd) += src/demo/demo_isp_h264_encode.c
LDFLAGS-$(APP_libisp_demo_isp_h264_encode_cmd) := -L../libhardware2/output/ -lhardware2
include tools/build_elf.mk

module_name = output/demo_isp_nv12_preview
src-$(APP_libisp_demo_isp_nv12_preview_cmd) += src/demo/demo_isp_nv12_preview.c
LDFLAGS-$(APP_libisp_demo_isp_nv12_preview_cmd) := -L../libhardware2/output/ -lhardware2
include tools/build_elf.mk

module_name = output/demo_mult_chan_preview
src-$(App_libisp_mult_chan_preview_cmd) += src/demo/demo_mult_chan_preview.c
LDFLAGS-$(App_libisp_mult_chan_preview_cmd) := -L../libhardware2/output/ -lhardware2 -lpthread
include tools/build_elf.mk

module_name = output/demo_frame_sync
src-$(App_libisp_demo_frame_sync_cmd) += src/demo/demo_frame_sync.c
LDFLAGS-$(App_libisp_mult_chan_preview_cmd) := -L../libhardware2/output/ -lhardware2 -lpthread
include tools/build_elf.mk
endif


ifeq ($(APP_libisp_x2580), y)
module_name = output/demo_isp_x2580
src-$(APP_libisp_demo_isp_x2580_cmd) += src/demo/demo_isp_x2580.c
include tools/build_elf.mk

module_name = output/demo_isp_jpeg_encode_x2580
LDFLAGS-$(APP_libisp_demo_isp_jpeg_encode_x2580_cmd) := -L../libhardware2/output/ -lhardware2
LDFLAGS-$(APP_libisp_demo_isp_jpeg_encode_x2580_cmd) += -L../libutils2/output/ -lutils2
src-$(APP_libisp_demo_isp_jpeg_encode_x2580_cmd) += src/demo/demo_isp_jpeg_encode_x2580.c
include tools/build_elf.mk

module_name = output/demo_isp_h264_encode_x2580
LDFLAGS-$(APP_libisp_demo_isp_h264_encode_x2580_cmd) := -L../libhardware2/output/ -lhardware2
LDFLAGS-$(APP_libisp_demo_isp_h264_encode_x2580_cmd) += -L../libutils2/output/ -lutils2
src-$(APP_libisp_demo_isp_h264_encode_x2580_cmd) += src/demo/demo_isp_h264_encode_x2580.c
include tools/build_elf.mk
endif

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

clean_install:
	$(clean_install_cmds)

clean:
	$(Q)rm -rf output/
	$(Q)rm -rf $(module_clean_files)

.PHONY: all clean all_targets clean_install install
