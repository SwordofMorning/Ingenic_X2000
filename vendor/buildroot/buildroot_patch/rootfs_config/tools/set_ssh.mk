ifeq ($(APP_br_ssh),y)

define SYSTEM_CONFIG_MV_SSH
	rootfs_config/tools/mv_etc_ssh.sh $(TARGET_DIR)
	cp $(APP_br_ssh_config_file) $(TARGET_DIR)/save/etc/ssh/
	cp rootfs_config/file/ssh/S30cp_ssh $(TARGET_DIR)/etc/init.d/
endef

endif

define SYSTEM_CONFIG_SET_SSH
	$(SYSTEM_CONFIG_MV_SSH)
endef