#
# 定义默认目标
#
all: all_targets

#
# 编译选项
#
CFLAGS = -Os -fPIC -Wall -Werror -Wextra -DPIC -fpic -std=gnu99
CFLAGS += -Iinclude/
CFLAGS += -include config.h
CFLAGS += -Wno-pointer-sign
CFLAGS += -Wno-pointer-to-int-cast

obj_dir = .objs/

# ----------------------
# 编译 libisp.so
# ----------------------
LDFLAGS = -shared -lm -lasound
LDFLAGS += -L../libiniparser/output/ -liniparser
module_name = output/libasound_module_pcm_aeq.so

src-y += src/aeq.c
src-y += src/common.c

include tools/build_elf.mk

# ---------------------------
# the end
# ---------------------------
all_targets: $(module_targets)
	@echo "  compiled $(all_modules)"

libs = $(filter %.so,$(all_modules))

ifneq ($(libs),)
define install_libs
	$(Q)mkdir -p $(FS_TARGET_DIR)/usr/lib/alsa-lib/
	$(Q)mkdir -p $(FS_STAGING_DIR)/usr/lib/alsa-lib/
	$(Q)cp -f $(libs) $(FS_TARGET_DIR)/usr/lib/alsa-lib/
	$(Q)cp -f $(libs) $(FS_STAGING_DIR)/usr/lib/alsa-lib/
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/aeq/
	$(Q)mkdir -p $(FS_STAGING_DIR)/etc/aeq/
	$(Q)cp -f config/default_config.ini $(FS_TARGET_DIR)/etc/aeq/config.ini
	$(Q)cp -f config/default_config.ini $(FS_STAGING_DIR)/etc/aeq/config.ini
	@echo "  installed $(libs) include/"
endef
define clean_install_libs
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/alsa-lib/, $(notdir $(libs)))
	$(Q)rm -f $(addprefix $(FS_STAGING_DIR)/usr/lib/alsa-lib/, $(notdir $(libs)))
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/aeq
	$(Q)rm -rf $(FS_STAGING_DIR)/etc/aeq
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
