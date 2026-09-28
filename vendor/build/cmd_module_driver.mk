
define CMD_MAKE_MODULE_DRIVER
	rm -rf $(MD_DIR)/output/*
	PATH=$(TOOLCHAIN_DIR):$$PATH make KERNEL_DIR=$(KERNEL_DIR) -C $(MD_DIR) config_in=$(config_in_file) --no-print-directory
endef

package_name-$(APP_module_driver) = module_driver
package_make_hook := CMD_MAKE_MODULE_DRIVER
include tools/package_common.mk
