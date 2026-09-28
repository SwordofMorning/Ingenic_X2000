ifeq ($(APP_br_yaffs2_mount_usr_data),y)
define SYSTEM_CONFIG_MOUNT_USR_DATA_YAFFS2
	cp -vrf rootfs_config/file/yaffs2/S21mount_yaffs2 $(TARGET_DIR)/etc/init.d/
	mkdir -p $(TARGET_DIR)/usr/data/
endef
else
define SYSTEM_CONFIG_MOUNT_USR_DATA_YAFFS2
	rm -f $(TARGET_DIR)/etc/init.d/S21mount_yaffs2
endef
endif

define SYSTEM_CONFIG_SET_YAFFS2
	$(SYSTEM_CONFIG_MOUNT_USR_DATA_YAFFS2)
endef
