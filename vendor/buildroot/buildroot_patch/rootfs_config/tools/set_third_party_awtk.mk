SYSTEM_CLEAR_THIRD_PARTY_AWTK:=

ifeq ($(APP_br_third_party_awtk),y)

ifeq ($(APP_br_awtk_demo_ui),y)
define SYSTEM_CONFIG_RUN_THIRD_PARTY_AWTK
	cp -vrf rootfs_config/file/third_party_awtk/S61run_third_party_awtk $(TARGET_DIR)/etc/init.d/
endef
else
	SYSTEM_CLEAR_THIRD_PARTY_AWTK += rm -f $(TARGET_DIR)/etc/init.d/S61run_third_party_awtk;
endif

else
	SYSTEM_CLEAR_THIRD_PARTY_AWTK += rm -f $(TARGET_DIR)/etc/init.d/S61run_third_party_awtk;
endif

define SYSTEM_CONFIG_SET_THIRD_PARTY_AWTK
	$(SYSTEM_CLEAR_THIRD_PARTY_AWTK)
	$(SYSTEM_CONFIG_RUN_THIRD_PARTY_AWTK)
endef