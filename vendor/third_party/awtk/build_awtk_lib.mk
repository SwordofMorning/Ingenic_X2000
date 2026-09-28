# ------------------------------------------------------------
# 编译/清除 libawtk.so
# ------------------------------------------------------------
include config.mk
include .config.in
 
CFLAGS += -std=gnu99 -Wall -fno-strict-aliasing -fPIC
CFLAGS += -Os
CXXFLAGS += -Wno-attributes -Wno-unused-local-typedefs -Wno-unused-variable -fPIC

ifdef APP_awtk_WITH_INGENIC_MSA_G2D
CFLAGS += -mmsa
endif

sinclude .awtk_files.mk
sinclude .awtk_cflags.mk

module_name = output/bin/libawtk.so
include tools/build_elf.mk

awtk_dynamic_lib: $(module_targets)
	@echo "  $(all_modules)"

clean_awtk_dynamic_lib:
	$(Q)rm -f $(module_clean_files)

install:
	$(Q)cp $(all_modules) $(FS_TARGET_DIR)/usr/lib/
ifdef APP_awtk_WITH_FS_RES
	$(Q)cp $(AWTK_DIR)/$(AWTK_DIR_NAME)/res/assets $(FS_TARGET_DIR)/usr/ -drf
endif
	@echo "  installed $(all_modules)"

clean_install:
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(all_modules)))
	$(Q)rm $(FS_TARGET_DIR)/usr/assets -drf
	@echo "  removed $(all_modules)"

.PHONY: awtk_dynamic_lib_clean awtk_dynamic_lib install clean_install
