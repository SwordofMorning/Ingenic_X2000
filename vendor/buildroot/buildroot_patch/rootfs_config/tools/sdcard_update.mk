ifeq ($(APP_br_sdcard_update),y)
define SYSTEM_CONFIG_SDCARD_UPDATE
	cp -vrf $(APP_br_sdcard_update_file_path) $(TARGET_DIR)/etc/init.d/S99sdcard_update
endef
else
define SYSTEM_CONFIG_SDCARD_UPDATE
	rm -f $(TARGET_DIR)/etc/init.d/S99sdcard_update
endef
endif
