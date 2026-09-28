ifeq ($(APP_br_wait_mcu_boot_animation),y)
define SYSTEM_CONFIG_WAIT_MCU_BOOT_ANIMATION
	cp -vrf rootfs_config/file/boot_animation/S30wait_mcu_boot_animation $(TARGET_DIR)/etc/init.d/
	mkdir -p $(TARGET_DIR)/usr/data/
endef
else
define SYSTEM_CONFIG_WAIT_MCU_BOOT_ANIMATION
	rm -f $(TARGET_DIR)/etc/init.d/S30wait_mcu_boot_animation
endef
endif

define SYSTEM_CONFIG_SET_BOOT_ANIMATION
	$(SYSTEM_CONFIG_WAIT_MCU_BOOT_ANIMATION)
endef
