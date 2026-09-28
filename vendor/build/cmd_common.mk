
define common_make_hook
	$(Q)make -C $1 all $(MAKE_ARG)
endef

define common_clean_hook
	$(Q)make -C $1 clean $(MAKE_ARG)
	$(Q)make -C $1 clean_install $(MAKE_ARG)
endef

define common_install_hook
	$(Q)make -C $1 install $(MAKE_ARG)
endef

# foreach 时命令输出之间的间隔,否则会交错在一起
define CMD_SEPARATOR
	$(warning, \n)
	$(warning, \n)
endef

define CMD_MAKE
	@echo "building $1"
	@echo "-------------------------"
	$(call $($1_package_make_hook),$($1_package_path))
	$(call $($1_package_install_hook),$($1_package_path))
	@echo ""
endef

define CMD_CLEAN
	@echo "cleaning $1"
	@echo "-------------------------"
	$(call $($1_package_clean_hook),$($1_package_path))
	@echo ""
endef

define CMD_CLEAN_APPS
	$(foreach package,$(packages),$(call CMD_CLEAN,$(package))$(CMD_SEPARATOR))
endef

define CMD_MAKE_APPS
	$(foreach package,$(packages),$(call CMD_MAKE,$(package))$(CMD_SEPARATOR))
endef
