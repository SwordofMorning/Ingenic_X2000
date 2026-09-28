
# 检查 .config.in include/config.h 是否有更新
make_sure_config_update:=$(shell tools/check_config.sh $(config_in) 2> /dev/null)
make_sure_output_is_ok:=$(shell mkdir -p output/)

CROSS_COMPILE?=../output/host/usr/bin/mips-linux-gnu-

export V
export Q
export CROSS_COMPILE
export FS_TARGET_DIR
export FS_STAGING_DIR

# make V=1 可以看见Makefile运转的细节
ifneq ($(V),1)
Q = @
MAKE_ARG = --no-print-directory
else
Q =
MAKE_ARG =
endif

all:
	$(Q)+make $(MAKE_ARG) -f libmedia.mk all
	$(Q)+make $(MAKE_ARG) -f libmedia_apps.mk all

clean:
	$(Q)+make $(MAKE_ARG) -f libmedia.mk clean
	$(Q)+make $(MAKE_ARG) -f libmedia_apps.mk clean
	$(Q)rm -rf output/ .objs/

clean_install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(if $(FS_STAGING_DIR),,$(error must set FS_STAGING_DIR))
	$(Q)+make $(MAKE_ARG) -f libmedia.mk clean_install
	$(Q)+make $(MAKE_ARG) -f libmedia_apps.mk clean_install

install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(if $(FS_STAGING_DIR),,$(error must set FS_STAGING_DIR))
	$(Q)+make $(MAKE_ARG) -f libmedia.mk install
	$(Q)+make $(MAKE_ARG) -f libmedia_apps.mk install

.PHONY: all clean install

