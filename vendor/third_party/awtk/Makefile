# # 检查 .config.in include/config.h 是否有更新
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

export AWTK_DIR_NAME ?= awtk
export AWTK_DIR ?= .
export AWTK_INGENIC_DIR_NAME ?= awtk-ingenic
export obj_dir = output/.objs/

awtk_z = awtk-1.7.1.tar.gz
awtk_site=ftp://ftp.ingenic.com.cn/sz_ingenic/$(awtk_z)

all: awtk
	$(Q)+make $(MAKE_ARG) -f build_awtk_file_list.mk gen_awtk_file_list
	$(Q)echo "--- build awtk lib"
	$(Q)+make $(MAKE_ARG) -f build_awtk_lib.mk awtk_dynamic_lib
	$(Q)echo "--- build apps"
	$(Q)+make $(MAKE_ARG) -f build_awtk_apps.mk apps

$(awtk_z):
	$(Q)echo "--- downloading $(awtk_z)"
	$(Q)wget $(awtk_site)

awtk: $(awtk_z)
	$(Q)echo "--- extracting $(awtk_z)"
	$(Q)tar -xaf $(awtk_z)
	$(Q)touch awtk

clean:
	$(Q)+make $(MAKE_ARG) -f build_awtk_lib.mk clean_awtk_dynamic_lib
	$(Q)+make $(MAKE_ARG) -f build_awtk_apps.mk clean_apps

clean_install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)+make $(MAKE_ARG) -f build_awtk_lib.mk clean_install
	$(Q)+make $(MAKE_ARG) -f build_awtk_apps.mk clean_install

install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)+make $(MAKE_ARG) -f build_awtk_lib.mk install
	$(Q)+make $(MAKE_ARG) -f build_awtk_apps.mk install
