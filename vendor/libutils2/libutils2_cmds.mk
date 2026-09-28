include .config.in

#
# 定义默认目标
#
all: all_targets

#
# 编译选项
#
CFLAGS = -Os
CFLAGS += -Iinclude/
CFLAGS += -include config.h

LDFLAGS = -Loutput/ -lutils2

obj_dir = .objs/

# ----------------------
# 编译 cmds
# ----------------------

module_name = output/show_boot_time
src-$(APP_libutils2_boot_time_cmd) += src/cmds/show_boot_time_main.c
include tools/build_elf.mk

module_name = output/cmd_uevent
src-$(APP_libutils2_uevent_cmd) += src/cmds/cmd_uevent.c
include tools/build_elf.mk

module_name = output/cmd_udp
src-$(APP_libutils2_udp_cmd) += src/cmds/cmd_udp.c
include tools/build_elf.mk

module_name = output/cmd_usb_device_state
src-$(APP_libutils2_usb_device_state_cmd) += src/cmds/cmd_usb_device_state.c
include tools/build_elf.mk

module_name = output/cmd_rotate_test
LDFLAGS-$(APP_libutils2_rotate_test) += -lm
src-$(APP_libutils2_rotate_test) += src/cmds/cmd_rotate_test.c
include tools/build_elf.mk

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
