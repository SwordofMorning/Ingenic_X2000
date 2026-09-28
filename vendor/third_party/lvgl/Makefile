# 检查 .config.in include/config.h 是否有更新
make_sure_config_update:=$(shell tools/check_config.sh $(config_in) 2> /dev/null)
make_sure_output_is_ok:=$(shell mkdir -p output/)

CROSS_COMPILE?=../../buildroot/buildroot/output/host/usr/bin/mips-linux-gnu-

export V
export Q
export CROSS_COMPILE
export FS_TARGET_DIR

# make V=1 可以看见Makefile运转的细节
ifneq ($(V),1)
Q = @
MAKE_ARG = --no-print-directory
else
Q =
MAKE_ARG =
endif

export LVGL_DIR_NAME ?= lvgl
export LVGL_DIR ?= .
export obj_dir = .objs/

lvgl_z = lvgl.8.39.tar.bz2
lvgl_site=ftp://ftp.ingenic.com.cn/sz_ingenic/$(lvgl_z)

patchs += patchs/ingenic_sw_blend.patch

define patch_file
	$(Q)patch -p1 -N -s  -d lvgl < $1
endef

define CMD_SEPARATOR
	$(warning, \n)
	$(warning, \n)
endef

all: lvgl
	$(Q)if [ ! -f lv_conf.h ]; then cp lv_conf_default.h lv_conf.h; fi
	$(Q)echo "--- generate lvgl file list"
	$(Q)+make $(MAKE_ARG) -f build_lvgl_file_list.mk gen_lvgl_file_list
	$(Q)echo "--- build lvgl lib"
	$(Q)+make $(MAKE_ARG) -f build_lvgl_lib.mk lvgl_dynamic_lib
	$(Q)echo "--- build apps"
	$(Q)+make $(MAKE_ARG) -f build_lvgl_apps.mk apps

$(lvgl_z):
	$(Q)echo "--- downloading $(lvgl_z)"
	$(Q)wget $(lvgl_site)

lvgl: $(lvgl_z)
	$(Q)echo "--- extracting $(lvgl_z)"
	$(Q)tar -xf $(lvgl_z)
	$(Q)touch lvgl
	$(Q)echo "--- patching lvgl"
	$(foreach file,$(patchs),$(call patch_file, $(file))$(CMD_SEPARATOR))

clean:
	$(Q)rm -rf lv_conf.h
	$(Q)+make $(MAKE_ARG) -f build_lvgl_lib.mk clean_lvgl_dynamic_lib
	$(Q)+make $(MAKE_ARG) -f build_lvgl_apps.mk clean_apps

clean_install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)+make $(MAKE_ARG) -f build_lvgl_lib.mk clean_install
	$(Q)+make $(MAKE_ARG) -f build_lvgl_apps.mk clean_install

install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)+make $(MAKE_ARG) -f build_lvgl_lib.mk install
	$(Q)+make $(MAKE_ARG) -f build_lvgl_apps.mk install
