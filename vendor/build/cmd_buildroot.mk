
define CMD_MAKE_BUILDROOT
	@echo "building buildroot"
	@echo "-------------------------"
	$(Q)cp .config.in $(BR_DIR)/../buildroot_patch/.config.in
	$(Q)if [ ! -e $(BR_DIR)/.config ]; then \
		cp -v $(APP_br_config_file) $(BR_DIR)/.config; \
	else \
		echo "keep the .config in buildroot, not override it!"; \
	fi
	make -C $(BR_DIR) --no-print-directory
	@echo ""
endef

define CMD_CLEAN_BUILDROOT
	@echo "cleaning buildroot"
	@echo "-------------------------"
	make -C $(BR_DIR) clean --no-print-directory
	$(Q)rm -f $(BR_DIR)/.config
	@echo ""
endef

define CMD_MAKE_SET_BUILDROOT_CONFIGS
	$(Q)if [ -e $(BR_DIR)/.config ]; then \
		cp $(BR_DIR)/.config $(BR_DIR)/.config.save ; \
		rm $(BR_DIR)/.config ; \
	fi

	$(Q)cp -v $(APP_br_config_file) $(BR_DIR)/.config
endef