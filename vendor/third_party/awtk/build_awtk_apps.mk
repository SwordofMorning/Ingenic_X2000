
# ------------------------------------------------------------
# 编译/清除 awtk tools
# ------------------------------------------------------------
include config.mk
include .config.in

CFLAGS += -Wall
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -O2
CFLAGS += -include config.h

LDFLAGS += -Wl,--gc-sections
LDFLAGS += -lhardware2 -lutils2 -lpthread -l2d
LDFLAGS += -lm -ldl -lstdc++ -lrt
LDFLAGS += -lawtk -L./output/bin -Wl,-rpath=./
CFLAGS += -include config.h -I./
CFLAGS += -DLV_CONF_INCLUDE_SIMPLE

include .awtk_cflags.mk

src-$(APP_awtk_demoui) += $(AWTK_DIR)/$(AWTK_DIR_NAME)/demos/assets.c
src-$(APP_awtk_demoui) += $(AWTK_DIR)/$(AWTK_DIR_NAME)/demos/demo_ui_app.c
module_name := output/bin/awtk_demo_ui
include tools/build_elf.mk

apps: $(module_targets)
	$(Q)echo "  $(all_modules)"

clean_apps:
	$(Q)rm -rf $(module_clean_files)

install:
ifdef all_modules
	$(Q)cp $(all_modules) $(FS_TARGET_DIR)/usr/bin/
endif
	@echo "  installed $(all_modules)"

clean_install:
ifdef all_modules
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(all_modules)))
endif
	@echo "  removed $(all_modules)"

.PHONY: apps apps_clean install clean_install $(all_modules)
