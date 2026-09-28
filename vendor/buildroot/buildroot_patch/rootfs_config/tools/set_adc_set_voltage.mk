shell_file = cmd_adc_set_voltage

ifeq ($(APP_br_adc_set_vddio_cim_voltage),y)
	shell_file += dvp_adc_ch=$(APP_br_adc_set_vddio_cim_adc_ch)
	shell_file += dvp_R0=$(APP_br_adc_set_vddio_cim_r0)
	shell_file += dvp_R1=$(APP_br_adc_set_vddio_cim_r1)
endif

ifeq ($(APP_br_adc_set_vddio_sd_voltage),y)
	shell_file += sd_adc_ch=$(APP_br_adc_set_vddio_sd_adc_ch)
	shell_file += sd_R0=$(APP_br_adc_set_vddio_sd_r0)
	shell_file += sd_R1=$(APP_br_adc_set_vddio_sd_r1)
endif

define SYSTEM_CONFIG_GENERATE_SHELL_FILE
	echo $(shell_file) > $(TARGET_DIR)/usr/bin/adc_set_voltage.sh
	chmod +x $(TARGET_DIR)/usr/bin/adc_set_voltage.sh
endef

ifeq ($(APP_br_adc_set_voltage),y)
define SYSTEM_CONFIG_ADC_SET_VOLTAGE
	cp -f rootfs_config/file/adc_set_voltage/S15set_voltage $(TARGET_DIR)/etc/init.d/
endef
else
define SYSTEM_CONFIG_ADC_SET_VOLTAGE
	rm -f $(TARGET_DIR)/etc/init.d/S15set_voltage
	rm -f $(TARGET_DIR)/usr/bin/adc_set_voltage.sh
endef
endif

define SYSTEM_CONFIG_SET_ADC_SET_VOLTAGE
	$(SYSTEM_CONFIG_GENERATE_SHELL_FILE)
	$(SYSTEM_CONFIG_ADC_SET_VOLTAGE)
endef
