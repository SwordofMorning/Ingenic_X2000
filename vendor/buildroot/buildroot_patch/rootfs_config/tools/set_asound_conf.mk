ifeq ($(APP_br_asound_conf_dmix_softvol),y)
SYSTEM_CONFIG_ASOUND_CONF_FILE_PATH=rootfs_config/file/alsa/asound_dmix_with_softvol.conf
else ifeq ($(APP_br_asound_conf_use_dmix),y)
SYSTEM_CONFIG_ASOUND_CONF_FILE_PATH=rootfs_config/file/alsa/asound_dmix.conf
else ifeq ($(APP_br_asound_conf_use_softvol),y)
SYSTEM_CONFIG_ASOUND_CONF_FILE_PATH=rootfs_config/file/alsa/asound_softvol.conf
endif

ifeq ($(APP_br_asound_conf_use_dmix),y)
define SYSTEM_CONFIG_ASOUND_CONF_DMIX_FILE
	sed -i 's/FMT/$(APP_br_asound_conf_dmix_format)/g' $(TARGET_DIR)/etc/asound.conf
	sed -i 's/RATE/$(APP_br_asound_conf_dmix_rate)/g' $(TARGET_DIR)/etc/asound.conf
	sed -i 's/CHANNELS/$(APP_br_asound_conf_dmix_channels)/g' $(TARGET_DIR)/etc/asound.conf
	sed -i 's/PERIOD_SIZE/$(APP_br_asound_conf_dmix_period_size)/g' $(TARGET_DIR)/etc/asound.conf
	sed -i 's/BUF_SIZE/$(APP_br_asound_conf_dmix_buffer_size)/g' $(TARGET_DIR)/etc/asound.conf
endef
endif

ifeq ($(APP_br_asound_conf_use_softvol),y)
define SYSTEM_CONFIG_ASOUND_CONF_SOFTVOL_FILE
	sed -i 's/MIN_dB/$(APP_br_asound_conf_softvol_min_dB)/g' $(TARGET_DIR)/etc/asound.conf
	sed -i 's/MAX_dB/$(APP_br_asound_conf_softvol_max_dB)/g' $(TARGET_DIR)/etc/asound.conf
	sed -i 's/RESOLUTION/$(APP_br_asound_conf_softvol_resolution)/g' $(TARGET_DIR)/etc/asound.conf
endef
endif

ifeq ($(APP_br_asound_conf_config),y)
define SYSTEM_CONFIG_ASOUND_CONF_FILE
	cp $(SYSTEM_CONFIG_ASOUND_CONF_FILE_PATH) $(TARGET_DIR)/etc/asound.conf
	sed -i 's/CODEC_NAME/$(APP_br_asound_conf_codec_name)/g' $(TARGET_DIR)/etc/asound.conf
	$(SYSTEM_CONFIG_ASOUND_CONF_DMIX_FILE)
	$(SYSTEM_CONFIG_ASOUND_CONF_SOFTVOL_FILE)
endef
else
define SYSTEM_CONFIG_ASOUND_CONF_FILE
	rm -f $(TARGET_DIR)/etc/asound.conf
endef
endif

ifeq ($(APP_br_asound_conf_keep),y)
define SYSTEM_CONFIG_SET_ASOUND_CONF
endef
else
define SYSTEM_CONFIG_SET_ASOUND_CONF
	$(SYSTEM_CONFIG_ASOUND_CONF_FILE)
endef
endif
