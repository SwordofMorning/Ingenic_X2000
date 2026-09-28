define CMD_MAKE_UBOOT
	@echo "building uboot"
	@echo "-------------------------"
	PATH=$(UBOOT_TOOLCHAIN_DIR):$$PATH make -C $(UBOOT_DIR) distclean --no-print-directory
	PATH=$(UBOOT_TOOLCHAIN_DIR):$$PATH make -C $(UBOOT_DIR) $(APP_uboot_config) $(THREAD_ARG) --no-print-directory
	@echo ""
endef

define CMD_CLEAN_UBOOT
	@echo "cleaning uboot"
	@echo "-------------------------"
	PATH=$(UBOOT_TOOLCHAIN_DIR):$$PATH make -C $(UBOOT_DIR) distclean --no-print-directory
	@echo ""
endef
