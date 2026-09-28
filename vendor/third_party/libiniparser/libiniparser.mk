
#
# 定义默认目标
#
all: all_targets

#
# 编译选项
#
CFLAGS = -Os -fPIC
CFLAGS += -Iinclude/
CFLAGS += -include config.h
CFLAGS += -Wno-pointer-sign
CFLAGS += -Wno-pointer-to-int-cast

obj_dir = .objs/

# ----------------------
# 编译 libiniparser.so
# ----------------------
LDFLAGS =
module_name = output/libiniparser.so

src-y += src/iniparser.c
src-y += src/dictionary.c

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
	$(Q)cp -rf include/libiniparser $(FS_STAGING_DIR)/usr/include/
	@echo "  installed $(libs) include/"
endef
define clean_install_libs
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(libs)))
	$(Q)rm -f $(addprefix $(FS_STAGING_DIR)/usr/lib/, $(notdir $(libs)))
	$(Q)rm -rf $(FS_STAGING_DIR)/usr/include/libiniparser
	@echo "  removed $(libs) include/libiniparser/"
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
