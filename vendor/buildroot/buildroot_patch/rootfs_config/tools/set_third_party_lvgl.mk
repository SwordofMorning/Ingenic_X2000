SYSTEM_CLEAR_THIRD_PARTY_LVGL:=

ifeq ($(APP_br_third_party_lvgl),y)

ifeq ($(APP_br_lvgl_demo_widgets),y)
define SYSTEM_CONFIG_RUN_THIRD_PARTY_LVGL
	cp -vrf rootfs_config/file/third_party_lvgl/S31run_third_party_lvgl $(TARGET_DIR)/etc/init.d/
endef
else
	SYSTEM_CLEAR_THIRD_PARTY_LVGL += rm -f $(TARGET_DIR)/etc/init.d/S31run_third_party_lvgl;
endif

ifeq ($(APP_br_lvgl_video_player),y)
define SYSTEM_CONFIG_RUN_THIRD_PARTY_LVGL
	cp -vrf rootfs_config/file/third_party_lvgl/S32run_third_party_video_player $(TARGET_DIR)/etc/init.d/
endef
else
	SYSTEM_CLEAR_THIRD_PARTY_LVGL += rm -f $(TARGET_DIR)/etc/init.d/S32run_third_party_video_player;
endif

ifeq ($(APP_br_lvgl_demo_lock2),y)
define SYSTEM_CONFIG_RUN_THIRD_PARTY_LVGL
	cp -vrf rootfs_config/file/third_party_lvgl/S33run_third_party_demo_ilock2 $(TARGET_DIR)/etc/init.d/
endef
else
	SYSTEM_CLEAR_THIRD_PARTY_LVGL += rm -f $(TARGET_DIR)/etc/init.d/S33run_third_party_demo_ilock2;
endif

ifeq ($(APP_br_lvgl_demo_lock_tuya),y)
define SYSTEM_CONFIG_RUN_THIRD_PARTY_LVGL
	cp -vrf rootfs_config/file/third_party_lvgl/S34run_third_party_demo_ilock_tuya $(TARGET_DIR)/etc/init.d/
endef
else
	SYSTEM_CLEAR_THIRD_PARTY_LVGL += rm -f $(TARGET_DIR)/etc/init.d/S34run_third_party_demo_ilock_tuya;
endif

else
	SYSTEM_CLEAR_THIRD_PARTY_LVGL += rm -f $(TARGET_DIR)/etc/init.d/S31run_third_party_lvgl;
	SYSTEM_CLEAR_THIRD_PARTY_LVGL += rm -f $(TARGET_DIR)/etc/init.d/S32run_third_party_video_player;
	SYSTEM_CLEAR_THIRD_PARTY_LVGL += rm -f $(TARGET_DIR)/etc/init.d/S33run_third_party_demo_ilock2;
	SYSTEM_CLEAR_THIRD_PARTY_LVGL += rm -f $(TARGET_DIR)/etc/init.d/S34run_third_party_demo_ilock_tuya;
endif

define SYSTEM_CONFIG_SET_THIRD_PARTY_LVGL
	$(SYSTEM_CLEAR_THIRD_PARTY_LVGL)
	$(SYSTEM_CONFIG_RUN_THIRD_PARTY_LVGL)
endef