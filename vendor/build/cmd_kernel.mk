TARGET_IMAGE=xImage

ifdef APP_kernel_uImage
TARGET_IMAGE=uImage
endif

ifdef APP_kernel_uImage_split
TARGET_IMAGE=uImage_split
endif

define CMD_MAKE_KERNEL
	@echo "building kernel"
	@echo "-------------------------"
	$(Q)if [ ! -e $(KERNEL_DIR)/.config ]; then \
		echo "config $(APP_kernel_config) to .config"; \
		PATH=$(TOOLCHAIN_DIR):$$PATH make -C $(KERNEL_DIR) $(APP_kernel_config) --no-print-directory; \
	else \
		echo "keep the .config in kernel, not override it!"; \
	fi
	PATH=$(TOOLCHAIN_DIR):$$PATH make -C $(KERNEL_DIR) $(TARGET_IMAGE) modules $(THREAD_ARG) --no-print-directory
	@echo ""
endef

define CMD_CLEAN_KERNEL
	@echo "cleaning kernel"
	@echo "-------------------------"
	PATH=$(TOOLCHAIN_DIR):$$PATH make -C $(KERNEL_DIR) clean --no-print-directory
	$(Q)rm -f $(KERNEL_DIR)/.config
	@echo ""
endef

define CMD_MAKE_SET_KERNEL_CONFIGS
	$(Q)if [ -e $(KERNEL_DIR)/.config ]; then \
		cp $(KERNEL_DIR)/.config $(KERNEL_DIR)/.config.save ; \
		rm $(KERNEL_DIR)/.config ; \
	fi

	$(Q)PATH=$(TOOLCHAIN_DIR):$$PATH make -C $(KERNEL_DIR) $(APP_kernel_config) --no-print-directory
endef