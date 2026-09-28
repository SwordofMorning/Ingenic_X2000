include .config.in

#
# 定义默认目标
#
all: all_targets

#
# 编译选项
#
CFLAGS = -Os -fPIC -Wall -Werror
ifeq ($(APP_libisp_x2000), y)
CFLAGS += -Iinclude/libisp/ -include isp_tuning.h
endif
ifeq ($(APP_libisp_x2580), y)
CFLAGS += -Iinclude/libisp/ -include isp_tuning_x2580.h
endif
CFLAGS += -Iinclude/libisp -include isp.h
CFLAGS += -include config.h
CFLAGS += -Wno-pointer-sign
CFLAGS += -Wno-pointer-to-int-cast

obj_dir = .objs/

# ----------------------
# 编译 libisp.so
# ----------------------
LDFLAGS = -lpthread
module_name = output/libisp.so
src-y += src/lib/isp.c

ifeq ($(APP_libisp_x2000), y)
src-y += src/lib/isp_tuning.c
endif
ifeq ($(APP_libisp_x2580), y)
src-y += src/lib/isp_tuning_x2580.c
endif

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
	$(Q)cp -rf include/libisp $(FS_STAGING_DIR)/usr/include/
	@echo "  installed $(libs) include/"
endef
define clean_install_libs
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(libs)))
	$(Q)rm -f $(addprefix $(FS_STAGING_DIR)/usr/lib/, $(notdir $(libs)))
	$(Q)rm -rf $(FS_STAGING_DIR)/usr/include/libisp
	@echo "  removed $(libs) include/libisp"
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
