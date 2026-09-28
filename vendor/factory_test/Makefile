
# 检查 .config.in include/config.h 是否有更新
make_sure_config_update:=$(shell tools/check_config.sh $(config_in) 2> /dev/null)

sinclude ../build/.config.in
sinclude .config.in

output_dir = factory_test
test_conf_file = factory_test/factory_test_conf
wifi_conf_file = factory_test/wpa_supplicant.conf
to_adc_keyboard_code = $(if $($(1)),$(shell tools/key_to_code.sh $($(1))),-1)



ifeq ($(APP_test_keyboard),y)
define KEYBOARD_TEST
	$(Q)echo 'enable_keyboard_test=y' >> $(test_conf_file)
	$(Q)echo -n 'keys="' >> $(test_conf_file)
	$(Q)echo -n '$(filter-out -1,$(call to_adc_keyboard_code,APP_test_keyboard_KEY1_CODE))' >> $(test_conf_file)
	$(Q)echo -n ' $(filter-out -1,$(call to_adc_keyboard_code,APP_test_keyboard_KEY2_CODE))' >> $(test_conf_file)
	$(Q)echo -n ' $(filter-out -1,$(call to_adc_keyboard_code,APP_test_keyboard_KEY3_CODE))' >> $(test_conf_file)
	$(Q)echo -n ' $(filter-out -1,$(call to_adc_keyboard_code,APP_test_keyboard_KEY4_CODE))' >> $(test_conf_file)
	$(Q)echo -n ' $(filter-out -1,$(call to_adc_keyboard_code,APP_test_keyboard_KEY5_CODE))' >> $(test_conf_file)
	$(Q)echo -n ' $(filter-out -1,$(call to_adc_keyboard_code,APP_test_keyboard_KEY6_CODE))' >> $(test_conf_file)
	$(Q)echo -n ' $(filter-out -1,$(call to_adc_keyboard_code,APP_test_keyboard_KEY7_CODE))' >> $(test_conf_file)
	$(Q)echo    ' $(filter-out -1,$(call to_adc_keyboard_code,APP_test_keyboard_KEY8_CODE))"' >> $(test_conf_file)
endef
endif

ifeq ($(APP_test_pwm),y)
define PWM_TEST
	$(Q)echo 'enable_pwm_test=y' >> $(test_conf_file)
	$(Q)echo 'pwm_gpio=$(APP_test_pwm_gpio)' >> $(test_conf_file)
	$(Q)echo 'pwm_max_level=$(APP_test_pwm_max_level)' >> $(test_conf_file)
	$(Q)echo 'pwm_freq=$(APP_test_pwm_freq)' >> $(test_conf_file)
	$(Q)echo 'pwm_active_level=$(APP_test_pwm_active_level)' >> $(test_conf_file)
	$(Q)echo 'pwm_accuracy_priority=$(APP_test_pwm_accuracy_priority)' >> $(test_conf_file)
endef
endif

ifeq ($(APP_test_lcd),y)
define LCD_TEST
	$(Q)echo 'enable_fb_test=y' >> $(test_conf_file)
	$(Q)echo 'fb_dev=$(APP_test_lcd_dev)' >> $(test_conf_file)
endef
endif

ifeq ($(APP_test_camera),y)
define CAMERA_TEST
	$(Q)echo 'enable_camera_test=y' >> $(test_conf_file)
endef
endif

ifeq ($(APP_test_wifi),y)
define WIFI_TEST
	$(Q)echo 'enable_wifi_test=y' >> $(test_conf_file)
	$(Q)echo 'ctrl_interface=/var/run/wpa_supplicant' > $(wifi_conf_file)
	$(Q)echo 'update_config=1' >> $(wifi_conf_file)
	$(Q)echo 'country=GB' >> $(wifi_conf_file)
	$(Q)echo 'network={' >> $(wifi_conf_file)
	$(Q)echo '	ssid="$(APP_test_wifi_ssid)"' >> $(wifi_conf_file)
	$(Q)echo '	psk="$(APP_test_wifi_psk)"' >> $(wifi_conf_file)
	$(Q)echo '}' >> $(wifi_conf_file)
endef
endif

ifeq ($(APP_test_mac),y)
define MAC_TEST
	$(Q)echo 'enable_mac_test=y' >> $(test_conf_file)
	$(Q)echo 'mac_ip_addr=$(APP_test_mac_ip_addr)' >> $(test_conf_file)
	$(Q)echo 'mac_mask=$(APP_test_mac_mask)' >> $(test_conf_file)
	$(Q)echo 'mac_gw=$(APP_test_mac_gw)' >> $(test_conf_file)
endef
endif

define BT_TEST_BCM
	$(Q)echo 'enable_bt_bcm_test=y' >> $(test_conf_file)
	$(Q)echo 'bt_detect_time=$(APP_test_bt_detect_time)' >> $(test_conf_file)

	if [ $(MD_BT_BSA_XBURST1) = y ];then \
		cp ../wireless/bcm/bin/xburst1_bsa/app/app_manager factory_test/; \
	else \
		cp ../wireless/bcm/bin/xburst2_bsa/app/app_manager factory_test/; \
	fi
endef

define BT_TEST_REALTEK
	$(Q)echo 'enable_bt_realtek_test=y' >> $(test_conf_file)
endef

ifeq ($(APP_test_bt),y)
define BT_TEST
	$(BT_TEST_$(APP_test_bt_select))
endef
endif

ifeq ($(APP_test_amic),y)
define AMIC_TEST
	$(Q)echo 'enable_amic_test=y' >> $(test_conf_file)
	$(Q)echo 'amic_channel=$(APP_test_amic_channel)' >> $(test_conf_file)
endef
endif

ifeq ($(APP_test_dmic),y)
define DMIC_TEST
	$(Q)echo 'enable_dmic_test=y' >> $(test_conf_file)
	$(Q)echo 'dmic_channel=$(APP_test_dmic_channel)' >> $(test_conf_file)
endef
endif

ifeq ($(APP_test_speak),y)
define SPEAK_TEST
	$(Q)echo 'enable_speak_test=y' >> $(test_conf_file)
	cp audio_pcm.aiff factory_test/
endef
endif

all:
	$(shell if [ ! -e $(output_dir) ];then mkdir -p $(output_dir); fi)

	echo -n '' > $(test_conf_file)
	$(PWM_TEST)
	$(LCD_TEST)
	$(CAMERA_TEST)
	$(KEYBOARD_TEST)
	$(WIFI_TEST)
	$(MAC_TEST)
	$(AMIC_TEST)
	$(DMIC_TEST)
	$(SPEAK_TEST)
	$(BT_TEST)


clean_install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)rm -f $(FS_TARGET_DIR)/etc/init.d/S99factory_test_shell
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/factory_test/
	$(Q)echo "  removed S99factory_test_shell "
	$(Q)echo "  removed etc/factory_test "

install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)cp S99factory_test_shell $(FS_TARGET_DIR)/etc/init.d/
	$(Q)cp -rf factory_test $(FS_TARGET_DIR)/etc/


.PONHY: clean install clean_install

clean:
	@rm -rf factory_test
