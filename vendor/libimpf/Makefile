
# 检查 .config.in include/config.h 是否有更新
make_sure_config_update:=$(shell tools/check_config.sh $(config_in) 2> /dev/null)

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

include .config.in
ifeq ($(APP_libimpf_v120), y)
LIBIMPF_SRC_DIR = x1520
endif

ifeq ($(APP_libimpf_v130), y)
LIBIMPF_SRC_DIR = x1830
endif

LIB = src/lib/$(LIBIMPF_SRC_DIR)

INCLUDE = include/$(LIBIMPF_SRC_DIR)

all:
	$(Q)cp -f $(LIB)/libimpf.so $(FS_TARGET_DIR)/usr/lib/
	$(Q)cp -f $(LIB)/libimpf.so $(FS_STAGING_DIR)/usr/lib/
	$(Q)cp -f $(LIB)/libimp.so $(FS_TARGET_DIR)/usr/lib/
	$(Q)cp -f $(LIB)/libimp.so $(FS_STAGING_DIR)/usr/lib/
	$(Q)cp -f $(LIB)/libalog.so $(FS_TARGET_DIR)/usr/lib/
	$(Q)cp -f $(LIB)/libalog.so $(FS_STAGING_DIR)/usr/lib/
	$(Q)cp -rf $(INCLUDE)/impf $(FS_STAGING_DIR)/usr/include/
	$(Q)cp -rf $(INCLUDE)/imp $(FS_STAGING_DIR)/usr/include/
	$(Q)+make $(MAKE_ARG) -f libimpf_cmds.mk all

clean:
	$(Q)+make $(MAKE_ARG) -f libimpf_cmds.mk clean

clean_install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(if $(FS_STAGING_DIR),,$(error must set FS_STAGING_DIR))
	$(Q)rm -f $(FS_TARGET_DIR)/usr/lib/libimpf.so
	$(Q)rm -f $(FS_STAGING_DIR)/usr/lib/libimpf.so
	$(Q)rm -f $(FS_TARGET_DIR)/usr/lib/libimp.so
	$(Q)rm -f $(FS_STAGING_DIR)/usr/lib/libimp.so
	$(Q)rm -f $(FS_TARGET_DIR)/usr/lib/libalog.so
	$(Q)rm -f $(FS_STAGING_DIR)/usr/lib/libalog.so
	$(Q)rm -rf $(FS_STAGING_DIR)/usr/include/impf
	$(Q)rm -rf $(FS_STAGING_DIR)/usr/include/imp
	$(Q)+make $(MAKE_ARG) -f libimpf_cmds.mk clean_install

install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(if $(FS_STAGING_DIR),,$(error must set FS_STAGING_DIR))
	$(Q)cp -f $(LIB)/libimpf.so $(FS_TARGET_DIR)/usr/lib/
	$(Q)cp -f $(LIB)/libimpf.so $(FS_STAGING_DIR)/usr/lib/
	$(Q)cp -f $(LIB)/libimp.so $(FS_TARGET_DIR)/usr/lib/
	$(Q)cp -f $(LIB)/libimp.so $(FS_STAGING_DIR)/usr/lib/
	$(Q)cp -f $(LIB)/libalog.so $(FS_TARGET_DIR)/usr/lib/
	$(Q)cp -f $(LIB)/libalog.so $(FS_STAGING_DIR)/usr/lib/
	$(Q)cp -rf $(INCLUDE)/impf $(FS_STAGING_DIR)/usr/include/
	$(Q)cp -rf $(INCLUDE)/imp $(FS_STAGING_DIR)/usr/include/
	$(Q)+make $(MAKE_ARG) -f libimpf_cmds.mk install

.PHONY: all clean install

